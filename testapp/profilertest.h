#ifndef PROFILERTEST_H
#define PROFILERTEST_H

#include <QString>

class ProfilerTest
{
public:
    ProfilerTest();

    void run();

private:
    void benchmark() const;

};

#endif // PROFILERTEST_H
