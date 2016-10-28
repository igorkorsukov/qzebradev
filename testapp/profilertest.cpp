#include "profilertest.h"
#include "qzebradev/helpfulmacro.h"

#include <QDebug>
#include <QElapsedTimer>

ProfilerTest::ProfilerTest()
{

}

void ProfilerTest::run()
{
    qint64 sum = 0;
    for (int i = 0; i < 10; ++i) {
        sum += overTest();
    }

    qDebug() << "avgover:" << sum / 10 << "ms";
}

qint64 ProfilerTest::overTest()
{
    int count = 1000000;

    QElapsedTimer timer;
    timer.start();
    qint64 start = timer.elapsed();
    for (int i = 0; i < count; ++i) {
        func_pure();
    }
    qint64 endpure = timer.elapsed();

    for (int i = 0; i < count; ++i) {
        func_prof();
    }
    qint64 endprof = timer.elapsed();

    qint64 timepure = endpure - start;
    qint64 timeprof = endprof - endpure;
    qint64 timeover = timeprof - timepure;

    qDebug() << "timepure:" << timepure << "timeprof:" << timeprof << "timeover:" << timeover << "ms";

    return timeover;
}

QString ProfilerTest::func()
{
    static int i = 0;
    QString str = QString("%1").arg(++i);
    return str;
}

void ProfilerTest::func_pure()
{
    func();
}

void ProfilerTest::func_prof()
{
    TRACEFUNC;
    func();
}

