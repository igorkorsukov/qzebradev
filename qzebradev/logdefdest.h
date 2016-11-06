#ifndef DEFLOGDEST_H
#define DEFLOGDEST_H

#include "logger.h"
#include <QFile>
#include <QTextStream>

namespace QZebraDev
{

class MemLogDest : public LogDest
{
public:
    MemLogDest(const LogLayout &l);

    QString name() const;
    void write(const LogMsg &logMsg);

    QString content() const;

private:
    QTextStream m_stream;
    QString m_str;
};

class FileLogDest : public LogDest
{
public:
    FileLogDest(const QString &path, const QString &name, const QString &ext, const LogLayout &l);
    ~FileLogDest();

    QString name() const;
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

class ConsoleLogDest : public LogDest
{
public:
    explicit ConsoleLogDest(const LogLayout &l);

    QString name() const;
    void write(const LogMsg &logMsg);
};

}

#endif // DEFLOGDEST_H
