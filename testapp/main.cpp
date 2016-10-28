
#include <QCoreApplication>
#include <QDebug>

#include "profilertest.h"

int main(int argc, char **argv)
{

    QCoreApplication app(argc, argv);

    qDebug() << "=== QZebraDev Test App ===";

    ProfilerTest prof;
    prof.run();

    return 0;
}
