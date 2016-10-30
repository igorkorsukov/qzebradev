#include "profiler.h"
#include <QCoreApplication>
#include <QPair>
#include "log.h"

using namespace QZebraDev;

Profiler* QZebraDev::Profiler::s_profiler = 0;
bool QZebraDev::Profiler::enabled(true);
bool QZebraDev::Profiler::trace(false);
bool QZebraDev::Profiler::longFuncDetectorEnabled(true);

struct Sleep : public QThread { using QThread::msleep; };

#define MAIN_THREAD_INDEX 0

#define LONG_FUNC_TRESHOLD 3000 // порог (в мс) после которого функция считается зависшей
#define LONG_FUNC_CHECK_PERIOD 3000 // интервал (в мс) с которым запускается проверка на зависшие функции

Profiler::Profiler()
    : m_printer(0)
{
    for (int i = 0; i < THREAD_MAX_COUNT; ++i) {
        m_funcThreads[i] = 0;
    }
    m_funcThreads[MAIN_THREAD_INDEX] = reinterpret_cast<quintptr>(qApp->thread());

    m_dummy = QString("Exhausted of a custom count of info, max 1000");

    if (longFuncDetectorEnabled) {
        m_detector.enabled = true;
        m_detector.timer.start(LONG_FUNC_CHECK_PERIOD);
        m_detector.timer.moveToThread(&m_detector.thread);
        connect(&m_detector.timer, SIGNAL(timeout()), this, SLOT(th_checkLongFuncs()), Qt::DirectConnection);
        m_detector.thread.start();
    }
}

Profiler::~Profiler()
{
    s_profiler = 0;
    delete m_printer;

    {
        QMutexLocker locker(&m_funcMutex);
        for (int i = 0; i < THREAD_MAX_COUNT; ++i) {
            if (i == MAIN_THREAD_INDEX && longFuncDetectorEnabled) {
                m_detector.mutex.lock();
            }

            FuncTimers timers = m_funcTimers[i];
            qDeleteAll(timers);

            if (i == MAIN_THREAD_INDEX && longFuncDetectorEnabled) {
                m_detector.mutex.unlock();
            }
        }
    }
    
    {
        QMutexLocker locker(&m_stepMutex);
        qDeleteAll(m_stepTimers);
        m_stepTimers.clear();
    }
}

void Profiler::stepTime(const QString &tag, const QString &info, bool isRestart)
{
    QMutexLocker locker(&m_stepMutex);
    StepTimer *stepTimer = m_stepTimers.value(tag, NULL);
    if (!stepTimer) {
        stepTimer = new StepTimer();
        stepTimer->start();
        m_stepTimers.insert(tag, stepTimer);
    }
    
    if (isRestart) {
        stepTimer->start();
    }

    printer()->printStep(stepTimer->beginMs(), stepTimer->stepMs(), info);
    
    stepTimer->nextStep();
}

void Profiler::beginFunc(const QString &func, quintptr th)
{
    int index = threadIndex(th);
    if (index == -1) {
        index = threadAdd(th);
        if (index == -1) {
            return;
        }
    }

    m_detector.lockIfNeed(index);

    FuncTimer *timer = m_funcTimers[index].value(&func, NULL);
    if (!timer) {
        timer = new FuncTimer(func);
        m_funcTimers[index].insert(&func, timer);
    } else {
        if (timer->timer.isValid()) {
            printer()->printDebug(QString("Recursion detected, measure only first call, func: ") + func);
            return;
        }
    }
    
    timer->timer.start();

    m_detector.unlockIfNeed(index);
}

void Profiler::endFunc(const QString &func, quintptr th)
{
    int index = threadIndex(th);
    if (index == -1) {
        return;
    }

    m_detector.lockIfNeed(index);
    
    FuncTimer *timer = m_funcTimers[index].value(&func, NULL);
    if (timer) {
        if (!timer->timer.isValid()) {
            //! NOTE Вероятно рекурсивный вызов
            return;
        }

        timer->callcount++;
        timer->calltime = timer->timer.nsecsElapsed();
        timer->sumtime += timer->calltime * 0.0000001;
        timer->timer.invalidate();
        
        if (Profiler::trace) {
            printer()->printTrace(func, timer->calltime, timer->callcount, timer->sumtime);
        }
    }

    m_detector.unlockIfNeed(index);
}

const QString& Profiler::static_info(const QString &info)
{
    for (int i = 0; i < m_static_info.count(); ++i) {
        if (m_static_info.at(i) == info) {
            return m_static_info.at(i);
        }
    }
    
    if (m_static_info.count() == 999) {
        return m_dummy;
    }
    
    m_static_info.append(info);
    return m_static_info.last();
}

