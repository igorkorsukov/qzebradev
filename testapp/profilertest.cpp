#include "profilertest.h"
#include "qzebradev/log.h"
#include "benchmark.h"

using namespace QZebraDev;

ProfilerTest::ProfilerTest()
{

}

void ProfilerTest::run()
{


    benchmark();

    Profiler::instance()->printMain();
}

void ProfilerTest::benchmark() const
{
    struct Funcs : public Benchmark::OverheadFuncs {
        QString func() {
            static int i = 0;
            return QString::number(++i);
        }

        void pureFunc() {
            func();
        }

        void overFunc() {
            TRACEFUNC;
            func();
        }
    };

    Funcs funcs;

    Benchmark::overheadPrint("Profiler", &funcs, 1000000);
}



