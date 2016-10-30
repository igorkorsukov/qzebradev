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

struct OverheadFuncs : public Overhead::Funcs {
    QString func() {
        QString str;
        for (int i = 0; i < 50; ++i) {
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
        LOGD() << str;
    }
};

TEST_F(LoggerTests, Overhead_Noop)
{
    Logger::instance()->setLevel(Logger::Debug);
    Logger::instance()->clearLogDest();
    Logger::instance()->addLogDest(new NoopLogDest(LogLayout("${time} | ${type} | ${tag} | ${thread} | ${message}"))); //! Like console
    Logger::instance()->addLogDest(new NoopLogDest(LogLayout("${longdate} | ${type} | ${tag} | ${thread} | ${message}"))); //! Like file

    OverheadFuncs funcs;

    Overhead::Result over = Overhead::overheadWithPrint("Logger", &funcs, 100000);

    ASSERT_LT(over.overPercent, 25);
}

TEST_F(LoggerTests, Overhead_Off)
{
    Logger::instance()->setLevel(Logger::Normal);
    Logger::instance()->clearLogDest();
    Logger::instance()->addLogDest(new NoopLogDest(LogLayout("${time} | ${type} | ${tag} | ${thread} | ${message}"))); //! Like console
    Logger::instance()->addLogDest(new NoopLogDest(LogLayout("${longdate} | ${type} | ${tag} | ${thread} | ${message}"))); //! Like file

    OverheadFuncs funcs;

    Overhead::Result over = Overhead::overheadWithPrint("Logger", &funcs, 100000);

    ASSERT_LT(over.overPercent, 0.5);
}
