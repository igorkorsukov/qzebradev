#include "gtest/gtest.h"
#include "qzebradev/log.h"
#include "qzebradev/logdefdest.h"
#include "qzebradev/gtesthelpful.h"

#include "overhead.h"

using namespace QZebraDev;


class LoggerTests : public ::testing::Test
{

};

class LogDestMock: public LogDest {
public:
    LogDestMock() : LogDest(LogLayout("")) {}

    QString name() const { return "LogDestMock"; }
    void write(const LogMsg &_msg) { msgs.append(_msg); }

    QList<LogMsg> msgs;
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
    logger->clearDests();

    //! Console
    logger->addDest(new ConsoleLogDest(LogLayout("${time} | ${type|5} | ${tag|26} | ${thread} | ${message}")));

    //! File,this creates a file named "apppath/logs/myapp-yyMMdd.log"
    QString logPath = qApp->applicationDirPath() + "/logs";
    logger->addDest(new FileLogDest(logPath, "myapp", "log", LogLayout("${datetime} | ${type|5} | ${tag|26} | ${thread} | ${message}")));

    /** NOTE Layout have a tags
    "${datetime}"   - yyyy-MM-ddThh:mm:ss.zzz
    "${time}"       - hh:mm:ss.zzz
    "${type}"       - type
    "${tag}"        - tag
    "${thread}"     - thread, the main thread output as "main" otherwise hex
    "${message}"    - message
    "${trimmessage}" - trimmed message

    |N - min field width
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

TEST_F(LoggerTests, Logger_DefaultSetup)
{
    Logger* logger = Logger::instance();
    logger->setupDefault();

    //! Default dest to console
    QList<LogDest*> dests = logger->dests();
    ASSERT_EQ(dests.count(), 1);

    LogDest* dest = dests.at(0);
    EXPECT_EQ(dest->name(), "ConsoleLogDest");

    //! Format output to console
    EXPECT_EQ_STR(dest->layout().format(), "${time} | ${type|5} | ${tag|26} | ${thread} | ${message}");

    //! Level Normal
    EXPECT_EQ(logger->level(), Logger::Normal);

    //! Types ERROR, WARN, INFO, DEBUG
    QSet<QString> types = logger->types();
    EXPECT_EQ(types.count(), 4);
    EXPECT_TRUE(types.contains("ERROR"));
    EXPECT_TRUE(types.contains("WARN"));
    EXPECT_TRUE(types.contains("INFO"));
    EXPECT_TRUE(types.contains("DEBUG"));

    //! Catch Qt message
    LogDestMock *dest2 = new LogDestMock();
    logger->addDest(dest2);

    qWarning() << "Catch Qt message";

    ASSERT_EQ(dest2->msgs.count(), 1);
    LogMsg dest2Msg = dest2->msgs.at(0);
    EXPECT_EQ_STR(dest2Msg.tag, "Qt");
    EXPECT_EQ_STR(dest2Msg.message, "Catch Qt message");
}

TEST_F(LoggerTests, LOG)
{
    Logger* logger = Logger::instance();
    logger->setupDefault();
    logger->clearDests();
    LogDestMock *dest = new LogDestMock();
    logger->addDest(dest);

#undef LOG_TAG
#define LOG_TAG CLASSNAME(Q_FUNC_INFO) //! NOTE Default log tag, see log.h

    LOGE() << "Error msg";
    ASSERT_EQ(dest->msgs.count(), 1);
    EXPECT_EQ_STR(dest->msgs.at(0).type, "ERROR");
    EXPECT_EQ_STR(dest->msgs.at(0).tag, " LoggerTests_LOG_Test");        //! NOTE LoggerTests_LOG_Test - class name
    EXPECT_EQ_STR(dest->msgs.at(0).message, "TestBody() Error msg ");   //! NOTE TestBody() - function name

    LOGW() << "Warning msg";
    ASSERT_EQ(dest->msgs.count(), 2);
    EXPECT_EQ_STR(dest->msgs.at(1).type, "WARN");
    EXPECT_EQ_STR(dest->msgs.at(1).tag, " LoggerTests_LOG_Test");
    EXPECT_EQ_STR(dest->msgs.at(1).message, "TestBody() Warning msg ");

    LOGI() << "Info msg";
    ASSERT_EQ(dest->msgs.count(), 3);
    EXPECT_EQ_STR(dest->msgs.at(2).type, "INFO");
    EXPECT_EQ_STR(dest->msgs.at(2).tag, " LoggerTests_LOG_Test");
    EXPECT_EQ_STR(dest->msgs.at(2).message, "TestBody() Info msg ");

    LOGD() << "Debug msg";
    ASSERT_EQ(dest->msgs.count(), 3); //! NOTE Not output with log level Normal (default)

    logger->setLevel(Logger::Debug);

    LOGD() << "Debug msg";
    ASSERT_EQ(dest->msgs.count(), 4);
    EXPECT_EQ_STR(dest->msgs.at(3).type, "DEBUG");
    EXPECT_EQ_STR(dest->msgs.at(3).tag, " LoggerTests_LOG_Test");
    EXPECT_EQ_STR(dest->msgs.at(3).message, "TestBody() Debug msg ");
}

TEST_F(LoggerTests, QtDebug)
{
    Logger* logger = Logger::instance();
    logger->setupDefault();
    logger->clearDests();
    LogDestMock *dest = new LogDestMock();
    logger->addDest(dest);

    qCritical() << "Error msg";
    ASSERT_EQ(dest->msgs.count(), 1);
    EXPECT_EQ_STR(dest->msgs.at(0).type, "ERROR");
    EXPECT_EQ_STR(dest->msgs.at(0).tag, "Qt");
    EXPECT_EQ_STR(dest->msgs.at(0).message, "Error msg");

    qWarning() << "Warning msg";
    ASSERT_EQ(dest->msgs.count(), 2);
    EXPECT_EQ_STR(dest->msgs.at(1).type, "WARN");
    EXPECT_EQ_STR(dest->msgs.at(1).tag, "Qt");
    EXPECT_EQ_STR(dest->msgs.at(1).message, "Warning msg");

    bool isInfo = false;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
    isInfo = true;

    qInfo() << "Info msg";
    ASSERT_EQ(dest->msgs.count(), 3);
    EXPECT_EQ_STR(dest->msgs.at(2).type, "INFO");
    EXPECT_EQ_STR(dest->msgs.at(2).tag, "Qt");
    EXPECT_EQ_STR(dest->msgs.at(2).message, "Info msg");
#endif

    qDebug() << "Debug msg";
    int count = isInfo ? 3 : 2;
    ASSERT_EQ(dest->msgs.count(), count); //! NOTE Not output with log level Normal (default)

    logger->setLevel(Logger::Debug);

    qDebug() << "Debug msg";
    count = isInfo ? 4 : 3;
    int index = isInfo ? 3 : 2;
    ASSERT_EQ(dest->msgs.count(), count);
    EXPECT_EQ_STR(dest->msgs.at(index).type, "DEBUG");
    EXPECT_EQ_STR(dest->msgs.at(index).tag, "Qt");
    EXPECT_EQ_STR(dest->msgs.at(index).message, "Debug msg");
}

TEST_F(LoggerTests, Logger_Dest)
{
    LogDestMock *dest1 = new LogDestMock();
    LogDestMock *dest2 = new LogDestMock();
    Logger::instance()->clearDests();
    Logger::instance()->addDest(dest1);
    Logger::instance()->addDest(dest2);

    LOG_STREAM("INFO", "MYTAG") << "TestDestMsg";

    QDateTime dt = QDateTime::currentDateTime();
    QThread *thread = QThread::currentThread();

    ASSERT_EQ(dest1->msgs.count(), 1);
    LogMsg dest1Msg = dest1->msgs.at(0);

    EXPECT_EQ_STR(dest1Msg.type, "INFO");
    EXPECT_EQ_STR(dest1Msg.tag, "MYTAG");
    EXPECT_EQ_STR(dest1Msg.message, "TestDestMsg ");
    EXPECT_EQ(dest1Msg.dateTime, dt);
    EXPECT_EQ(dest1Msg.thread, thread);

    ASSERT_EQ(dest2->msgs.count(), 1);
    LogMsg dest2Msg = dest2->msgs.at(0);

    EXPECT_EQ_STR(dest2Msg.type, "INFO");
    EXPECT_EQ_STR(dest2Msg.tag, "MYTAG");
    EXPECT_EQ_STR(dest2Msg.message, "TestDestMsg ");
    EXPECT_EQ(dest2Msg.dateTime, dt);
    EXPECT_EQ(dest2Msg.thread, thread);
}

TEST_F(LoggerTests, Logger_Level)
{
    /*
    enum Level {
        Off     = 0,
        Normal  = 1,
        Debug   = 2,
        Full    = 3
    };
    */

