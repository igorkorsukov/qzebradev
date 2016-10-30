
#include <QCoreApplication>
#include <QDebug>

#include "qzebradev/defaultsetup.h"
#include "logtest.h"
#include "profilertest.h"

int main(int argc, char **argv)
{

    QCoreApplication app(argc, argv);

    qDebug() << "=== QZebraDev Test App ===";

    QZebraDev::DefaultSetup::setup();

//    LogTest log;
//    log.run();

    ProfilerTest prof;
    prof.run();

    return 0;
}
