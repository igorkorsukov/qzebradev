#ifndef CONCURRENT_H
#define CONCURRENT_H

#include <QObject>
#include <QtConcurrent/QtConcurrentRun>
#include <QThreadPool>
#include <QTimer>
#include <QElapsedTimer>

namespace QZebraDev {

class Concurrent: public QObject
{    
    Q_OBJECT
    
    static Concurrent* s_concurent;
public:
    
    static Concurrent *instance() {
        if(!s_concurent)
            s_concurent = new Concurrent();
        return s_concurent;
    }
    
    enum Type {
        Global,
        Network,
        Cache,
        WriteDb,
        Script,
        Other
    };
    
    
    //
    template <typename T, typename Class>
    QFuture<T> run(Class *object, T (Class::*fn)(), Type type, const QString &info = QString())
    {
        Task<T, Class> task(type, object, fn, info);
        return QtConcurrent::run(task.pool->pool, task);
    }
    template <typename T, typename Class, typename Param1, typename Arg1>
    QFuture<T> run(Class *object, T (Class::*fn)(Param1), const Arg1 &arg1, Type type, const QString &info = QString())
    {
        TaskArg1<T, Class, Param1, Arg1> task(type, object, fn, arg1, info);
        return QtConcurrent::run(task.pool->pool, task);
    }
    
    //
    template <typename T, typename Class>
    QFuture<T> run(const Class *object, T (Class::*fn)() const, Type type, const QString &info = QString())
    {
        ConstTask<T, Class> task(type, object, fn, info);
        return QtConcurrent::run(task.pool->pool, task);
    }
    template <typename T, typename Class, typename Param1, typename Arg1>
    QFuture<T> run(const Class *object, T (Class::*fn)(Param1) const, const Arg1 &arg1, Type type, const QString &info = QString())
    {
        ConstTaskArg1<T, Class, Param1, Arg1> task(type, object, fn, arg1, info);
        return QtConcurrent::run(task.pool->pool, task);
    }
    
    //
    template <typename FunctionObject>
    QFuture<typename FunctionObject::result_type> run(FunctionObject functionObject, Type type, const QString &info = QString())
    {
        FunctorTask<FunctionObject> task(type, functionObject, info);
        return QtConcurrent::run(task.pool->pool, task);
    }
    
    void abort();
    
    void print();
    
    static bool debuging;
    
private slots:
    void updateMaxActive();
    
private:
    Concurrent();
    ~Concurrent();
    
    struct CommonInfo {
        QMutex mutex;
        int tasks_active;
        int tasks_maxactive;
        int threads_maxrunning;
        QSet<quintptr> threads;
        CommonInfo() : tasks_active(0), tasks_maxactive(0), threads_maxrunning(0) {}
    };
    
    enum TaskState {
        Wait = 0,
        Started,
        Finished
    };
    
    struct TaskInfo {
        QString info;
        TaskState state;
        quint64 duration;
        explicit TaskInfo(const QString &str) : info(str), state(Wait), duration(0) {}
    };
    
    struct ThreadPool {
        QString name;
        QThreadPool *pool;
        QMutex mutex;
        QSet<quintptr> threads;
        int threads_maxrunning;
        int tasks_active;
        int tasks_maxactive;
        int tasks_wait;
        int tasks_maxwait;
        QList<TaskInfo> tasks;
        
        ThreadPool(const QString &nm, QThreadPool *p = new QThreadPool()) : name(nm), pool(p),
            threads_maxrunning(0), tasks_active(0), tasks_maxactive(0), tasks_wait(0), tasks_maxwait(0) {}
        ~ThreadPool() { if (QThreadPool::globalInstance() != pool) { delete pool; }}
    };
    
    struct TaskCallMarker {
        const QString &info;
        ThreadPool *pool;
        QElapsedTimer timer;
        TaskCallMarker(ThreadPool *p, const QString &i) : info(i), pool(p) {
            Concurrent::instance()->startTask(pool, info);
            timer.start();
        }
        ~TaskCallMarker() {
            Concurrent::instance()->finishTask(pool, info, timer.elapsed());
        }
    };
    