    Logger* logger = Logger::instance();

    logger->setLevel(Logger::Off);
    EXPECT_FALSE(logger->isLevel(Logger::Normal));
    EXPECT_FALSE(logger->isLevel(Logger::Debug));
    EXPECT_FALSE(logger->isLevel(Logger::Full));

    logger->setLevel(Logger::Normal);
    EXPECT_TRUE(logger->isLevel(Logger::Normal));
    EXPECT_FALSE(logger->isLevel(Logger::Debug));
    EXPECT_FALSE(logger->isLevel(Logger::Full));

    logger->setLevel(Logger::Debug);
    EXPECT_TRUE(logger->isLevel(Logger::Normal));
    EXPECT_TRUE(logger->isLevel(Logger::Debug));
    EXPECT_FALSE(logger->isLevel(Logger::Full));

    logger->setLevel(Logger::Full);
    EXPECT_TRUE(logger->isLevel(Logger::Normal));
    EXPECT_TRUE(logger->isLevel(Logger::Debug));
    EXPECT_TRUE(logger->isLevel(Logger::Full));
}

TEST_F(LoggerTests, Logger_Types_Set)
{
    Logger* logger = Logger::instance();

    QSet<QString> types;
    types << "ERROR" << "WARN" << "INFO" << "DEBUG";

    logger->setTypes(types);

    EXPECT_TRUE(logger->isType("ERROR"));
    EXPECT_FALSE(logger->isType("TRACE"));

    logger->setType("TRACE", true);
    EXPECT_TRUE(logger->isType("TRACE"));

    logger->setType("DEBUG", false);
    EXPECT_FALSE(logger->isType("DEBUG"));
}

