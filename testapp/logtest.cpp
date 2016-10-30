#include "logtest.h"
#include "qzebradev/log.h"
#include "benchmark.h"

using namespace QZebraDev;

LogTest::LogTest()
{

}

void LogTest::run()
{
    example();
    benchmark();
}

void LogTest::example() const
{
    LOGE() << "This is error";
    LOGW() << "This is warning";
    LOGI() << "This is info";
    LOGD() << "This is debug";
}

void LogTest::benchmark() const
{
    Logger::instance()->clearLogDest();
    Logger::instance()->setIsCatchQtMsg(false);

    struct Funcs : public Benchmark::OverheadFuncs {
        QString func() {
            static int i = 0;
            return QString::number(++i);
        }

        void pureFunc() {
            QString str = func();
            Q_UNUSED(str);
        }

        void overFunc() {
            QString str = func();
            Q_UNUSED(str);
            LOGD() << str;
        }
    };

    Funcs funcs;

    Benchmark::overheadPrint("Logger", &funcs, 1000000);
}


