#include "gtest/gtest.h"
#include "qzebradev/log.h"
#include "qzebradev/logdefdest.h"

#include "overhead.h"

using namespace QZebraDev;


class LoggerTests : public ::testing::Test
{
protected:

    LoggerTests()
    {
        Logger::instance()->setIsCatchQtMsg(false);
    }


};

TEST_F(LoggerTests, Example)
{
    LOGE() << "This is error";
    LOGW() << "This is warning";
    LOGI() << "This is info";
    LOGD() << "This is debug";
}

TEST_F(LoggerTests, Overhead_Noop)
{
    Logger::instance()->clearLogDest();
    Logger::instance()->addLogDest(new NoopLogDest(LogLayout("${time} | ${type} | ${tag} | ${thread} | ${message}"))); //! Like console
    Logger::instance()->addLogDest(new NoopLogDest(LogLayout("${longdate} | ${type} | ${tag} | ${thread} | ${message}"))); //! Like file

    struct Funcs : public Overhead::Funcs {
        QString func() {
            QString str;
            for (int i = 0; i < 100; ++i) {
                str += QString::number(i);
            }
            return str;
        }

        void pureFunc() {
            QString str = func();
            Q_UNUSED(str);
        }

        void overFunc() {
            QString str = func();
            Q_UNUSED(str);
            LOGI() << str;
        }
    };

    Funcs funcs;

    Overhead::Result over = Overhead::overheadWithPrint("Logger", &funcs, 10000);

    ASSERT_LT(over.overPercent, 100);
}

//TEST_F(LoggerTests, Overhead_Console)
//{
//    Logger::instance()->clearLogDest();
//    Logger::instance()->addLogDest(new ConsoleLogDest(LogLayout("${time} | ${type} | ${tag} | ${thread} | ${message}")));



//    Funcs funcs;

//    Overhead::Result over = Overhead::overheadWithPrint("Logger", &funcs, 1000);

//    ASSERT_GT(6, over.overPercent);
//}
