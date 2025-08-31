#pragma once 

#ifdef _WIN32
#include "windows.h"
#endif

namespace sync {

#ifdef _WIN32
typedef CRITICAL_SECTION Mutex;
typedef CONDITION_VARIABLE ConditionVar; 
typedef SEMAPHORE Semaphore;
typedef DWORD TimeSpan; 
#endif

Mutex make_mutex();
void lock(Mutex &m);
bool trylock(Mutex &m);
void unlock(Mutex &m);
void dispose(Mutex &m);

ConditionVar make_condition_var();
void wait(ConditionVar &cond, Mutex &m, TimeSpan ts);
void wake_one(ConditionVar &cond);
void wake_all(ConditionVar &cond);
void dispose(Mutex &m);

Semaphore make_semaphore();
void wait(Semaphore &s);
void post(Semaphore &s);
void dispose(Mutex &m);

TimeSpan from_ms(double milliseconds);
TimeSpan from_s(double seconds);
TimeSpan from_min(double minutes);
TimeSpan from_hours(double hours);
TimeSpan infinite();

double to_ms(TimeSpan &sp);
double to_s(TimeSpan &sp);
double to_min(TimeSpan &sp);
double to_hours(TimeSpan &sp);

// Defs

#ifdef _WIN32

Mutex make_mutex() {
    Mutex m;
    InitializeCriticalSection(&m);
    return m;
}

void lock(Mutex &m) {
    EnterCriticalSection(&m);
}

bool trylock(Mutex &m) {
    return TryEnterCriticalSection(&m);
}

void unlock(Mutex &m) {
    LeaveCriticalSection(&m);
}

void dispose(Mutex &m) {
    DeleteCriticalSection(&m);
}

ConditionVar make_condition_var() {
    ConditionVar cond;
    InitializeConditionVariable(&cond);
    return cond;
}

void wait(ConditionVar &cond, Mutex &m, TimeSpan ts) {
    SleepConditionVariable(&cond, &m, 
}
 
#endif

} // namespace sync
