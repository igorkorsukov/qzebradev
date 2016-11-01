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
    }


};

TEST_F(LoggerTests, Example)
{
    Logger::instance()->setupDefault();

    //! Default output to console, catch Qt message

    LOGE() << "This is error";
    LOGW() << "This is warning";
    LOGI() << "This is info";
    LOGD() << "This is debug"; //! NOTE Default not output

    qCritical() << "This is qCritical";
    qWarning() << "This is qWarning";
    qDebug() << "This is qDebug"; //! NOTE Default not output

    /*
    21:26:05.282 | ERROR | LoggerTests_Example_Test   | main | TestBody() This is error
    21:26:05.282 | WARN  | LoggerTests_Example_Test   | main | TestBody() This is warning
    21:26:05.282 | INFO  | LoggerTests_Example_Test   | main | TestBody() This is info
    21:26:05.282 | ERROR | Qt                         | main | This is qCritical
    21:26:05.282 | WARN  | Qt                         | main | This is qWarning
    */

    //! Set tag (default class name)

#undef LOG_TAG
#define LOG_TAG "MYTAG"

    LOGI() << "This is info with tag";
    /*
    21:20:56.452 | INFO  | MYTAG                      | main | TestBody() This is info with tag
    */

    //! Set log level
    Logger::instance()->setLevel(Logger::Debug);

    LOGD() << "This is debug";
    qDebug() << "This is qDebug";

    /*
    21:28:49.410 | DEBUG | MYTAG                      | main | TestBody() This is debug
    21:28:49.411 | DEBUG | Qt                         | main | This is qDebug
    */


    //! Setup logger
    Logger *logger = Logger::instance();

    //! Destination and format
    logger->clearLogDest();
    logger->addLogDest(new ConsoleLogDest(LogLayout("${time} | ${type} | ${tag} | ${thread} | ${message}")));

    /** NOTE Layout have a tags
    "${longdate}" - yyyy-MM-ddThh:mm:ss.zzz
    "${time}" - hh:mm:ss.zzz
    "${type}" - type
    "${tag}" - tag
    "${thread}" - thread, the main thread output as "main" otherwise hex
    "${message}" - message
    "${trimmessage}" - trimmed message
      */

    //! NOTE This creates a file named "apppath/logs/myapp-yyMMdd.log"
    QString logPath = qApp->applicationDirPath() + "/logs";
    logger->addLogDest(new FileLogDest(logPath, "myapp", "log", LogLayout("${longdate} | ${type} | ${tag} | ${thread} | ${message}")));

    //! Level
    logger->setLevel(Logger::Debug);

    //! Catch Qt message
    logger->setIsCatchQtMsg(true);

    //! Custom types
    logger->setType("SQLTRACE", true);

    //! Add to log.h
#define LOGSQLTRACE() IF_LOGLEVEL(QZebraDev::Logger::Debug) LOG("SQLTRACE", LOG_TAG)

    LOGSQLTRACE() << "This sql trace";
/*
21:53:05.601 | SQLTRACE | MYTAG                      | main | TestBody() This sql trace
*/

    //! That type does not output
    logger->setType("SQLTRACE", false); //! NOTE Type must be a debug level

    LOGSQLTRACE() << "This sql trace"; //! NOTE Not output


    //! Custom LogLayout - inherits of the LogLayout and override method "output"
    //! Custom LogDest - inherits of the LogDest and override method "write"
    //! Custom log macro - see log.h
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
    Logger::instance()->setIsCatchQtMsg(false); //! NOTE For output overhead
    Logger::instance()->clearLogDest();
    Logger::instance()->addLogDest(new NoopLogDest(LogLayout("${time} | ${type} | ${tag} | ${thread} | ${message}"))); //! Like console
    Logger::instance()->addLogDest(new NoopLogDest(LogLayout("${longdate} | ${type} | ${tag} | ${thread} | ${message}"))); //! Like file

    OverheadFuncs funcs;

    Overhead::Result over = Overhead::overheadWithPrint("Logger", &funcs, 1000);

    ASSERT_LT(over.overPercent, 25);
}

TEST_F(LoggerTests, Overhead_NoDebug)
{
    Logger::instance()->setLevel(Logger::Normal);
    Logger::instance()->setIsCatchQtMsg(false); //! NOTE For output overhead
    Logger::instance()->clearLogDest();
    Logger::instance()->addLogDest(new NoopLogDest(LogLayout("${time} | ${type} | ${tag} | ${thread} | ${message}"))); //! Like console
    Logger::instance()->addLogDest(new NoopLogDest(LogLayout("${longdate} | ${type} | ${tag} | ${thread} | ${message}"))); //! Like file

    OverheadFuncs funcs;

    Overhead::Result over = Overhead::overheadWithPrint("Logger", &funcs, 100000);

    ASSERT_LT(over.overPercent, 0.5);
}