    template <typename T>
    struct BaseTask {
        typedef T result_type;
        QString info;
        ThreadPool *pool;
        BaseTask(Type type, const QString &str) : info(str), 
            pool(Concurrent::instance()->poolByType(type)) {
            
            Concurrent::instance()->addTask(pool, info); 
        }

        virtual ~BaseTask() {}
        
        T operator ()() {
            TaskCallMarker marker(pool, info);
            Q_UNUSED(marker);
            return call();
        }
        
        virtual T call() = 0;
    };
    
    template <typename T, typename Class>
    struct Task : public BaseTask<T> {
        Class *object;
        T (Class::*fn)();
        
        Task(Type type, Class *o, T (Class::*f)(), const QString &info) :
            BaseTask<T>(type, Concurrent::taskInfo(!info.isEmpty() ? info : Q_FUNC_INFO)), object(o), fn(f) {}
        
        T call() { return (object->*fn)(); }
    };
    
    template <typename T, typename Class, typename Param1, typename Arg1>
    struct TaskArg1 : public BaseTask<T> {
        Class *object;
        T (Class::*fn)(Param1);
        Arg1 arg1;
        
        TaskArg1(Type type, Class *o, T (Class::*f)(Param1), Arg1 a1, const QString &info) :
            BaseTask<T>(type, Concurrent::taskInfo(!info.isEmpty() ? info : Q_FUNC_INFO)), object(o), fn(f), arg1(a1) {}
        
        T call() { return (object->*fn)(arg1); }
    };
    
    template <typename T, typename Class>
    struct ConstTask : public BaseTask<T> {
        const Class *object;
        T (Class::*fn) () const;
        
        ConstTask(Type type, const Class *o, T (Class::*f)() const, const QString &info) :
            BaseTask<T>(type, Concurrent::taskInfo(!info.isEmpty() ? info : Q_FUNC_INFO)), object(o), fn(f) {}
        
        T call() { return (object->*fn)(); }
    };
    
    template <typename T, typename Class, typename Param1, typename Arg1>
    struct ConstTaskArg1 : public BaseTask<T> {
        const Class *object;
        T (Class::*fn)(Param1) const;
        Arg1 arg1;
        
        ConstTaskArg1(Type type, const Class *o, T (Class::*f)(Param1) const, Arg1 a1, const QString &info) :
            BaseTask<T>(type, Concurrent::taskInfo(!info.isEmpty() ? info : Q_FUNC_INFO)), object(o), fn(f), arg1(a1) {}
        
        T call() { return (object->*fn)(arg1); }
    };
    
    template <typename Class>
    struct FunctorTask : public BaseTask<void> {
        Class functor;
        
        FunctorTask(Type type, const Class fn, const QString &info) :
            BaseTask<void>(type, Concurrent::taskInfo(!info.isEmpty() ? info : Q_FUNC_INFO)), functor(fn) {}
        
        void call() { functor(); }
    };
    
    ThreadPool* poolByType(const Type t) const;
    QThreadPool* requestPool(const Type t) const;
    
    static QString taskInfo(const QString  &info);
    void addTask(ThreadPool *pool, const QString &info); 
    void startTask(ThreadPool *pool, const QString &info);
    void finishTask(ThreadPool *pool, const QString &info, quint64 duration);
    
    void toStream(QTextStream &stream, const ThreadPool *pool) const;
    void tasksToStream(QTextStream &stream, const QString &title, const QList<TaskInfo> &tasks, TaskState state) const;
    
    typedef QMap<Type, ThreadPool* > Pools;
    
    static int tasks_increment;
    Pools m_pools;
    CommonInfo m_commonInfo;
    QTimer m_timer;
    
};

}

#endif // CONCURRENT_H
