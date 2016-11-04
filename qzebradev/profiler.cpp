#include "profiler.h"
#include <QDebug>
#include <QCoreApplication>
#include <QPair>

using namespace QZebraDev;

Profiler* Profiler::s_profiler(0);
Profiler::Options Profiler::m_options;

struct Sleep : public QThread { using QThread::msleep; };

#define MAIN_THREAD_INDEX 0

Profiler::Profiler()
    : QObject(), m_printer(0)
{
    connect(this, SIGNAL(detectorStarted(int)), &m_detector.timer, SLOT(start(int)), Qt::QueuedConnection);
    connect(this, SIGNAL(detectorStoped()), &m_detector.timer, SLOT(stop()), Qt::QueuedConnection);
    connect(&m_detector.timer, SIGNAL(timeout()), this, SLOT(th_checkLongFuncs()), Qt::DirectConnection);

    m_detector.timer.moveToThread(&m_detector.thread);

    setup(Options(), new Printer());
}

Profiler::~Profiler()
{
    s_profiler = 0;
    delete m_printer;

    if (m_detector.enabled) {
        emit detectorStoped();
        m_detector.thread.exit();
    }

    {
        QMutexLocker locker(&m_funcs.mutex);
        for (int i = 0; i < m_funcs.threads.count(); ++i) {
            if (i == MAIN_THREAD_INDEX && m_detector.enabled) {
                m_detector.mutex.lock();
            }

            FuncTimers timers = m_funcs.timers[i];
            qDeleteAll(timers);
            m_funcs.timers[i].clear();

            if (i == MAIN_THREAD_INDEX && m_detector.enabled) {
                m_detector.mutex.unlock();
            }
        }
    }
    
    {
        QMutexLocker locker(&m_steps.mutex);
        qDeleteAll(m_steps.timers);
        m_steps.timers.clear();
    }
}

void Profiler::setup(const Options &opt, Printer *printer)
{
    m_options = opt;
    m_options.funcsMaxThreadCount = m_options.funcsMaxThreadCount < 1 ? 1 : m_options.funcsMaxThreadCount;

    //! Func timers
    m_funcs.timers.fill(FuncTimers(), m_options.funcsMaxThreadCount);
    m_funcs.threads.fill(0, m_options.funcsMaxThreadCount);
    m_funcs.threads[MAIN_THREAD_INDEX] = reinterpret_cast<quintptr>(qApp->thread());

    //! Long func detector
    if (m_options.longFuncDetectorEnabled) {

        m_detector.enabled = true;
        m_detector.thread.start();
        emit detectorStarted(m_options.longFuncThreshold * 0.45);

    } else {

        emit detectorStoped();
        m_detector.enabled = false;
        m_detector.thread.exit();
    }

    if (printer) {
        delete m_printer;
        m_printer = printer;
    }
}

void Profiler::stepTime(const QString &tag, const QString &info, bool isRestart)
{
    QMutexLocker locker(&m_steps.mutex);
    StepTimer *stepTimer = m_steps.timers.value(tag, NULL);
    if (!stepTimer) {
        stepTimer = new StepTimer();
        stepTimer->start();
        m_steps.timers.insert(tag, stepTimer);
    }
    
    if (isRestart) {
        stepTimer->start();
    }

    printer()->printStep(tag, stepTimer->beginMs(), stepTimer->stepMs(), info);
    
    stepTimer->nextStep();
}

