#ifndef PROFILER_H
#define PROFILER_H

#include <QString>
#include <QVector>
#include <QElapsedTimer>
#include <QMutex>
#include <QHash>
#include <QTextStream>
#include <QThread>
#include <QTimer>

#ifndef BEGIN_STEP_TIME
#define BEGIN_STEP_TIME(tag) if (Profiler::options().stepTimeEnabled) { Profiler::instance()->stepTime(tag, QString("Begin"), true); }
#endif

#ifndef STEP_TIME
#define STEP_TIME(tag, info) if (Profiler::options().stepTimeEnabled) { Profiler::instance()->stepTime(tag, info); }
#endif


#ifndef TRACEFUNC
#define TRACEFUNC \
    static QString __func_info(Q_FUNC_INFO); \
    QZebraDev::FuncMarker __funcMarker(__func_info);
#endif


#ifndef TRACEFUNC_INFO
#define TRACEFUNC_INFO(info) \
    QZebraDev::FuncMarker __funcMarkerInfo(Profiler::instance()->staticInfo(info));
#endif


namespace QZebraDev {

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

    struct Options {

        bool stepTimeEnabled;

        bool funcsTimeEnabled;
        bool funcsTraceEnabled;
        int funcsMaxThreadCount;

        bool longFuncDetectorEnabled;
        int longFuncThreshold;

        int dataTopCount;

        Options() : stepTimeEnabled(true),
            funcsTimeEnabled(true), funcsTraceEnabled(false), funcsMaxThreadCount(100),
            longFuncDetectorEnabled(true), longFuncThreshold(3000),
            dataTopCount(150){}
    };

    struct Data {

        enum Mode {
            All,
            OnlyMain,
            OnlyOther
        };

        struct Func {
            QString func;
            uint callcount;
            double calltime;    // ms
            double sumtime;     // ms
            Func(const QString& f, uint cc, double ct, double st)
                : func(f), callcount(cc), calltime(ct), sumtime(st){}
        };

        struct Thread {
            quintptr thread;
            QList<Func> funcs;
            Thread() : thread(0) {}
        };

        quintptr mainThread;
        QHash<quintptr, Thread> threads;
        Data() : mainThread(0){}
    };

    struct Printer {
        virtual ~Printer();
        virtual void printDebug(const QString &str);
        virtual void printInfo(const QString &str);
        virtual void printStep(const QString &tag, qint64 beginMs, qint64 stepMs, const QString &info);
        virtual void printTrace(const QString& func, double calltimeMs, qint64 callcount, double sumtimeMs);
        virtual void printLongFuncs(const QStringList &funcsStack);
        virtual void printEndLongFunc(const QString &func, double calltimeMs);
        virtual void printData(const Data &data, Data::Mode mode, int maxcount);
        virtual QString formatData(const Data &data, Data::Mode mode, int maxcount) const;
        virtual void funcsToStream(QTextStream &stream, const QString &title, const QList<Data::Func> &funcs, int count) const;
    };

    void setup(const Options &opt = Options(), Printer *printer = 0);

    static const Options& options();
    Printer* printer() const;


    void stepTime(const QString &tag = "App", const QString &info = "", bool isRestart = false);
    
    void beginFunc(const QString &func, const quintptr th);
    void endFunc(const QString &func, const quintptr th);
    
    const QString& staticInfo(const QString &info); //! NOTE Сохраняет строки
    
    void clear();

    Data threadsData(Data::Mode mode = Data::All) const;

    QString threadsDataString(Data::Mode mode = Data::All) const;
    void printThreadsData(Data::Mode mode = Data::All) const;
    
signals:
    void detectorStarted(int ms);
    void detectorStoped();

private slots:
    void th_checkLongFuncs();
    
private:
    Profiler();
    ~Profiler();

    static Profiler *s_profiler;

    friend struct FuncMarker;
    
    struct StepTimer {
        QElapsedTimer beginTime;
        QElapsedTimer stepTime;
        
        qint64 beginMs() const { return beginTime.elapsed(); }
        qint64 stepMs() const { return stepTime.elapsed(); }
        void start() { beginTime.start(); stepTime.start(); }
        void restart() { beginTime.restart(); stepTime.restart(); }
        void nextStep() { stepTime.restart(); }
    };

    struct StepsData {
        QMutex mutex;
        QHash<QString, StepTimer*> timers;
    };
    
    struct FuncTimer {
        const QString& func;
        QElapsedTimer timer;
        uint callcount;
        double calltimeMs;
        double sumtimeMs;
        explicit FuncTimer(const QString &f) : func(f), callcount(0), calltimeMs(0), sumtimeMs(0) {}
    };

    typedef QHash<const QString*, FuncTimer* > FuncTimers;
    struct FuncsData {
        QMutex mutex;
        QVector<quintptr> threads;
        QVector<FuncTimers> timers;
        QList<QString> staticInfo;

        int threadIndex(quintptr th) const;
        int addThread(quintptr th);
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

    static Options m_options;
    Printer *m_printer;

    StepsData m_steps;
    FuncsData m_funcs;
    
    mutable LongFuncDetector m_detector;
};

struct FuncMarker
{
    explicit FuncMarker(const QString &fn) : func(fn), th(0)
    {
        if (Profiler::m_options.funcsTimeEnabled) {
            th = reinterpret_cast<quintptr>(QThread::currentThread());
            Profiler::instance()->beginFunc(func, th);
        }
    }

    ~FuncMarker()
    {
        if (Profiler::m_options.funcsTimeEnabled) {
            Profiler::instance()->endFunc(func, th);
        }
    }

    const QString &func;
    quintptr th;
};

}

#endif // PROFILER_H