int Profiler::threadIndex(quintptr th) const
{
    for (int i = 0; i < THREAD_MAX_COUNT; ++i) {
        quintptr thi = m_funcThreads[i];
        if (thi == th) {
            return i;
        }
        
        if (thi == 0) {
            return -1;
        }
    }
    return -1;
}

int Profiler::threadAdd(quintptr th)
{
    QMutexLocker locker(&m_funcMutex);
    int index = threadIndex(th);
    if (index > -1) {
        return index;
    }
    
    for (int i = 0; i < THREAD_MAX_COUNT; ++i) {
        quintptr thi = m_funcThreads[i];
        if (thi == 0) {
            m_funcThreads[i] = th;
            return i;
        }
    }
    return -1;
}

void Profiler::clear()
{
    QMutexLocker flocker(&m_funcMutex);
    for (int i = 0; i < THREAD_MAX_COUNT; ++i) {

        m_detector.lockIfNeed(i);

        FuncTimers timers = m_funcTimers[i];
        qDeleteAll(timers);
        m_funcTimers[i] = FuncTimers();
        m_funcThreads[i] = 0;

        m_detector.unlockIfNeed(i);
    }
    m_funcThreads[MAIN_THREAD_INDEX] = reinterpret_cast<quintptr>(qApp->thread());

    QMutexLocker slocker(&m_stepMutex);
    qDeleteAll(m_stepTimers);
    m_stepTimers.clear();
}

bool Profiler::isLessBySum(const Profiler::FuncTimer* f, const Profiler::FuncTimer* s)
{
    return f->sumtime > s->sumtime;
}

QString Profiler::mainString()
{
    ONLY_MAIN_THREAD; // т.к. longFuncDetectorEnabled thread unsafe

    m_detector.lockIfNeed(MAIN_THREAD_INDEX);

    QList<FuncTimer*> list = m_funcTimers[MAIN_THREAD_INDEX].values();
    QString result = prepare("Main thread. ", list);

    m_detector.unlockIfNeed(MAIN_THREAD_INDEX);

    return result;
}

QString Profiler::threadsString()
{
    FuncTimers timers;
    for (int i = 1; i < THREAD_MAX_COUNT; ++i) {
        quintptr th = m_funcThreads[i];
        if (th == 0) {
            break;
        }

        QList<FuncTimer*> list = m_funcTimers[i].values();
        for (int j = 0; j < list.count(); ++j) {
            FuncTimer *tht = list.at(j);
            FuncTimer *timer = timers.value(&tht->func, NULL);
            if (!timer) {
                timer = new FuncTimer(tht->func);
                timers.insert(&tht->func, timer);
            }

            timer->callcount += tht->callcount;
            timer->sumtime += tht->sumtime;
        }
    }
    
    QList<FuncTimer*> list = timers.values();
    QString str = prepare("Other threads. ", list);
    qDeleteAll(list);
    
    return str;
}

void Profiler::printMain()
{
    LOGI() << mainString();
}

void Profiler::printThreads()
{
    LOGI() << threadsString();
}

Profiler::Data Profiler::mainData() const
{
    ONLY_MAIN_THREAD; // т.к. longFuncDetectorEnabled thread unsafe

    m_detector.lockIfNeed(MAIN_THREAD_INDEX);

    QList<FuncTimer*> list = m_funcTimers[MAIN_THREAD_INDEX].values();
    qSort(list.begin(), list.end(), Profiler::isLessBySum);
    int count = 150;
    count = list.count() < count ? list.count() : count;
    Data data;
    for(int i(0); i < count; i++)
    {
        FuncTimer *info = list.at(i);
        QVariantMap obj;
        obj["Function"] = info->func;
        obj["Call time"] = info->callcount ? (info->sumtime / info->callcount) : 0;
        obj["Call count"] = info->callcount;
        obj["Sum time"] = info->sumtime;
        data.append(obj);
    }

    m_detector.unlockIfNeed(MAIN_THREAD_INDEX);

    return data;
}

void Profiler::LongFuncDetector::lockIfNeed(int index)
{
    if (index == MAIN_THREAD_INDEX && this->enabled && this->needLock) {
        this->mutex.lock();
        this->locked = true;
    }
}

void Profiler::LongFuncDetector::unlockIfNeed(int index)
{
    if (index == MAIN_THREAD_INDEX && this->locked) {
        this->mutex.unlock();
        this->locked = false;
    }
}

