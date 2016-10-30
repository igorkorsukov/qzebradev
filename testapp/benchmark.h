#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <QtGlobal>

class Benchmark
{
public:

    struct OverheadFuncs {
        virtual void pureFunc() = 0;
        virtual void overFunc() = 0;
        virtual ~OverheadFuncs() {}
    };

    static qint64 overhead(OverheadFuncs *funcs, int callCount);
    static qint64 overheadPrint(const QString &info, OverheadFuncs *funcs, int callCount);
};

#endif // BENCHMARK_H