void Profiler::beginFunc(const QString &func, quintptr th)
{
    int index = m_funcs.threadIndex(th);
    if (index == -1) {
        index = m_funcs.addThread(th);
        if (index == -1) {
            return;
        }
    }

    m_detector.lockIfNeed(index);

    FuncTimer *timer = m_funcs.timers[index].value(&func, NULL);
    if (!timer) {
        timer = new FuncTimer(func);
        m_funcs.timers[index].insert(&func, timer);
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
    int index = m_funcs.threadIndex(th);
    if (index == -1) {
        return;
    }

    m_detector.lockIfNeed(index);
    
    FuncTimer *timer = m_funcs.timers[index].value(&func, NULL);
    if (timer) {
        if (timer->timer.isValid()) { //! NOTE If not valid then it is probably a recursive call

            timer->callcount++;
            timer->calltimeMs = timer->timer.nsecsElapsed() * 0.000001; //! NOTE To millisecond
            timer->sumtimeMs += timer->calltimeMs;
            timer->timer.invalidate();

            if (m_options.funcsTraceEnabled) {
                printer()->printTrace(func, timer->calltimeMs, timer->callcount, timer->sumtimeMs);
            }

            if (index == MAIN_THREAD_INDEX && m_detector.enabled && timer->calltimeMs > m_options.longFuncThreshold) {
                printer()->printEndLongFunc(func, timer->calltimeMs);
            }
        }
    }

    m_detector.unlockIfNeed(index);
}

const QString& Profiler::staticInfo(const QString &info)
{
    int count = m_funcs.staticInfo.count();
    for (int i = 0; i < count; ++i) {
        if (m_funcs.staticInfo.at(i) == info) {
            return m_funcs.staticInfo.at(i);
        }
    }

    m_funcs.staticInfo.append(info);
    return m_funcs.staticInfo.last();
}

double Profiler::StepTimer::beginMs() const
{
    return this->beginTime.nsecsElapsed() * 0.000001; //! NOTE To millisecond
}

double Profiler::StepTimer::stepMs() const
{
    return this->stepTime.nsecsElapsed() * 0.000001; //! NOTE To millisecond
}

void Profiler::StepTimer::start()
{
    this->beginTime.start();
    this->stepTime.start();
}
void Profiler::StepTimer::restart()
{
    this->beginTime.restart();
    this->stepTime.restart();
}

void Profiler::StepTimer::nextStep()
{
   this-> stepTime.restart();
}

int Profiler::FuncsData::threadIndex(quintptr th) const
{
    int count = this->threads.count();
    for (int i = 0; i < count; ++i) {
        quintptr thi = this->threads[i];
        if (thi == th) {
            return i;
        }
        
        if (thi == 0) {
            return -1;
        }
    }
    return -1;
}

int Profiler::FuncsData::addThread(quintptr th)
{
    QMutexLocker locker(&this->mutex);
    int index = threadIndex(th);
    if (index > -1) {
        return index;
    }
    
    int count = this->threads.count();
    for (int i = 0; i < count; ++i) {
        quintptr thi = this->threads[i];
        if (thi == 0) {
            this->threads[i] = th;
            return i;
        }
    }
    return -1;
}

void Profiler::clear()
{
    QMutexLocker flocker(&m_funcs.mutex);
    for (int i = 0; i < m_funcs.threads.count(); ++i) {

        m_detector.lockIfNeed(i);

        FuncTimers timers = m_funcs.timers[i];
        qDeleteAll(timers);
        m_funcs.timers[i] = FuncTimers();
        m_funcs.threads[i] = 0;

        m_detector.unlockIfNeed(i);
    }
    m_funcs.threads[MAIN_THREAD_INDEX] = reinterpret_cast<quintptr>(qApp->thread());

    QMutexLocker slocker(&m_steps.mutex);
    qDeleteAll(m_steps.timers);
    m_steps.timers.clear();
}

Profiler::Data Profiler::threadsData(Data::Mode mode) const
{
    if (mode != Data::OnlyOther) {
        m_detector.lockIfNeed(MAIN_THREAD_INDEX);
    }

    Data data;
    data.mainThread = m_funcs.threads[MAIN_THREAD_INDEX];

    for (int i = 0; i < m_funcs.threads.count(); ++i) {

        quintptr th = m_funcs.threads[i];
        if (th == 0) {
            break;
        }

        if (i == MAIN_THREAD_INDEX) {
            if (mode == Data::OnlyOther) {
                continue;
            }
        } else {
            if (mode == Data::OnlyMain) {
                continue;
            }
        }

        Data::Thread thdata;
        thdata.thread = th;

        FuncTimers timers = m_funcs.timers[i];
        foreach (const FuncTimer* ft, timers) {
            Data::Func f(
                        ft->func,
                        ft->callcount,
                        ft->callcount ? (ft->sumtimeMs / static_cast<double>(ft->callcount)) : 0,
                        ft->sumtimeMs
                        );

            thdata.funcs << f;
        }

        data.threads[thdata.thread] = thdata;

        if (i == MAIN_THREAD_INDEX) {
            if (mode == Data::OnlyMain) {
                break;
            }
        }
    }

    if (mode != Data::OnlyOther) {
        m_detector.unlockIfNeed(MAIN_THREAD_INDEX);
    }

    return data;
}

QString Profiler::threadsDataString(Data::Mode mode) const
{
    Profiler::Data data = threadsData(mode);
    return printer()->formatData(data, mode, m_options.dataTopCount);
}

void Profiler::printThreadsData(Data::Mode mode) const
{
    Profiler::Data data = threadsData(mode);
    printer()->printData(data, mode, m_options.dataTopCount);
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

struct IsLessByTime {
    bool operator()(const QPair<quint64, QString> &f, const QPair<quint64, QString> &s)
    {
        return f.first > s.first;
    }
};

//! NOTE Сalled timeout timer, which is in background thread
void Profiler::th_checkLongFuncs()
{
    QList< QPair<quint64, QString> > longFuncs;

    { // находим незавершенные зависшие функции

        m_detector.needLock = true;
        Sleep::msleep(10); //! NOTE Let's wait to come out of the thread of execution beginFunc and endFunc

        QMutexLocker locker(&m_detector.mutex);
        foreach (const FuncTimer *f, m_funcs.timers[MAIN_THREAD_INDEX]) {
            if (f->timer.isValid()) {
                qint64 elapsed = f->timer.elapsed();
                if (elapsed > m_options.longFuncThreshold) {
                    longFuncs.append(QPair<qint64, QString>(elapsed, QString("%1: %2 ms").arg(f->func).arg(elapsed)));
                }
            }
        }
    }

    m_detector.needLock = false;
    std::sort(longFuncs.begin(), longFuncs.end(), IsLessByTime()); //! NOTE Sorting by time, as a result we obtain a stack of the main thread

    QStringList funcs;
    for (int i = 0; i < longFuncs.count(); ++i) {
        funcs.append(longFuncs.at(i).second);
    }

    if (!funcs.isEmpty()) {
        printer()->printLongFuncs(funcs);
    }
}

const Profiler::Options& Profiler::options()
{
    return m_options;
}

Profiler::Printer* Profiler::printer() const
{
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

void Profiler::Printer::printStep(const QString &tag, double beginMs, double stepMs, const QString &info)
{
    static const QString COLON(" : ");
    static const QChar SEP('/');
    static const QString MS(" ms: ");

    QString str;
    str.reserve(100);

    str
            .append(tag)
            .append(COLON)
            .append(QString::number(beginMs, 'f', 3))
            .append(SEP)
            .append(QString::number(stepMs, 'f', 3))
            .append(MS)
            .append(info);

    printDebug(str);
}

void Profiler::Printer::printTrace(const QString& func, double calltimeMs, qint64 callcount, double sumtimeMs)
{
    static const QString CALLTIME("calltime: ");
    static const QString CALLCOUNT("callcount: ");
    static const QString SUMTIME("sumtime: ");

    if (calltimeMs > 10 || callcount%100 == 1.) { //! NOTE Anti spam
        QString str;
        str.reserve(100);
        str
                .append(func)
                .append(CALLTIME).append(QString::number(calltimeMs, 'f', 3))
                .append(CALLCOUNT).append(QString::number(callcount))
                .append(SUMTIME).append(QString::number(sumtimeMs, 'f', 3));

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

void Profiler::Printer::printEndLongFunc(const QString &func, double calltimeMs)
{
    QString str;
    str.reserve(100);
    str
            .append("Long: ")
            .append(QString::number(calltimeMs, 'f', 3))
            .append(" ms, func: ")
            .append(func);

    printInfo(str);
}

void Profiler::Printer::printData(const Data &data, Data::Mode mode, int maxcount)
{
    printInfo(formatData(data, mode, maxcount));
}

struct IsLessBySum {
    bool operator()(const Profiler::Data::Func &f, const Profiler::Data::Func &s)
    {
        return f.sumtime > s.sumtime;
    }
};

QString Profiler::Printer::formatData(const Data &data, Data::Mode mode, int maxcount) const
{
    QString str;
    QTextStream stream(&str);
    stream << "\n\n";

    if (mode == Data::OnlyMain || mode == Data::All) {

        Data::Thread thdata = data.threads[data.mainThread];
        std::sort(thdata.funcs.begin(), thdata.funcs.end(), IsLessBySum());
        funcsToStream(stream, QString("Main thread. Top %1 by sum time (total count: %2)").arg(maxcount).arg(thdata.funcs.count()), thdata.funcs, maxcount);
    }

    if (mode == Data::OnlyOther || mode == Data::All) {

        QList<Data::Func> funcs;
        QHash<quintptr, Data::Thread>::ConstIterator it = data.threads.constBegin(), end = data.threads.constEnd();
        while (it != end) {

            if (it.key() != data.mainThread) {
                funcs << it.value().funcs;
            }

            ++it;
        }

        std::sort(funcs.begin(), funcs.end(), IsLessBySum());
        funcsToStream(stream, QString("Other threads. Top %1 by sum time (total count: %2)").arg(maxcount).arg(funcs.count()), funcs, maxcount);
    }

    return str;
}

#define FORMAT(str, width) QString(str).leftJustified(width, ' ', true).append("  ").toLatin1().constData()
#define TITLE(str) FORMAT(QString(str), 18)
#define VALUE(val, unit) FORMAT(QString::number(val) + unit, 18)
#define VALUE_D(val, unit) FORMAT(QString::number(val, 'f', 3) + unit, 18)

void Profiler::Printer::funcsToStream(QTextStream &stream, const QString &title, const QList<Data::Func> &funcs, int _count) const
{
    stream << title << "\n";
    stream << FORMAT("Function", 60) << TITLE("Call time") << TITLE("Call count") << TITLE("Sum time") << "\n";
    int count = funcs.count() < _count ? funcs.count() : _count;
    for (int i = 0; i < count; ++i) {
        const Data::Func &f = funcs.at(i);
        stream << FORMAT(f.func, 60) << VALUE_D(f.calltime, " ms") << VALUE(f.callcount, "") << VALUE_D(f.sumtime, " ms") << "\n";
    }
    stream << "\n\n";
}
