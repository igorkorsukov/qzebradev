#include "gtest/gtest.h"
#include "qzebradev/log.h"
#include "qzebradev/logdefdest.h"

#include "overhead.h"

using namespace QZebraDev;


class LoggerTests : public ::testing::Test
{

};

TEST_F(LoggerTests, Example)
{
    Logger::instance()->setupDefault();

    //! Default output to console, catch Qt messages

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
#define LOG_TAG QStringLiteral("MYTAG")

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


    //! --- Setup logger ---
    Logger *logger = Logger::instance();

    //! Destination and format
    logger->clearLogDest();

    //! Console
    logger->addLogDest(new ConsoleLogDest(LogLayout("${time} | ${type} | ${tag} | ${thread} | ${message}")));

    //! File,this creates a file named "apppath/logs/myapp-yyMMdd.log"
    QString logPath = qApp->applicationDirPath() + "/logs";
    logger->addLogDest(new FileLogDest(logPath, "myapp", "log", LogLayout("${datetime} | ${type} | ${tag} | ${thread} | ${message}")));

    /** NOTE Layout have a tags
    "${datetime}"   - yyyy-MM-ddThh:mm:ss.zzz
    "${time}"       - hh:mm:ss.zzz
    "${type}"       - type
    "${tag}"        - tag
    "${thread}"     - thread, the main thread output as "main" otherwise hex
    "${message}"    - message
    "${trimmessage}" - trimmed message
      */


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

TEST_F(LoggerTests, LogLayout_FormatTime)
{
    LogLayout l("");

    QTime time(11, 47, 3, 34);

    EXPECT_EQ(l.formatTime(time), "11:47:03.034");
}

TEST_F(LoggerTests, LogLayout_FormatDate)
{
    LogLayout l("");

    QDate date(2016, 11, 4);

    EXPECT_EQ(l.formatDate(date), "2016-11-04");
}

TEST_F(LoggerTests, LogLayout_FormatDateTime)
{
    LogLayout l("");

    QDateTime dt(QDate(2016, 11, 4), QTime(12, 2, 32, 345));

    EXPECT_EQ(l.formatDateTime(dt), "2016-11-04T12:02:32.345");
}

TEST_F(LoggerTests, LogLayout_ParcePattern)
{
    QString format("| ${type} | ${tag|26} ");

    LogLayout::Pattern p = LogLayout::parcePattern(format, "${type}");
    EXPECT_EQ(p.index, 2);
    EXPECT_EQ(p.count, 7);
    EXPECT_EQ(p.leftJustified, 0);
    EXPECT_EQ(p.beforeStr.toStdString(), "| ");

    p = LogLayout::parcePattern(format, "${tag}");
    EXPECT_EQ(p.index, 12);
    EXPECT_EQ(p.count, 9);
    EXPECT_EQ(p.leftJustified, 26);
    EXPECT_EQ(p.beforeStr.toStdString(), " | ");


    p = LogLayout::parcePattern(format, "${datetime}");
    EXPECT_EQ(p.index, -1);
    EXPECT_EQ(p.count, 0);
    EXPECT_EQ(p.leftJustified, 0);
    EXPECT_EQ(p.beforeStr.toStdString(), "");
}

TEST_F(LoggerTests, LogLayout_Patterns)
{
    QString format("${datetime} | ${type|5} | ${tag|26} | ${thread} | ${message}");

    QList<LogLayout::Pattern> patterns = LogLayout::patterns(format);
    ASSERT_EQ(patterns.count(), 5);
    EXPECT_EQ(patterns.at(0).pattern.toStdString(), "${datetime}");
    EXPECT_EQ(patterns.at(1).pattern.toStdString(), "${type}");
    EXPECT_EQ(patterns.at(2).pattern.toStdString(), "${tag}");
    EXPECT_EQ(patterns.at(3).pattern.toStdString(), "${thread}");
    EXPECT_EQ(patterns.at(4).pattern.toStdString(), "${message}");
}

TEST_F(LoggerTests, LogLayout_FormatOutput)
{
    LogLayout l("${datetime} | ${type|5} | ${tag|26} | ${thread} | ${message}");

    LogMsg msg("WARN", "MyTag", "LogLayout_FormatOutput");
    msg.dateTime = QDateTime(QDate(2016, 11, 4), QTime(12, 2, 32, 345));

    EXPECT_EQ(l.output(msg).toStdString(), "2016-11-04T12:02:32.345 | WARN  | MyTag                      | main | LogLayout_FormatOutput");
}

struct LogLayoutBench : public Overhead::BenchFunc {

    LogLayout l;
    LogMsg msg;

    void func() {
        QString str = l.output(msg);
        Q_UNUSED(str);
    }

    LogLayoutBench(const LogLayout &_l, const LogMsg _m)
        : l(_l), msg(_m) {}

};

TEST_F(LoggerTests, LogLayout_Benchmark)
{

    LogLayoutBench b(LogLayout("${datetime} | ${type|5} | ${tag|26} | ${thread} | ${message}"), LogMsg("WARN", "MyTag", "LogLayout_FormatOutput"));

    Overhead::benchmarkWithPrint("LogLayout", &b, 10000);
}

struct OverheadFuncs : public Overhead::OverFuncs {
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
    Logger::instance()->addLogDest(new NoopLogDest(LogLayout("${datetime} | ${type} | ${tag} | ${thread} | ${message}"))); //! Like file

    OverheadFuncs funcs;

    Overhead::OverResult over = Overhead::overheadWithPrint("Logger", &funcs, 1000);

    ASSERT_LT(over.overPercent, 25);
}

TEST_F(LoggerTests, Overhead_NoDebug)
{
    Logger::instance()->setLevel(Logger::Normal);
    Logger::instance()->setIsCatchQtMsg(false); //! NOTE For output overhead
    Logger::instance()->clearLogDest();
    Logger::instance()->addLogDest(new NoopLogDest(LogLayout("${time} | ${type} | ${tag} | ${thread} | ${message}"))); //! Like console
    Logger::instance()->addLogDest(new NoopLogDest(LogLayout("${datetime} | ${type} | ${tag} | ${thread} | ${message}"))); //! Like file

    OverheadFuncs funcs;

    Overhead::OverResult over = Overhead::overheadWithPrint("Logger", &funcs, 100000);

    ASSERT_LT(over.overPercent, 0.5);
}
