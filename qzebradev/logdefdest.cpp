#include "logdefdest.h"
#include <QDir>
#include <QCoreApplication>

using namespace QZebraDev;

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
OutputDest::OutputDest(const LogLayout &l)
    : LogDest(l)
{
}

#if defined(Q_OS_WIN)
#include "qt_windows.h"
#include <QTextCodec>
#include <QTextStream>

struct OutStream {
    QTextStream cout;
    OutStream() : cout(stdout) {
        QTextCodec *codec = QTextCodec::codecForName("CP1251");
        cout.setCodec(codec);
    }
};

void OutputDest::write(const LogMsg &logMsg)
{
    static OutStream stream;

    stream.cout << m_layout.output(logMsg) << "\r\n";
    stream.cout.flush();
}

StdOutDest::StdOutDest(const LogLayout &l)
    : LogDest(l)
{
}

void StdOutDest::write(const LogMsg &logMsg)
{
    fprintf(stderr, "%s\n", qPrintable(m_layout.output(logMsg)));
    fflush(stderr);
}

#elif defined (Q_OS_ANDROID)
#include <android/log.h>
void OutputDest::write(const LogMsg &logMsg)
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
#include <cstdio>
void OutputDest::write(const LogMsg &logMsg)
{
    fprintf(stderr, "%s\n", qPrintable(m_layout.output(logMsg)));
    fflush(stderr);
}

#endif

