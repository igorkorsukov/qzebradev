#include "benchmark.h"
#include <QElapsedTimer>
#include <QDebug>

qint64 Benchmark::overhead(OverheadFuncs *funcs, int callCount)
{
    QElapsedTimer timer;
    timer.start();

    int measureCount = 50;

    qint64 sumover = 0;
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

        qint64 timepure = endpure - start;
        qint64 timeprof = endprof - endpure;
        qint64 timeover = timeprof - timepure;

        sumover += timeover;
    }

    return sumover / measureCount;
}

qint64 Benchmark::overheadPrint(const QString &info, OverheadFuncs *funcs, int callCount)
{
    qDebug() << info << "- start measure overhead";

    qint64 over = overhead(funcs, callCount);

    qDebug() << info << "overhead:" << over << "ms on" << callCount << "calls";

    return over;
}
