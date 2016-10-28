#ifndef HELPFULMACRO_H
#define HELPFULMACRO_H

#include <QCoreApplication>
#include <QString>
#include <QDebug>

struct HelpfulMacro {
    inline static QString className(const QString &funcInfo);
    inline static QString methodName(const QString &funcInfo);
};

//! Format
#define PTRSTR(ptr) QString::fromLatin1("0x%1").arg(reinterpret_cast<quintptr>(ptr), QT_POINTER_SIZE*2, 16, QLatin1Char('0'))
#define QOBJECTINFO(o) QString::fromLatin1("%1(%2)").arg(QString::fromLatin1(o->metaObject()->className())).arg(PTRSTR(o))
#define CLASSNAME(fi) HelpfulMacro::className(fi)
#define FUNCNAME(fi) HelpfulMacro::methodName(fi)

//! Log
#define LOGE() qCritical() << Q_FUNC_INFO
#define LOGW() qWarning() << Q_FUNC_INFO
#define LOGI() qDebug() << Q_FUNC_INFO
#define LOGD() qDebug() << Q_FUNC_INFO

//! Helps
#define ONLY_MAIN_THREAD \
    if (QThread::currentThread() != qApp->thread()) { \
    const QObject *o = dynamic_cast<const QObject*>(this); \
    LOGE() << "There should be only main thread(" << qApp->thread() << "), currentThread:" << QThread::currentThread() << "Object:" << (o ? o : 0); \
    Q_ASSERT(QThread::currentThread() == qApp->thread()); \
    }

//! Profiler
#include "profiler.h"
#define TRACEFUNC_INFO(info) \
    FuncMarker funcMarkerInfo(Profiler::instance()->static_info(info));

#define TRACEFUNC \
    static QString _func_info(Q_FUNC_INFO); \
    FuncMarker funcMarker(_func_info);

#endif // HELPFULMACRO_H
