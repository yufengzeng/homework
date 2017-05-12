#ifndef __ZZ_LOCK_H__
#define __ZZ_LOCK_H__

#include "sl_os.h"
#include "zz_log.h"
#include "zz_error.h"

#define __ZZ_LOCK_TYPE_MUTEX
//#define __ZZ_LOCK_TYPE_SEM

typedef struct 
{
#ifdef __ZZ_LOCK_TYPE_MUTEX
    int32 mutex;
    int32 user;
#endif
#ifdef __ZZ_LOCK_TYPE_SEM
    SL_SEMAPHORE sem;
#endif    
} ZZLock;

#ifdef __ZZ_LOCK_TYPE_MUTEX
#define ZZ_LOCK_NULL    {0, 0}
#endif

#ifdef __ZZ_LOCK_TYPE_SEM
#define ZZ_LOCK_NULL    {{{0}}}
#endif

ZZLock zzCreateLock(void);
void zzLock(ZZLock *lock);
void zzUnlock(ZZLock lock);
void zzDeleteLock(ZZLock lock);

#endif /* __ZZ_LOCK_H__ */

