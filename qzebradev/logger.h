#ifndef QZebraDev_LOGGER_H
#define QZebraDev_LOGGER_H

#include <QDebug>
#include <QVector>
#include <QMutex>
#include <QThread>
#include <QStringList>
#include <cassert>
#include <QDateTime>

namespace QZebraDev {

//! Message --------------------------------
class LogMsg 
{
public:
    
    LogMsg(const QString &l, const QString &t)
        : type(l), tag(t), dateTime(QDateTime::currentDateTime()),
          thread(QThread::currentThread()) {}
    
    LogMsg(const QString &l, const QString &t, const QString &m)
        : type(l), tag(t), message(m), dateTime(QDateTime::currentDateTime()),
          thread(QThread::currentThread()) {}
    
    QString type;
    QString tag;
    QString message;
    QDateTime dateTime;
    QThread *thread;
};

//! Layout ---------------------------------
class LogLayout
{
public:
    explicit LogLayout(const QString &format);
    virtual ~LogLayout();

    virtual QString output(const LogMsg &logMsg) const;

private:
    QString m_format;
};

//! Destination ----------------------------
class LogDest
{
    
public:
    explicit LogDest(const LogLayout &l);
    virtual ~LogDest();
    
    virtual void write(const LogMsg &logMsg) = 0;
    LogLayout layout() const;
    
protected:
    LogLayout m_layout;
};


//! Logger ---------------------------------
class Logger
{

public:

    static Logger* instance() {
        if(!s_logger) {
            s_logger = new Logger();
        }
        return s_logger;
    }
    
    enum Level {
        Off     = 0,
        Normal  = 1,
        Debug   = 2,
        Full    = 3
    };

    static const QString ERROR;
    static const QString WARN;
    static const QString INFO;
    static const QString DEBUG;

    void setupDefault();
    
    void setLevel(Level level);
    Level level() const;
    inline bool isLevel(Level level) const { return level <= m_level && level != Off; }
    
    QStringList typeList() const;
    void setType(const QString &type, bool enb);
    inline bool isType(const QString &type) const { return m_types.contains(type); }

    static void setIsCatchQtMsg(bool arg);

    void write(const LogMsg &logMsg);
    
    void addLogDest(LogDest *dest);
    void clearLogDest();
    
private:
    Logger();
    ~Logger();
    static Logger *s_logger;
    
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    static void logMsgHandler(QtMsgType, const QMessageLogContext &, const QString &);
#else
    static void logMsgHandler (enum QtMsgType defType, const char * msg);
#endif
    
    static QString qtMsgTypeToString(enum QtMsgType defType);

    Level m_level;
    QVector<LogDest*> m_destList;
    QSet<QString> m_types;
    QMutex m_mutex;
};

//! Stream ---------------------------------
class LogStream
{
public:
    explicit LogStream(const QString &type, const QString &tag)
        : m_msg(type, tag), m_stream(&m_msg.message) {}
    
    ~LogStream() {
        QT_TRY  {
            Logger::instance()->write(m_msg);
        } QT_CATCH(...) {
            Q_ASSERT(!"exception in logger helper destructor");
            QT_RETHROW;
        }
    }
    
    QDebug& stream() { return m_stream.noquote(); }
    
private:
    LogMsg m_msg;
    QDebug m_stream;
};

}

#endif // QZebraDev_LOGGER_H
