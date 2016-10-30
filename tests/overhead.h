#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <QtGlobal>

class Overhead
{
public:

    struct Funcs {
        virtual void pureFunc() = 0;
        virtual void overFunc() = 0;
        virtual ~Funcs() {}
    };

    struct Result {
        qint64 pureFuncTime;
        qint64 overFuncTime;
        qint64 overTime;
        double overPercent;
        Result() : pureFuncTime(0), overFuncTime(0), overTime(0),
            overPercent(0.)
        {}
    };

    static Result overhead(Funcs *funcs, int callCount);
    static Result overheadWithPrint(const QString &info, Funcs *funcs, int callCount);
};

#endif // BENCHMARK_H
