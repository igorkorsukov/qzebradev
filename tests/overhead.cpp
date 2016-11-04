#include "overhead.h"
#include <QElapsedTimer>
#include <QDebug>

qint64 Overhead::benchmark(BenchFunc *func, int callCount)
{
    qint64 retNs(0);

    QElapsedTimer timer;
    timer.start();

    int measureCount = 25;

    for (int i = 0; i < measureCount; ++i) {

        qint64 startNs = timer.nsecsElapsed();
        for (int i = 0; i < callCount; ++i) {
            func->func();
        }
        qint64 endNs = timer.nsecsElapsed();

        retNs += (endNs - startNs);
    }

    retNs = retNs / measureCount;

    return retNs;
}

qint64 Overhead::benchmarkWithPrint(const QString &info, BenchFunc *func, int callCount)
{
    qDebug() << info << "- start measure benchmark";

    qint64 retNs = benchmark(func, callCount);
    double retMs = retNs * 0.000001;

    qDebug() << info << "benchmark:" << QString::number(retMs, 'f', 3) << "ms on " << callCount;

    return retNs;
}

Overhead::OverResult Overhead::overhead(OverFuncs *funcs, int callCount)
{
    OverResult result;

    QElapsedTimer timer;
    timer.start();

    int measureCount = 25;

    for (int i = 0; i < measureCount; ++i) {

        qint64 start = timer.elapsed();
        for (int i = 0; i < callCount; ++i) {
            funcs->pureFunc();
        }
        qint64 endpure = timer.elapsed();

        for (int i = 0; i < callCount; ++i) {
            funcs->overFunc();
        }
        qint64 endprof = timer.elapsed();

        result.pureFuncTime += (endpure - start);
        result.overFuncTime += (endprof - endpure);
    }

    result.pureFuncTime = result.pureFuncTime / measureCount;
    result.overFuncTime = result.overFuncTime / measureCount;
    result.overTime = result.overFuncTime - result.pureFuncTime;
    result.overPercent = (static_cast<double>(result.overFuncTime) / static_cast<double>(result.pureFuncTime) - 1) * 100.;

    return result;
}

Overhead::OverResult Overhead::overheadWithPrint(const QString &info, OverFuncs *funcs, int callCount)
{
    qDebug() << info << "- start measure overhead";

    OverResult over = overhead(funcs, callCount);

    qDebug() << info << "pureFuncTime:" << over.pureFuncTime << "ms" << "overFuncTime:" << over.overFuncTime << "ms";
    qDebug() << info << "overhead:" << over.overTime << "ms on" << callCount << "calls OR" << over.overPercent << "%";

    return over;
}
