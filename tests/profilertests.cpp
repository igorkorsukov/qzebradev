#include "gtest/gtest.h"
#include "qzebradev/profiler.h"
#include "qzebradev/profilerlogprinter.h"
#include "qzebradev/logger.h"

#include "overhead.h"

using namespace QZebraDev;

class ProfilerTests : public ::testing::Test
{
protected:

    ProfilerTests()
    {
        Profiler::instance()->setup(Profiler::Options(), new ProfilerLogPrinter());
        Logger::instance()->setLevel(Logger::Debug);
    }
};

TEST_F(ProfilerTests, Example)
{
    struct Example {

        QString func() const
        {
            TRACEFUNC;
            static int i(0);
            return QString::number(++i);
        }
    };

    Example example;

    example.func();

    Profiler::instance()->printThreadsData();
}

TEST_F(ProfilerTests, Overhead)
{
    struct Funcs : public Overhead::Funcs {
        void func() {
            QString str;
            str = "str";
            (void)str;
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

    Overhead::Result over = Overhead::overheadWithPrint("Profiler", &funcs, 1000000);

    ASSERT_LT(over.overPercent, 110);
}