TEST_F(LoggerTests, Logger_Types_Assept)
{
    Logger* logger = Logger::instance();

    QSet<QString> types;
    types << "ERROR" << "WARN" << "INFO" << "DEBUG";
    logger->setTypes(types);

    logger->setLevel(Logger::Normal);
    EXPECT_TRUE(logger->isAsseptMsg("ERROR"));

    //! NOTE In normal level, not the type checking, should be level checking before output msg, see log.h
    EXPECT_TRUE(logger->isAsseptMsg("DEBUG"));
    EXPECT_TRUE(logger->isAsseptMsg("TRACE"));


    logger->setLevel(Logger::Debug);
    EXPECT_TRUE(logger->isAsseptMsg("DEBUG"));
    EXPECT_FALSE(logger->isAsseptMsg("TRACE"));

    logger->setType("DEBUG", false);
    logger->setType("TRACE", true);
    EXPECT_FALSE(logger->isAsseptMsg("DEBUG"));
    EXPECT_TRUE(logger->isAsseptMsg("TRACE"));


    logger->setLevel(Logger::Full);
    logger->setType("DEBUG", false);
    logger->setType("TRACE", false);

    EXPECT_TRUE(logger->isAsseptMsg("DEBUG"));
    EXPECT_TRUE(logger->isAsseptMsg("TRACE"));
}

TEST_F(LoggerTests, Logger_CatchQtMsg)
{
    Logger* logger = Logger::instance();
    logger->setupDefault();
    logger->setLevel(Logger::Debug);

    LogDestMock *dest = new LogDestMock();
    logger->clearDests();
    logger->addDest(dest);

    logger->setIsCatchQtMsg(true);

    qDebug() << "TestMsg";

    QDateTime dt = QDateTime::currentDateTime();
    QThread *thread = QThread::currentThread();

    ASSERT_EQ(dest->msgs.count(), 1);
    LogMsg destMsg = dest->msgs.at(0);

    EXPECT_EQ_STR(destMsg.type, "DEBUG");
    EXPECT_EQ_STR(destMsg.tag, "Qt");
    EXPECT_EQ_STR(destMsg.message, "TestMsg");
    EXPECT_EQ(destMsg.dateTime, dt);
    EXPECT_EQ(destMsg.thread, thread);

    logger->setIsCatchQtMsg(false);

    qDebug() << "TestMsg";

    ASSERT_EQ(dest->msgs.count(), 1);
}

