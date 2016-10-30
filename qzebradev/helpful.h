#ifndef HELPFUL_H
#define HELPFUL_H

#include <QString>

namespace QZebraDev {

struct Helpful {
    static QString className(const QString &funcInfo);
    static QString methodName(const QString &funcInfo);
};

}

#endif // HELPFUL_H
