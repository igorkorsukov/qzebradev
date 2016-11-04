#include "gtest/gtest.h"
#include "qzebradev/profiler.h"
#include "qzebradev/profilerlogprinter.h"
#include "qzebradev/logger.h"

#include <QElapsedTimer>
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

    Profiler::instance()->setup(Profiler::Options(), new ProfilerLogPrinter());

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

struct StepPrinterMock : public Profiler::Printer
{
    void printStep(const QString &_tag, double _beginMs, double _stepMs, const QString &_info)
    {
        tag = _tag;
        beginMs = _beginMs;
        stepMs = _stepMs;
        info = _info;
    }

    QString tag;
    double beginMs;
    double stepMs;
    QString info;
};

int roundMs(double ms)
{
    return static_cast<int>(ms);
}

TEST_F(ProfilerTests, Step)
{
    StepPrinterMock *printer = new StepPrinterMock();
    Profiler::instance()->setup(Profiler::Options(), printer);

    QElapsedTimer btimer;
    QElapsedTimer stimer;

    BEGIN_STEP_TIME("StepTest");
    btimer.start();
    stimer.start();

    EXPECT_EQ(printer->tag.toStdString(), "StepTest");
    EXPECT_EQ(roundMs(printer->beginMs), btimer.elapsed());
    EXPECT_EQ(roundMs(printer->stepMs), stimer.elapsed());
    EXPECT_EQ(printer->info.toStdString(), "Begin");

    Sleep::msleep(2);

    STEP_TIME("StepTest", "end step 1");

    EXPECT_EQ(printer->tag.toStdString(), "StepTest");
    EXPECT_EQ(roundMs(printer->beginMs), btimer.elapsed());
    EXPECT_EQ(roundMs(printer->stepMs), stimer.elapsed());
    EXPECT_EQ(printer->info.toStdString(), "end step 1");

    stimer.restart();

    Sleep::msleep(10);

    STEP_TIME("StepTest", "end step 2");

    EXPECT_EQ(printer->tag.toStdString(), "StepTest");
    EXPECT_EQ(roundMs(printer->beginMs), btimer.elapsed());
    EXPECT_EQ(roundMs(printer->stepMs), stimer.elapsed());
    EXPECT_EQ(printer->info.toStdString(), "end step 2");
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
