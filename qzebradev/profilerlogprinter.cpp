#include "profilerlogprinter.h"
#include "log.h"

using namespace QZebraDev;

#define P_LOGI() IF_LOGLEVEL(Logger::Normal) LogStream(QZebraDev::Logger::INFO, QStringLiteral("Profiler")).stream()
#define P_LOGD() IF_LOGLEVEL(Logger::Debug) LogStream(QZebraDev::Logger::DEBUG, QStringLiteral("Profiler")).stream()

ProfilerLogPrinter::ProfilerLogPrinter()
{}

ProfilerLogPrinter::~ProfilerLogPrinter()
{}

void ProfilerLogPrinter::printDebug(const QString &str)
{
    P_LOGD() << str;
}

void ProfilerLogPrinter::printInfo(const QString &str)
{
    P_LOGI() << str;
}
