#ifndef GETTHREADPOOLPRIVATE_H
#define GETTHREADPOOLPRIVATE_H

class QThreadPool;
class QThreadPoolPrivate;
class GetThreadPoolPrivate
{
public:
    GetThreadPoolPrivate();
    
    static const QThreadPoolPrivate* pool_private(const QThreadPool *pool);
};

#endif // GETTHREADPOOLPRIVATE_H
