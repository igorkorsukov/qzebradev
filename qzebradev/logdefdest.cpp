#include "logdefdest.h"
#include <QDir>
#include <QCoreApplication>

using namespace QZebraDev;

NoopLogDest::NoopLogDest(const LogLayout &l)
    : LogDest(l)
{

}

void NoopLogDest::write(const LogMsg &logMsg)
{
    QString str = m_layout.output(logMsg);
    Q_UNUSED(str);
}

// FileLogDest
FileLogDest::FileLogDest(const QString &path, const QString &name, const QString &ext, const LogLayout &l)
    : LogDest(l), m_path(path), m_name(name), m_ext(ext), m_stream(&m_file)
{
    rotate();
}

FileLogDest::~FileLogDest()
{
    if (m_file.isOpen())
        m_file.close();
}

void FileLogDest::write(const LogMsg &logMsg)
{
    if (m_rotateDate != logMsg.dateTime.date())
        rotate();

    m_stream << m_layout.output(logMsg) << "\r\n";
    m_stream.flush();
}

void FileLogDest::rotate()
{
    if (m_file.isOpen())
        m_file.close();

    QDir dir(m_path);
    if (!dir.exists() && !dir.mkpath(m_path)) {
        fprintf(stderr, "Debug: FileLogDest can not mkpath %s\n", qPrintable(m_path));
        fflush(stderr);
        return;
    }

    m_rotateDate = QDate::currentDate();
    QString fileName = QString("%1/%2-%3.%4").arg(m_path).arg(m_name).arg(m_rotateDate.toString("yyMMdd")).arg(m_ext);
    m_file.setFileName(fileName);
    if (!m_file.open(QFile::Append)) {
        fprintf(stderr, "Debug: FileLogDest can not open %s\n", qPrintable(fileName));
        fflush(stderr);
        return;
    }
}


// OutputDest
ConsoleLogDest::ConsoleLogDest(const LogLayout &l)
    : LogDest(l)
{
}

#if defined (Q_OS_ANDROID)
#include <android/log.h>
void ConsoleLogDest::write(const LogMsg &logMsg)
{
    static QString INFO("INFO");
    static QString WARN("WARN");
    static QString FATAL("FATAL");

    int TYPE;
    if (logMsg.type == INFO) TYPE = ANDROID_LOG_INFO;
    else if (logMsg.type == WARN ) TYPE = ANDROID_LOG_WARN;
    else if (logMsg.type == FATAL ) TYPE = ANDROID_LOG_FATAL;
    else TYPE = ANDROID_LOG_DEBUG;

    __android_log_print(TYPE, qPrintable(""), qPrintable(m_layout.output(logMsg) + "\r\n"));
}

#else

#include <QTextCodec>
#include <QTextStream>

struct StdOut {
    QTextStream stream;
    StdOut() : stream(stdout) {
        QTextCodec *c = QTextCodec::codecForLocale();
        stream.setCodec(c);
    }
};

void ConsoleLogDest::write(const LogMsg &logMsg)
{
    static StdOut out;

    out.stream << m_layout.output(logMsg) << "\r\n";
    out.stream.flush();
}

#endif

