#include "logger.h"
#include <QCoreApplication>

using namespace QZebraDev;

// Layout ---------------------------------
#define PTRSTR(ptr) QString::fromLatin1("0x%1").arg(reinterpret_cast<quintptr>(ptr), QT_POINTER_SIZE*2, 16, QLatin1Char('0'))

LogLayout::LogLayout(const QString &format)
    : m_format(format)
{
}

LogLayout::~LogLayout()
{
}

static const int LENGTHOFTYPE = 5;
static const int LENGTHOFTAG = 26;

static const QString LONGDATE_PATTERN("${longdate}");
static const QString TIME_PATTERN("${time}");
static const QString TYPE_PATTERN("${type}");
static const QString TAG_PATTERN("${tag}");
static const QString THREAD_PATTERN("${thread}");
static const QString MESSAGE_PATTERN("${message}");
static const QString TRIMMESSAGE_PATTERN("${trimmessage}");

static const QChar HYPEN('-');
static const QChar COLON(':');
static const QChar SPACE(' ');

QString LogLayout::output(const LogMsg &logMsg) const
{
    QString str = m_format;
    QDate date = logMsg.dateTime.date();
    QTime time = logMsg.dateTime.time();

    QString timeStr = QString::number(time.hour()) + COLON +
            QString::number(time.minute()) + COLON +
            QString::number(time.second()) + '.' +
            QString::number(time.msec());

    QString dateStr = QString::number(date.year()) + HYPEN +
            QString::number(date.month()) + HYPEN +
            QString::number(date.day()) + 'T' + timeStr;

    str.replace(LONGDATE_PATTERN, dateStr);
    str.replace(TIME_PATTERN, timeStr);
    str.replace(TYPE_PATTERN, logMsg.type.leftJustified(LENGTHOFTYPE, SPACE));
    str.replace(TAG_PATTERN, logMsg.tag.trimmed().leftJustified(LENGTHOFTAG, SPACE));
    str.replace(THREAD_PATTERN, (qApp && qApp->thread() == logMsg.thread) ? "main" : PTRSTR(logMsg.thread));
    str.replace(MESSAGE_PATTERN, logMsg.message);
    str.replace(TRIMMESSAGE_PATTERN, logMsg.message.simplified().remove(QChar('"')).replace("\\", "\\\\"));

    return str;
}

LogDest::LogDest(const LogLayout &l) : m_layout(l)
{}

LogDest::~LogDest()
{}

LogLayout LogDest::layout() const
{
    return m_layout;
}

#include "logdefdest.h"

// Logger ---------------------------------
Logger *Logger::s_logger = 0;
const QString Logger::ERROR("ERROR");
const QString Logger::WARN("WARN");
const QString Logger::INFO("INFO");
const QString Logger::DEBUG("DEBUG");

Logger::Logger()
    : m_level(Normal)
{
    setupDefault();
}

Logger::~Logger()
{
    Logger::s_logger = 0;
    setIsCatchQtMsg(false);
    clearLogDest();
}

void Logger::setupDefault()
{
    clearLogDest();
    addLogDest(new ConsoleLogDest(LogLayout("${time} | ${type} | ${tag} | ${thread} | ${message}")));

    m_level = Normal;

    m_types.clear();
    m_types << ERROR << WARN << INFO << DEBUG;

    setIsCatchQtMsg(true);
}

void Logger::write(const LogMsg &logMsg)
{
    QMutexLocker locker(&m_mutex);
    if ((m_level == Full || m_level == Normal || isType(logMsg.type))){
        foreach (LogDest *dest, m_destList) {
            dest->write(logMsg);
        }
    }
}

void Logger::addLogDest(LogDest *dest)
{
    Q_ASSERT(dest);
    m_destList.append(dest);
}

void Logger::clearLogDest()
{
    qDeleteAll(m_destList);
    m_destList.clear();
}

void Logger::setLevel(const Level level)
{
    m_level = level;
}

Logger::Level Logger::level() const
{
    return m_level;
}

QStringList Logger::typeList() const
{
    return m_types.toList();
}

void Logger::setType(const QString &type, bool enb)
{
    if (enb) {
        if (!m_types.contains(type)) {
            m_types.insert(type);
        }
    } else {
        m_types.remove(type);
    }
}


#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
void Logger::logMsgHandler(QtMsgType type, const QMessageLogContext &context, const QString &s)
{
    Q_UNUSED(context);
#else
void Logger::logMsgHandler(enum QtMsgType type, const char *s)
{
#endif
    if (type == QtDebugMsg && !Logger::instance()->isLevel(Logger::Debug)) {
        return;
    }

    LogMsg logMsg(qtMsgTypeToString(type), "Qt", QString(s));

    Logger::instance()->write(logMsg);
}

QString Logger::qtMsgTypeToString(enum QtMsgType defType)
{
    switch (defType) {
    case QtDebugMsg: return DEBUG;
    case QtWarningMsg: return WARN;
    case QtCriticalMsg: return ERROR;
    case QtFatalMsg: return ERROR;
    default: return INFO;
    }
}

void Logger::setIsCatchQtMsg(bool arg)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QtMessageHandler h = arg ? logMsgHandler : 0;
    qInstallMessageHandler(h);
#else
    QtMsgHandler h = arg ? logMsgHandler : 0;
    qInstallMsgHandler(h);
#endif
}