TEST_F(LoggerTests, LogLayout_FormatTime)
{
    LogLayout l("");

    QTime time(11, 47, 3, 34);

    EXPECT_EQ_STR(l.formatTime(time), "11:47:03.034");
}

TEST_F(LoggerTests, LogLayout_FormatDate)
{
    LogLayout l("");

    QDate date(2016, 11, 4);

    EXPECT_EQ_STR(l.formatDate(date), "2016-11-04");
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
    EXPECT_EQ(p.minWidth, 0);
    EXPECT_EQ_STR(p.beforeStr, "| ");

    p = LogLayout::parcePattern(format, "${tag}");
    EXPECT_EQ(p.index, 12);
    EXPECT_EQ(p.count, 9);
    EXPECT_EQ(p.minWidth, 26);
    EXPECT_EQ_STR(p.beforeStr, " | ");


    p = LogLayout::parcePattern(format, "${datetime}");
    EXPECT_EQ(p.index, -1);
    EXPECT_EQ(p.count, 0);
    EXPECT_EQ(p.minWidth, 0);
    EXPECT_EQ_STR(p.beforeStr, "");
}

TEST_F(LoggerTests, LogLayout_Patterns)
{
    QString format("${datetime} | ${type|5} | ${tag|26} | ${thread} | ${message}");

    QList<LogLayout::Pattern> patterns = LogLayout::patterns(format);
    ASSERT_EQ(patterns.count(), 5);
    EXPECT_EQ_STR(patterns.at(0).pattern, "${datetime}");
    EXPECT_EQ_STR(patterns.at(1).pattern, "${type}");
    EXPECT_EQ_STR(patterns.at(2).pattern, "${tag}");
    EXPECT_EQ_STR(patterns.at(3).pattern, "${thread}");
    EXPECT_EQ_STR(patterns.at(4).pattern, "${message}");
}

TEST_F(LoggerTests, LogLayout_FormatOutput)
{
    LogLayout l("${datetime} | ${type|5} | ${tag|26} | ${thread} | ${message}");

    LogMsg msg("WARN", "MyTag", "LogLayout_FormatOutput");
    msg.dateTime = QDateTime(QDate(2016, 11, 4), QTime(12, 2, 32, 345));

    EXPECT_EQ_STR(l.output(msg), "2016-11-04T12:02:32.345 | WARN  | MyTag                      | main | LogLayout_FormatOutput");
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
    Logger::instance()->clearDests();
    Logger::instance()->addDest(new MemLogDest(LogLayout("${time} | ${type} | ${tag} | ${thread} | ${message}"))); //! Like console
    Logger::instance()->addDest(new MemLogDest(LogLayout("${datetime} | ${type} | ${tag} | ${thread} | ${message}"))); //! Like file

    OverheadFuncs funcs;

    Overhead::OverResult over = Overhead::overheadWithPrint("Logger", &funcs, 1000);

    ASSERT_LT(over.overPercent, 25);
}

TEST_F(LoggerTests, Overhead_NoDebug)
{
    Logger::instance()->setLevel(Logger::Normal);
    Logger::instance()->setIsCatchQtMsg(false); //! NOTE For output overhead
    Logger::instance()->clearDests();
    Logger::instance()->addDest(new MemLogDest(LogLayout("${time} | ${type} | ${tag} | ${thread} | ${message}"))); //! Like console
    Logger::instance()->addDest(new MemLogDest(LogLayout("${datetime} | ${type} | ${tag} | ${thread} | ${message}"))); //! Like file

    OverheadFuncs funcs;

    Overhead::OverResult over = Overhead::overheadWithPrint("Logger", &funcs, 100000);

    ASSERT_LT(over.overPercent, 0.5);
}
