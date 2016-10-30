#ifndef LOG_H
#define LOG_H

#include <QCoreApplication>
#include <QString>
#include <QDebug>
#include "helpful.h"
#include "logger.h"
#include "profiler.h"

//! Format
#define PTRSTR(ptr) QString::fromLatin1("0x%1").arg(reinterpret_cast<quintptr>(ptr), QT_POINTER_SIZE*2, 16, QLatin1Char('0'))
#define QOBJECTINFO(o) QString::fromLatin1("%1(%2)").arg(QString::fromLatin1(o->metaObject()->className())).arg(PTRSTR(o))
#define CLASSNAME(fi) QZebraDev::Helpful::className(fi)
#define FUNCNAME(fi) QZebraDev::Helpful::methodName(fi)

//! Log
#define IS_LOGLEVEL(_level)  (_level <= QZebraDev::Logger::instance()->level())
#define LOG(type, tag, level)     if (IS_LOGLEVEL(level)) QZebraDev::LogStream(type, tag).stream()

static const QString LOG_ERROR("ERROR");
static const QString LOG_WARN("WARN");
static const QString LOG_INFO("INFO");
static const QString LOG_DEBUG("DEBUG");

static const QString LOG_TIME_NAME = "TIME";
static const QString LOG_TRACE_NAME = "TRACE";

#ifndef LOG_TAG
#define LOG_TAG CLASSNAME(Q_FUNC_INFO)
#endif

#define LOGE()                          LOG(LOG_ERROR, LOG_TAG, QZebraDev::Logger::Normal) << FUNCNAME(Q_FUNC_INFO)
#define LOGW()                          LOG(LOG_WARN, LOG_TAG, QZebraDev::Logger::Normal) << FUNCNAME(Q_FUNC_INFO)
#define LOGI()                          LOG(LOG_INFO, LOG_TAG, QZebraDev::Logger::Normal) << FUNCNAME(Q_FUNC_INFO)
#define LOGD()                          LOG(LOG_DEBUG, LOG_TAG, QZebraDev::Logger::Debug) << FUNCNAME(Q_FUNC_INFO)

//! Helps
#define ONLY_MAIN_THREAD \
    if (QThread::currentThread() != qApp->thread()) { \
    const QObject *o = dynamic_cast<const QObject*>(this); \
    LOGE() << "There should be only main thread(" << qApp->thread() << "), currentThread:" << QThread::currentThread() << "Object:" << (o ? o : 0); \
    Q_ASSERT(QThread::currentThread() == qApp->thread()); \
    }

//! Profiler
#define TRACEFUNC_INFO(info) \
    QZebraDev::FuncMarker __funcMarkerInfo(Profiler::instance()->static_info(info));

#define TRACEFUNC \
    static QString __func_info(Q_FUNC_INFO); \
    QZebraDev::FuncMarker __funcMarker(__func_info);

#endif // LOG_H
