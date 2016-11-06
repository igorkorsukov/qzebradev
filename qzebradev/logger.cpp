#include "logger.h"
#include <QCoreApplication>

#include "logdefdest.h"

using namespace QZebraDev;

// Layout ---------------------------------

static const QString DATETIME_PATTERN("${datetime}");
static const QString TIME_PATTERN("${time}");
static const QString TYPE_PATTERN("${type}");
static const QString TAG_PATTERN("${tag}");
static const QString THREAD_PATTERN("${thread}");
static const QString MESSAGE_PATTERN("${message}");
static const QString TRIMMESSAGE_PATTERN("${trimmessage}");

static const QChar ZERO('0');
static const QChar COLON(':');
static const QChar DOT('.');
static const QChar HYPEN('-');
static const QChar T('T');
static const QChar SPACE(' ');

static const QString MAIN("main");
#define PTRSTR(ptr) QString::fromLatin1("0x%1").arg(reinterpret_cast<quintptr>(ptr), QT_POINTER_SIZE*2, 16, QLatin1Char('0'))

LogLayout::LogLayout(const QString &format)
    : m_format(format)
{
    m_patterns = patterns(format);
}

LogLayout::~LogLayout()
{
}

struct IsLessByIndex {
    bool operator () (const LogLayout::Pattern &f, const LogLayout::Pattern &s)
    {
        return f.index < s.index;
    }
};

QList<LogLayout::Pattern> LogLayout::patterns(const QString &format)
{
    QStringList ps;
    ps << DATETIME_PATTERN << TIME_PATTERN << TYPE_PATTERN
       << TAG_PATTERN << THREAD_PATTERN << MESSAGE_PATTERN
       << TRIMMESSAGE_PATTERN;

    QList<LogLayout::Pattern> patterns;
    foreach (const QString &pstr, ps) {
        Pattern p = parcePattern(format, pstr);
        if (p.index > -1) {
            patterns << p;
        }
    }

    std::sort(patterns.begin(), patterns.end(), IsLessByIndex());

    return patterns;
}

LogLayout::Pattern LogLayout::parcePattern(const QString &format, const QString &pattern)
{
    Pattern p;
    p.pattern = pattern;
    QString beginPattern(pattern.left(pattern.count() - 1));
    int beginPatternIndex = format.indexOf(beginPattern);
    if (beginPatternIndex > -1) {
        p.index = beginPatternIndex;
        int last = beginPatternIndex + beginPattern.count();

        int filterIndex = -1;
        int endPatternIndex = -1;
        while (1) {

            if (!(last < format.count())) {
                break;
            }

            QChar c = format.at(last);
            if (c == '|') {

                filterIndex = last;

            } else if (c == '}') {

                endPatternIndex = last;
                break;
            }

            ++last;
        }

        if (filterIndex > -1) {
            QString filter = format.mid(filterIndex + 1, endPatternIndex - filterIndex - 1);
            p.minWidth = filter.toInt();
        }

        p.count = endPatternIndex - beginPatternIndex + 1;

        int beforeEndPatternIndex = 0;
        int beforeBeginPatterIndex = format.lastIndexOf("${", p.index - 1);
        if (beforeBeginPatterIndex > -1) {
            beforeEndPatternIndex = format.indexOf('}', beforeBeginPatterIndex) + 1;
        }

        p.beforeStr = format.mid(beforeEndPatternIndex, p.index - beforeEndPatternIndex);
    }

    return p;
}

QString LogLayout::output(const LogMsg &logMsg) const
{
    QString str;
    str.reserve(100);
    foreach (const Pattern &p, m_patterns) {
        str.append(p.beforeStr).append(formatPattern(logMsg, p));
    }

    return str;
}

