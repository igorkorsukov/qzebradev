#include "profilerlogprinter.h"
#include "log.h"

using namespace QZebraDev;


ProfilerLogPrinter::ProfilerLogPrinter()
{}

ProfilerLogPrinter::~ProfilerLogPrinter()
{}

void ProfilerLogPrinter::printDebug(const QString &str)
{
    LOGD() << str;
}

void ProfilerLogPrinter::printInfo(const QString &str)
{
    LOGI() << str;
}
