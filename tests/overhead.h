#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <QtGlobal>

class Overhead
{
public:

    struct BenchFunc {
        virtual void func() = 0;
        virtual ~BenchFunc() {}
    };

    static qint64 benchmark(BenchFunc *func, int callCount);
    static qint64 benchmarkWithPrint(const QString &info, BenchFunc *func, int callCount);


    struct OverFuncs {
        virtual void pureFunc() = 0;
        virtual void overFunc() = 0;
        virtual ~OverFuncs() {}
    };

    struct OverResult {
        qint64 pureFuncTime;
        qint64 overFuncTime;
        qint64 overTime;
        double overPercent;
        OverResult() : pureFuncTime(0), overFuncTime(0), overTime(0),
            overPercent(0.)
        {}
    };

    static OverResult overhead(OverFuncs *funcs, int callCount);
    static OverResult overheadWithPrint(const QString &info, OverFuncs *funcs, int callCount);
};

#endif // BENCHMARK_H
