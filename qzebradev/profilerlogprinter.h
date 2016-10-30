#ifndef PROFILERLOGPRINTER_H
#define PROFILERLOGPRINTER_H

#include "profiler.h"

namespace QZebraDev {

class ProfilerLogPrinter: public QZebraDev::Profiler::Printer
{
public:
    ProfilerLogPrinter();
    ~ProfilerLogPrinter();

    void printDebug(const QString &str);
    void printInfo(const QString &str);
};

}

#endif // PROFILERLOGPRINTER_H
