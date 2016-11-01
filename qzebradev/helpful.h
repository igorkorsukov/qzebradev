#ifndef QZebraDev_HELPFUL_H
#define QZebraDev_HELPFUL_H

#include <QString>

namespace QZebraDev {

struct Helpful {
    static QString className(const QString &funcInfo);
    static QString methodName(const QString &funcInfo);
};

}

#endif // QZebraDev_HELPFUL_H
