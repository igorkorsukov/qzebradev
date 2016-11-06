#ifndef QZebraDev_LOGGER_H
#define QZebraDev_LOGGER_H

#include <QDebug>
#include <QList>
#include <QMutex>
#include <QThread>
#include <QDateTime>

namespace QZebraDev {

//! Message --------------------------------
class LogMsg 
{
public:

    LogMsg() : thread(0) {}
    
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

    struct Pattern {
        QString pattern;
        QString beforeStr;
        int index;
        int count;
        int minWidth;
        Pattern() : index(-1), count(0), minWidth(0) {}
    };

    QString format() const;

    virtual QString output(const LogMsg &logMsg) const;

    virtual QString formatPattern(const LogMsg &logMsg, const Pattern &p) const;
    virtual QString formatDateTime(const QDateTime &dt) const;
    virtual QString formatDate(const QDate &dt) const;
    virtual QString formatTime(const QTime &dt) const;


    static Pattern parcePattern(const QString &format, const QString &pattern);
    static QList<Pattern> patterns(const QString &format);

private:
    QString m_format;
    QList<Pattern> m_patterns;
};

//! Destination ----------------------------
class LogDest
{
    
public:
    explicit LogDest(const LogLayout &l);
    virtual ~LogDest();
    
    virtual QString name() const = 0;
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
    
    QSet<QString> types() const;
    void setTypes(const QSet<QString> &types);
    void setType(const QString &type, bool enb);
    bool isType(const QString &type) const;

    bool isAsseptMsg(const QString &type) const;

    static void setIsCatchQtMsg(bool arg);

    void write(const LogMsg &logMsg);
    
    void addDest(LogDest *dest);
    QList<LogDest *> dests() const;
    void clearDests();
    
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
    QList<LogDest*> m_dests;
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

        int mcount = m_msg.message.count();
        if (mcount > 0 && m_msg.message.at(mcount-1) == ' ') {
            m_msg.message.chop(1); //! QDebug adds at the end of the space
        }

        Logger::instance()->write(m_msg);
    }
    
    QDebug& stream() { return m_stream.noquote(); }
    
private:
    LogMsg m_msg;
    QDebug m_stream;
};

}

#endif // QZebraDev_LOGGER_H
