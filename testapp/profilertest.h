#ifndef PROFILERTEST_H
#define PROFILERTEST_H

#include <QString>

class ProfilerTest
{
public:
    ProfilerTest();

    void run();

private:
    QString func();
    void func_pure();
    void func_prof();

    qint64 overTest();
};

#endif // PROFILERTEST_H
