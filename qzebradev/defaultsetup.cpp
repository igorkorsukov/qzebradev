#include "defaultsetup.h"
#include "profiler.h"
#include "profilerlogprinter.h"

using namespace QZebraDev;

void DefaultSetup::setup()
{
    Profiler::instance()->setPrinter(new ProfilerLogPrinter());
}
