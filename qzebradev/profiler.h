#ifndef PROFILER_H
#define PROFILER_H

#include <QString>
#include <QElapsedTimer>
#include <QMutex>
#include <QHash>
#include <QTextStream>
#include <QThread>
#include <QVector>
#include <QDateTime>
#include <QTimer>

#define THREAD_MAX_COUNT 100

class Profiler: public QObject
{
    Q_OBJECT

public:
    static Profiler* instance() {
        if(!s_profiler) {
            s_profiler = new Profiler();
        }
        return s_profiler;
    }

    void stepTime(const QString &tag = "App", const QString &info = "", bool isRestart = false);
    
    void beginFunc(const QString &func, const quintptr th);
    void endFunc(const QString &func, const quintptr th);
    
    const QString& static_info(const QString &info); //! NOTE Сохраняет строки
    
    void clear();

    QString mainString();
    QString threadsString();

    void printMain();
    void printThreads();
    
    typedef QList<QVariantMap> Data;
    Data mainData() const;
    
    static bool enabled;
    static bool trace;
    static bool longFuncDetectorEnabled; //! NOTE not thread safe - можно использовать только из главного потока
    
private slots:
    void th_checkLongFuncs();
    
private:
    Profiler();
    ~Profiler();

    static Profiler *s_profiler;
    
    struct StepTimer {
        QElapsedTimer beginTime;
        QElapsedTimer stepTime;
        
        qint64 beginMs() const { return beginTime.elapsed(); }
        qint64 stepMs() const { return stepTime.elapsed(); }
        void start() { beginTime.start(); stepTime.start(); }
        void restart() { beginTime.restart(); stepTime.restart(); }
        void nextStep() { stepTime.restart(); }
    };
    
    struct FuncTimer {
        const QString& func;
        QElapsedTimer timer;
        uint callcount;
        qint64 calltime;
        double sumtime;
        explicit FuncTimer(const QString &f) : func(f), callcount(0), calltime(0), sumtime(0) {}
    };

    struct LongFuncDetector {
        QMutex mutex;
        QThread thread;
        QTimer timer;
        bool enabled;
        bool needLock;
        bool locked;

        LongFuncDetector() : mutex(QMutex::Recursive), enabled(true), needLock(false), locked(false) {}

        void lockIfNeed(int index);
        void unlockIfNeed(int index);
    };
    
    static bool isLessBySum(const Profiler::FuncTimer* f, const Profiler::FuncTimer* s);
    
    QString prepare(const QString &title, QList<FuncTimer*> &list) const;
    void toStream(const QString &title, QTextStream &stream, const QList<FuncTimer*> &list, int count) const;

    int threadIndex(quintptr th) const;
    int threadAdd(quintptr th);

    QMutex m_stepMutex;
    QHash<QString, StepTimer*> m_stepTimers;
    
    typedef QHash<const QString*, FuncTimer* > FuncTimers;
    
    QMutex m_funcMutex;
    FuncTimers m_funcTimers[THREAD_MAX_COUNT];
    quintptr m_funcThreads[THREAD_MAX_COUNT];
    
    QList<QString> m_static_info;
    QString m_dummy;
    
    mutable LongFuncDetector m_detector;
};

struct FuncMarker
{
    explicit FuncMarker(const QString &fn) : func(fn), th(0)
    {
        if (Profiler::enabled) {
            th = reinterpret_cast<quintptr>(QThread::currentThread());
            Profiler::instance()->beginFunc(func, th);
        }
    }
    
    ~FuncMarker()
    {
        if (Profiler::enabled) {
            Profiler::instance()->endFunc(func, th);
        }
    }
    
    const QString &func;
    quintptr th;
};

#endif // PROFILER_H
