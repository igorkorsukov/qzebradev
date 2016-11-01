#ifndef QZebraDev_LOG_H
#define QZebraDev_LOG_H

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

#ifndef LOG_TAG
#define LOG_TAG CLASSNAME(Q_FUNC_INFO)
#endif

#define IF_LOGLEVEL(level)  if(QZebraDev::Logger::instance()->isLevel(level))

#define LOG(type, tag)  QZebraDev::LogStream(type, tag).stream() << FUNCNAME(Q_FUNC_INFO)

#define LOGE()      IF_LOGLEVEL(QZebraDev::Logger::Normal) LOG(QZebraDev::Logger::ERROR, LOG_TAG)
#define LOGW()      IF_LOGLEVEL(QZebraDev::Logger::Normal) LOG(QZebraDev::Logger::WARN, LOG_TAG)
#define LOGI()      IF_LOGLEVEL(QZebraDev::Logger::Normal) LOG(QZebraDev::Logger::INFO, LOG_TAG)
#define LOGD()      IF_LOGLEVEL(QZebraDev::Logger::Debug) LOG(QZebraDev::Logger::DEBUG, LOG_TAG)

//! Helps
#define DEPRECATED LOGD() << "This function deprecated!!";
#define DEPRECATED_USE(use) LOGD() << "This function deprecated!! Use:" << use;
#define NOT_IMPLEMENTED LOGW() << "Not implemented!!";
#define NOT_IMPL_RETURN NOT_IMPLEMENTED return
#define NOT_SUPPORTED LOGW() << "Not supported!!";
#define NOT_SUPPORTED_USE(use) LOGW() << "Not supported!! Use:" << use;

#define ONLY_MAIN_THREAD \
    if (QThread::currentThread() != qApp->thread()) { \
    const QObject *o = dynamic_cast<const QObject*>(this); \
    LOGE() << "There should be only main thread(" << qApp->thread() << "), currentThread:" << QThread::currentThread() << "Object:" << (o ? o : 0); \
    Q_ASSERT(QThread::currentThread() == qApp->thread()); \
    }

#define IF_ASSERT(cond) Q_ASSERT(cond); if(!(cond)) { LOGE() << "\"ASSERT FAILED!\":" << #cond << __FILE__ << __LINE__; } if(!(cond))
#define IF_ASSERT_X(cond, info) if(!(cond)) { LOGE() << "\"ASSERT FAILED!\":" << #cond << info << __FILE__ << __LINE__; } Q_ASSERT(cond); if(!(cond))
#define IF_ASSERT_NOOP(cond) Q_ASSERT(cond); if(!(cond)) { LOGE() << "\"ASSERT FAILED!\":" << #cond << __FILE__ << __LINE__; }
#define IF_ASSERT_NOOP_X(cond, info) if(!(cond)) { LOGE() << "\"ASSERT FAILED!\":" << #cond << info << __FILE__ << __LINE__; } Q_ASSERT(cond);
#define CHECK_PARENT(obj) if (obj) { IF_ASSERT(obj->parent()) { LOGE() << "There should be set parent"; } }


#endif // QZebraDev_LOG_H
