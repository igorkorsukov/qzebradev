#include "gtest/gtest.h"
#include "qzebradev/profiler.h"
#include "qzebradev/profilerlogprinter.h"
#include "qzebradev/logger.h"

#include "overhead.h"

using namespace QZebraDev;

struct Sleep : public QThread { using QThread::msleep; };

class ProfilerTests : public ::testing::Test
{
protected:

    ProfilerTests()
    {
        Logger::instance()->setupDefault();
        Logger::instance()->setLevel(Logger::Debug);
        Profiler::instance()->setup(Profiler::Options(), new ProfilerLogPrinter());
    }
};

struct Example {

    void func() const
    {
        TRACEFUNC; //! NOTE It should be in the beginning
        for (int i = 0; i < 5; ++i) {
            func2();
        }
    }

    QString func2() const
    {
        TRACEFUNC; //! NOTE It should be in the beginning
        QString str;
        for (int i = 0; i < 500; ++i) {
            str += QString::number(i);
        }
        return str;
    }

    void funcWithSteps() const
    {
        BEGIN_STEP_TIME("Example");

        longFunc(10);
        STEP_TIME("Example", "end step 1");

        longFunc(100);
        STEP_TIME("Example", "end step 2");

        longFunc(50);
        STEP_TIME("Example", "end step 3");
    }

    void longFunc(unsigned long ms) const
    {
        TRACEFUNC;
        Sleep::msleep(ms);
    }

    void veryLongFunc()
    {
        TRACEFUNC;
        longFunc(200);
        stackFunc();
    }

    void stackFunc()
    {
        TRACEFUNC;
        longFunc(600);
        longFunc(800);
    }

};

TEST_F(ProfilerTests, Example)
{
    //! Measurement duration of functions

    Example example;

    example.func();

    Profiler::instance()->printThreadsData(); //! NOTE Print profiler data

    /*
    Main thread. Top 150 by sum time (total count: 2)
    Function                                                      Call time           Call count          Sum time
    void Example::func() const                                    0.050 ms            1                   0.050 ms
    QString Example::func2() const                                0.010 ms            5                   0.050 ms


    Other threads. Top 150 by sum time (total count: 0)
    Function                                                      Call time           Call count          Sum time
    */


    //! Measurement duration of steps

    example.funcWithSteps();

    /*
    11:54:38.252 | DEBUG | Profiler                   | main | Example : 0.000/0.000 ms: Begin
    11:54:38.263 | DEBUG | Profiler                   | main | Example : 10.230/10.101 ms: end step 1
    11:54:38.363 | DEBUG | Profiler                   | main | Example : 110.847/100.177 ms: end step 2
    11:54:38.414 | DEBUG | Profiler                   | main | Example : 161.880/50.263 ms: end step 3
    */

    //! Detected long functions

    example.veryLongFunc();

    /*
    11:54:39.636 | INFO  | Profiler                   | 0x0078d650 | Long functions on main thread: //! NOTE Detected during functions execution, output stack
    void Example::veryLongFunc(): 1221 ms
    void Example::stackFunc(): 1021 ms
    11:54:40.016 | INFO  | Profiler                   | main | Long: 1401.590 ms, func: void Example::stackFunc()
    11:54:40.017 | INFO  | Profiler                   | main | Long: 1602.358 ms, func: void Example::veryLongFunc()
    */

}

TEST_F(ProfilerTests, DISABLED_Overhead)
{
    struct Funcs : public Overhead::OverFuncs {
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

    Overhead::OverResult over = Overhead::overheadWithPrint("Profiler", &funcs, 1000000);

    ASSERT_LT(over.overPercent, 110);
}
