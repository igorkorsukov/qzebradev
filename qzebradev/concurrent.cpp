#include "concurrent.h"
#include <QThread>
#include <private/qthreadpool_p.h>
#include "getthreadpoolprivate.h"
#include "log.h"

using namespace QZebraDev;

Concurrent *Concurrent::s_concurent = 0;

#define FORMAT(str, width) QString(str).leftJustified(width, ' ', true).append("  ").toLatin1().constData()
#define TITLE(str) FORMAT(QString(str), 24)
#define VALUE(val) FORMAT(QString::number(val), 24)
#define VALUE_U(val, unit) FORMAT(QString::number(val) + " " + unit, 24)

bool Concurrent::debuging(false);
int Concurrent::tasks_increment(0);

Concurrent::Concurrent()
    : QObject(0)
{
    m_pools[Global] = new ThreadPool("Global", QThreadPool::globalInstance());
    m_pools[Network] = new ThreadPool("Network");
    m_pools[Cache] = new ThreadPool("Cache");
    m_pools[WriteDb] = new ThreadPool("WriteDb");
    m_pools[Script] = new ThreadPool("Script");
    m_pools[Other] = new ThreadPool("Other");
    
    if (debuging) {
        m_timer.setInterval(5000);
        connect(&m_timer, SIGNAL(timeout()), this, SLOT(updateMaxActive()));
        m_timer.start();
    }
}

Concurrent::~Concurrent()
{
    Concurrent::s_concurent = 0;
    qDeleteAll(m_pools);
}

void Concurrent::abort()
{
    TRACEFUNC;
    foreach (ThreadPool *pool, m_pools) {
        pool->pool->clear();
        QElapsedTimer timer;
        timer.start();
        LOGD() << "Started waitForDone pool:" << pool->name;
        pool->pool->waitForDone(1);
        LOGD() << "Finished waitForDone pool:" << pool->name << ", elapsed:" << timer.elapsed();
    }
}

Concurrent::ThreadPool* Concurrent::poolByType(const Type t) const
{
    ThreadPool *p = m_pools.value(t, NULL);
    IF_ASSERT(p) {
        p = m_pools.value(Other);
    }
    return p;
}

QThreadPool* Concurrent::requestPool(const Type t) const
{
    return poolByType(t)->pool;
}

const QString PATERN("Class = ");
QString Concurrent::taskInfo(const QString  &info)
{
    int bindex = info.indexOf(PATERN);
    if (bindex > -1) {
        bindex += PATERN.count();
        int eindex = info.indexOf(';', bindex);
        if (eindex == -1) {
            eindex = info.indexOf(']', bindex);
        }
        
        if (eindex == -1) {
            eindex = info.count() - 1;
        }
        
        return info.mid(bindex, eindex - bindex) + QString::number(++tasks_increment);
    }
    return info + QString::number(++tasks_increment);
}

void Concurrent::addTask(ThreadPool *pool, const QString &info)
{
    QMutexLocker locker(&pool->mutex);
    pool->tasks.append(TaskInfo(info));
    pool->tasks_wait++;
    pool->tasks_maxwait = qMax(pool->tasks_maxwait, pool->tasks_wait);
}

void Concurrent::startTask(ThreadPool *pool, const QString &info)
{
    QMutexLocker locker(&pool->mutex);
    for (int i = 0; i < pool->tasks.count(); ++i) {
        TaskInfo &t = pool->tasks[i];
        if (t.info == info)
            t.state = Started;
    }

    pool->tasks_wait--;
    pool->tasks_active++;
    pool->tasks_maxactive = qMax(pool->tasks_maxactive, pool->tasks_active);

    QMutexLocker clocker(&m_commonInfo.mutex);
    m_commonInfo.tasks_active++;
    m_commonInfo.tasks_maxactive = qMax(m_commonInfo.tasks_maxactive, m_commonInfo.tasks_active);
}

