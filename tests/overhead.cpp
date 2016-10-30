#include "overhead.h"
#include <QElapsedTimer>
#include <QDebug>

Overhead::Result Overhead::overhead(Funcs *funcs, int callCount)
{
    Result result;

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

Overhead::Result Overhead::overheadWithPrint(const QString &info, Funcs *funcs, int callCount)
{
    qDebug() << info << "- start measure overhead";

    Result over = overhead(funcs, callCount);

    qDebug() << info << "pureFuncTime:" << over.pureFuncTime << "ms" << "overFuncTime:" << over.overFuncTime << "ms";
    qDebug() << info << "overhead:" << over.overTime << "ms on" << callCount << "calls OR" << over.overPercent << "%";

    return over;
}
