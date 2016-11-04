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
        longFunc(500);
        stackFunc();
    }

    void stackFunc()
    {
        TRACEFUNC;
        longFunc(2000);
        longFunc(2000);
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
    10:15:40.131 | DEBUG | Profiler                   | main | Example : 0/0 ms: Begin
    10:15:40.142 | DEBUG | Profiler                   | main | Example : 10/10 ms: end step 1
    10:15:40.242 | DEBUG | Profiler                   | main | Example : 110/100 ms: end step 2
    10:15:40.293 | DEBUG | Profiler                   | main | Example : 161/50 ms: end step 3
    */

    //! Detected long functions

    example.veryLongFunc();

    /*
    22:48:13.023 | INFO  | Profiler                   | 0x0103d658 | Long functions on main thread: //! NOTE Detected during functions execution
    void Example::veryLongFunc(): 3470 ms
    22:48:14.382 | INFO  | Profiler                   | 0x0103d658 | Long functions on main thread: //! NOTE Detected during functions execution, output stack
    void Example::veryLongFunc(): 4828 ms
    void Example::stackFunc(): 3828 ms
    22:48:14.554 | INFO  | Profiler                   | main | Long: 4000.518 ms, func: void Example::stackFunc()
    22:48:14.554 | INFO  | Profiler                   | main | Long: 5001.242 ms, func: void Example::veryLongFunc()
    */

}

TEST_F(ProfilerTests, DISABLED_Overhead)
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
