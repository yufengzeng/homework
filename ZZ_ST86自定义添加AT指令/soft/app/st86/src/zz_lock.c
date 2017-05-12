#include "zz_lock.h"

#ifdef __ZZ_LOCK_TYPE_SEM
ZZLock zzCreateLock(void)
{
    ZZLock lock = ZZ_LOCK_NULL;
    int32 ret = 0;
    
    ret = SL_CreateSemaphore(&lock.sem, 1);
    if (ret < 0)
    {
        zzLogE("SL_CreateSemaphore err(%d, %u)", ret, lock.sem.element[0]);
    }
    else
    {
        zzLogI("SL_CreateSemaphore ok(%d, %u)", ret, lock.sem.element[0]);
    }
    
    return lock;
}

void zzLock(ZZLock *lock)
{
    int32 ret = 0;
    
    if (!lock)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return;    
    }
    
    zzLogI("locking(%u)", lock->sem.element[0]);
    ret = SL_TakeSemaphore(lock->sem, 0);
    if (ret < 0)
    {
        zzLogE("lock err(%d, %u)", ret, lock->sem.element[0]);
    }
    else
    {
        zzLogI("lock ok(%d, %u)", ret, lock->sem.element[0]);
    }
    
}

void zzUnlock(ZZLock lock)
{
    int32 ret = 0;
    
    //zzLogI("unlocking(%u)", lock.sem.element[0]);
    ret = SL_GiveSemaphore(lock.sem);
    zzLogI("unlock ok(%d, %u)", ret, lock.sem.element[0]);
}

void zzDeleteLock(ZZLock lock)
{
    int32 ret = 0;
    
    ret = SL_DeleteSemaphore(lock.sem);
    zzLogI("SL_DeleteSemaphore ok(%d, %u)", ret, lock.sem.element[0]);
}


#endif

#ifdef __ZZ_LOCK_TYPE_MUTEX
ZZLock zzCreateLock(void)
{
    ZZLock ret = ZZ_LOCK_NULL;
    
    ret.mutex = SL_CreateMutex();
    if (ret.mutex < 0)
    {
        zzLogE("SL_CreateMutex err(%d)", ret.mutex);
    }
    else
    {
        zzLogI("SL_CreateMutex ok(%d)", ret.mutex);
    }
    return ret;
}

void zzLock(ZZLock *lock)
{
    if (!lock)
    {
        zzLogE(ZZ_ERR_STR_INP);
        return;    
    }
    
    //zzLogI("locking(%d)", lock->mutex);
    lock->user = SL_TakeMutex(lock->mutex);
    if (lock->user < 0)
    {
        zzLogE("lock err(%d, %d)", lock->mutex, lock->user);
    }
    else
    {
        //zzLogP("lock ok(%d, %d)", lock->mutex, lock->user);
    }
    
}

void zzUnlock(ZZLock lock)
{
    //zzLogI("unlocking(%d, %d)", lock.mutex, lock.user);
    SL_GiveMutex(lock.mutex, lock.user);
    //zzLogP("unlock ok(%d, %d)", lock.mutex, lock.user);
}

void zzDeleteLock(ZZLock lock)
{
    SL_DeleteMutex(lock.mutex);
    zzLogI("SL_DeleteMutex ok(%d)", lock.mutex);
}

#endif