QString LogLayout::formatPattern(const LogMsg &logMsg, const Pattern &p) const
{
    if (DATETIME_PATTERN == p.pattern) {

        return formatDateTime(logMsg.dateTime).leftJustified(p.minWidth, SPACE);

    } else if (TIME_PATTERN == p.pattern) {

        return formatTime(logMsg.dateTime.time()).leftJustified(p.minWidth, SPACE);

    } else if (TYPE_PATTERN == p.pattern) {

        return logMsg.type.leftJustified(p.minWidth, SPACE);

    } else if (TAG_PATTERN == p.pattern) {

        return logMsg.tag.leftJustified(p.minWidth, SPACE);

    } else if (THREAD_PATTERN == p.pattern) {

        return ((qApp && qApp->thread() == logMsg.thread) ? MAIN : PTRSTR(logMsg.thread)).leftJustified(p.minWidth, SPACE);

    } else if (MESSAGE_PATTERN == p.pattern) {

        return logMsg.message.leftJustified(p.minWidth, SPACE);

    } else if (TRIMMESSAGE_PATTERN == p.pattern) {

        return logMsg.message.simplified().remove(QChar('"')).replace("\\", "\\\\").leftJustified(p.minWidth, SPACE);
    }

    return QString();
}

QString LogLayout::formatDateTime(const QDateTime &dt) const
{
    QString str;
    str.reserve(22);
    str
            .append(formatDate(dt.date()))
            .append(T)
            .append(formatTime(dt.time()));

    return str;
}

QString LogLayout::formatDate(const QDate &d) const
{
    QString str;
    str.reserve(10);

    str.append(QString::number(d.year())).append(HYPEN);

    if (d.month() < 10) {
        str.append(ZERO);
    }
    str.append(QString::number(d.month())).append(HYPEN);

    if (d.day() < 10) {
        str.append(ZERO);
    }
    str.append(QString::number(d.day()));

    return str;
}

QString LogLayout::formatTime(const QTime &t) const
{
    QString str;
    str.reserve(12);

    if (t.hour() < 10) {
        str.append(ZERO);
    }
    str.append(QString::number(t.hour())).append(COLON);

    if (t.minute() < 10) {
        str.append(ZERO);
    }
    str.append(QString::number(t.minute())).append(COLON);

    if (t.second() < 10) {
        str.append(ZERO);
    }
    str.append(QString::number(t.second())).append(DOT);

    if (t.msec() < 100) {
        str.append(ZERO);
    }

    if (t.msec() < 10) {
        str.append(ZERO);
    }
    str.append(QString::number(t.msec()));

    return str;
}

QString LogLayout::format() const
{
    return m_format;
}


// LogDest ---------------------------------

LogDest::LogDest(const LogLayout &l) : m_layout(l)
{}

LogDest::~LogDest()
{}

LogLayout LogDest::layout() const
{
    return m_layout;
}

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
    clearDests();
}

void Logger::setupDefault()
{
    clearDests();
    addDest(new ConsoleLogDest(LogLayout("${time} | ${type|5} | ${tag|26} | ${thread} | ${message}")));

    m_level = Normal;

    m_types.clear();
    m_types << ERROR << WARN << INFO << DEBUG;

    setIsCatchQtMsg(true);
}

void Logger::write(const LogMsg &logMsg)
{
    QMutexLocker locker(&m_mutex);
    if (isAsseptMsg(logMsg.type)) {
        foreach (LogDest *dest, m_dests) {
            dest->write(logMsg);
        }
    }
}

bool Logger::isAsseptMsg(const QString &type) const
{
    return m_level == Full || m_level == Normal || isType(type);
}

bool Logger::isType(const QString &type) const
{
    return m_types.contains(type);
}

void Logger::addDest(LogDest *dest)
{
    Q_ASSERT(dest);
    m_dests.append(dest);
}

QList<LogDest*> Logger::dests() const
{
    return m_dests;
}

void Logger::clearDests()
{
    qDeleteAll(m_dests);
    m_dests.clear();
}

void Logger::setLevel(const Level level)
{
    m_level = level;
}

Logger::Level Logger::level() const
{
    return m_level;
}

QSet<QString> Logger::types() const
{
    return m_types;
}

void Logger::setTypes(const QSet<QString> &types)
{
    m_types = types;
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

