#ifndef DEFLOGDEST_H
#define DEFLOGDEST_H

#include "logger.h"
#include <QFile>
#include <QTextStream>

namespace QZebraDev
{

class FileLogDest : public LogDest
{
public:
    FileLogDest(const QString &path, const QString &name, const QString &ext, const LogLayout &l);
    ~FileLogDest();

    void write(const LogMsg &logMsg);

private:
    void rotate();

    QFile m_file;
    QString m_path;
    QString m_name;
    QString m_ext;
    QTextStream m_stream;
    QDate m_rotateDate;
};

class OutputDest : public LogDest
{
public:
    explicit OutputDest(const LogLayout &l);

    void write(const LogMsg &logMsg);
};

class StdOutDest : public LogDest
{
public:
    explicit StdOutDest(const LogLayout &l);

    void write(const LogMsg &logMsg);
};

}

#endif // DEFLOGDEST_H