//! NOTE вызывается по таймауту таймера, который живет в другом потоке
void Profiler::th_checkLongFuncs()
{
    QList< QPair<quint64, QString> > longFuncs;

    { // находим незавершенные зависшие функции

        m_detector.needLock = true;
        Sleep::msleep(10); //! NOTE Подождём, чтобы вышли потоки исполнения из beginFunc и endFunc

        QMutexLocker locker(&m_detector.mutex);
        foreach (FuncTimer *f, m_funcTimers[MAIN_THREAD_INDEX]) {
            if (f->timer.isValid() && f->timer.elapsed() > LONG_FUNC_TRESHOLD) {
                const qint64 t = f->timer.nsecsElapsed();
                longFuncs.append(QPair<qint64, QString>(t, QString("%1: %2 sec")
                                                        .arg(f->func)
                                                        .arg(t / 1000000000.0)));
            }
        }
    }

    m_detector.needLock = false;
    qSort(longFuncs); //! NOTE сортируем по времени, в результате получим порядок стека главного потока

    QStringList funcs;
    for (int i = 0; i < longFuncs.count(); ++i) {
        funcs.append(longFuncs.at(i).second);
    }

    if (!funcs.isEmpty()) {
        printer()->printLongFuncs(funcs);
    }
}

void Profiler::setPrinter(Printer *printer)
{
    delete m_printer;
    m_printer = printer;
}

Profiler::Printer* Profiler::printer() const
{
    if (!m_printer) {
        m_printer = new Printer();
    }
    return m_printer;
}

Profiler::Printer::~Printer()
{}

void Profiler::Printer::printDebug(const QString &str)
{
    qDebug() << str;
}

void Profiler::Printer::printInfo(const QString &str)
{
    qDebug() << str;
}

void Profiler::Printer::printStep(qint64 beginMs, qint64 stepMs, const QString &info)
{
    static const QString SEP("/");
    static const QString MS(" ms");

    QString str;
    str.reserve(100);
    str
            .append(QString::number(beginMs))
            .append(SEP)
            .append(QString::number(stepMs))
            .append(MS)
            .append(info);

    printDebug(str);
}

void Profiler::Printer::printTrace(const QString& func, qint64 calltime, qint64 callcount, qint64 sumtime)
{
    static const QString CALLTIME("calltime:");
    static const QString CALLCOUNT("callcount:");
    static const QString SUMTIME("sumtime:");

    if (calltime > 10000 || callcount%100 == 1.) {
        QString str;
        str.reserve(100);
        str
                .append(func)
                .append(CALLTIME).append(QString::number(calltime))
                .append(CALLCOUNT).append(QString::number(callcount))
                .append(SUMTIME).append(QString::number(sumtime));

        printDebug(str);
    }
}

void Profiler::Printer::printLongFuncs(const QStringList &funcsStack)
{
    QString str;
    str.reserve(100);
    str
            .append("Long functions on main thread:\n")
            .append(funcsStack.join("\n"));

    printInfo(str);
}

#define FORMAT(str, width) QString(str).leftJustified(width, ' ', true).append("  ").toLatin1().constData()
#define TITLE(str) FORMAT(QString(str), 18)
#define VALUE(val, unit) FORMAT(QString::number(val) + unit, 18)
#define VALUE_D(val, unit) FORMAT(QString::number(val, 'f', 3) + unit, 18)

void Profiler::toStream(const QString &title, QTextStream &stream, const QList<FuncTimer*> &list, int _count) const
{
    stream << title << "\n";
    stream << FORMAT("Function", 60) << TITLE("Call time") << TITLE("Call count") << TITLE("Sum time") << "\n";
    
    int count = list.count() < _count ? list.count() : _count;
    for (int i = 0; i < count; ++i) {
        const FuncTimer *info = list.at(i);
        stream << FORMAT(info->func, 60) << VALUE_D(info->callcount ? (info->sumtime / info->callcount) : 0, " ms") << VALUE(info->callcount, "") << VALUE_D(info->sumtime, " ms") << "\n";
    }
    stream << "\n\n";
}

QString Profiler::prepare(const QString &title, QList<Profiler::FuncTimer *> &list) const
{
    QString str;
    QTextStream stream(&str);
    stream << "\n\n";
    
    qSort(list.begin(), list.end(), Profiler::isLessBySum);
    toStream(title + QString("Top 150 by sum time (total count: %1").arg(list.count()), stream, list, 150);
    
    return str;
}