void Concurrent::finishTask(ThreadPool *pool, const QString &info, quint64 duration)
{
    QMutexLocker locker(&pool->mutex);
    for (int i = 0; i < pool->tasks.count(); ++i) {
        TaskInfo &t = pool->tasks[i];
        if (t.info == info) {
            t.state = Finished;
            t.duration = duration;
        }
    }
    
    pool->tasks_active--;
    
    QMutexLocker clocker(&m_commonInfo.mutex);
    m_commonInfo.tasks_active--;
}

void Concurrent::updateMaxActive()
{
    //TRACEFUNC;
    int threads_running_total = 0;
    for (Pools::Iterator it = m_pools.begin(), end = m_pools.end(); it != end; ++it) {
        ThreadPool* p = it.value();
        const QThreadPoolPrivate *pp = GetThreadPoolPrivate::pool_private(p->pool);
        const QSet<QThreadPoolThread *> &threads = pp->allThreads;
        int threads_running = 0;
        foreach (const QThreadPoolThread *_pth, threads) {
            QThread *pth = (QThread*)(_pth);
            quintptr th = (quintptr)pth;
            p->threads.insert(th);
            if (pth->isRunning()) {
                ++threads_running;
            }
            
            m_commonInfo.threads.insert(th);
        }
        
        p->threads_maxrunning = qMax(p->threads_maxrunning, threads_running);
        threads_running_total += threads_running;
    }
    
    m_commonInfo.threads_maxrunning = qMax(m_commonInfo.threads_maxrunning, threads_running_total);
}

void Concurrent::print()
{
    TRACEFUNC;
    QString str;
    QTextStream stream(&str);
    stream << "\n\nConcurrent info\n\n";
    
    stream << "Total info:\n";
    stream << TITLE("Threads_total:") << VALUE(m_commonInfo.threads.count()) << "\n";
    stream << TITLE("Threads_maxrunning:") << VALUE(m_commonInfo.threads_maxrunning) << "\n";
    stream << TITLE("Tasks_maxactive:") << VALUE(m_commonInfo.tasks_maxactive) << "\n";
    
    stream << "\n\n";
    
    for (Pools::ConstIterator it = m_pools.constBegin(), end = m_pools.constEnd(); it != end; ++it) {
        const ThreadPool* p = it.value();
        toStream(stream, p);
    }
    
    stream << "\n\n";
    
    LOGI() << str;
}

void Concurrent::toStream(QTextStream &stream, const ThreadPool *p) const
{
    stream << p->name << " threads\n";
    stream << TITLE("MaxThreadCount:") << VALUE(p->pool->maxThreadCount()) << "\n";
    stream << TITLE("ExpiryTimeout:") << VALUE(p->pool->expiryTimeout()) << "\n";
    stream << TITLE("Threads_total:") << VALUE(p->threads.count()) << "\n";
    stream << TITLE("Threads_maxrunning:") << VALUE(p->threads_maxrunning) << "\n";
    stream << TITLE("Tasks_total:") << VALUE(p->tasks.count()) << "\n";
    stream << TITLE("Tasks_maxactive:") << VALUE(p->tasks_maxactive) << "\n";
    stream << TITLE("Tasks_maxwait:") << VALUE(p->tasks_maxwait) << "\n";
    
    tasksToStream(stream, "Wait tasks:", p->tasks, Wait);
    tasksToStream(stream, "Started tasks:", p->tasks, Started);
    tasksToStream(stream, "Finished tasks:", p->tasks, Finished);
    
    stream << "\n\n";
}

void Concurrent::tasksToStream(QTextStream &stream, const QString &title, const QList<TaskInfo> &tasks, TaskState state) const
{
    stream << TITLE(title) << "\n";
    for(int i = 0; i < tasks.count(); ++i) {
        const TaskInfo &task = tasks.at(i);
        if (task.state == state) {
            stream << "    " << FORMAT(task.info, 32) << VALUE_U(task.duration, "ms") << "\n";
        }
    }
}
