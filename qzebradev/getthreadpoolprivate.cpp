#include "getthreadpoolprivate.h"
#include <QThreadPool>

class QFutureInterfaceBase
{
public:
    
    static const QThreadPoolPrivate* pool_private(const QThreadPool *pool) {
        return reinterpret_cast<const QThreadPoolPrivate* >(qGetPtrHelper(pool->d_ptr));
    }
    
};

GetThreadPoolPrivate::GetThreadPoolPrivate()
{
    
}

const QThreadPoolPrivate* GetThreadPoolPrivate::pool_private(const QThreadPool *pool)
{
    return QFutureInterfaceBase::pool_private(pool);
}
