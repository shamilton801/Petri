#pragma once 

#ifdef _WIN32
#include "windows.h"
#endif

namespace sync {

#ifdef _WIN32
typedef CRITICAL_SECTION Mutex;
typedef CONDITION_VARIABLE ConditionVar; 
typedef HANDLE Semaphore;
typedef HANDLE Thread;
typedef DWORD TimeSpan; 
typedef DWORD WINAPI (*ThreadFunc)(_In_ LPVOID lpParameter);
typedef LPVOID ThreadArg
const TimeSpan infinite_ts = INFINITE;
#endif

Thread make_thread(ThreadFunc t);
void join(Thread t);

Mutex make_mutex();
void lock(Mutex &m);
bool trylock(Mutex &m);
void unlock(Mutex &m);
void dispose(Mutex &m);

ConditionVar make_condition_var();
void wait(ConditionVar &c, Mutex &m, TimeSpan ts);
void wake_one(ConditionVar &c);
void wake_all(ConditionVar &c);
void dispose(ConditionVar &c);

Semaphore make_semaphore(int initial, int max);
void wait(Semaphore &s);
void post(Semaphore &s);
void dispose(Semaphore &s);

TimeSpan from_ms(double milliseconds);
TimeSpan from_s(double seconds);
TimeSpan from_min(double minutes);
TimeSpan from_hours(double hours);

double to_ms(TimeSpan &sp);
double to_s(TimeSpan &sp);
double to_min(TimeSpan &sp);
double to_hours(TimeSpan &sp);

#ifdef _WIN32

Thread make_thread(ThreadFunc f, ThreadArg a) {
    DWORD tid;
    return CreateThread(NULL, 0, t, a, 0, &tid);
}

void join(Thread t) {
    WaitForSingleObject(t, infinite_ts);
}

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
    ConditionVar c;
    InitializeConditionVariable(&c);
    return c;
}

void wait(ConditionVar &c, Mutex &m, TimeSpan ts) {
    SleepConditionVariable(&c, &m, ts);
}

void wake_one(ConditionVar &c) {
    WakeConditionVariable(&c);
}

void wake_all(ConditionVar &c) {
    WakeAllConditionVariable(&c);
}

void dispose(ConditionVar &c) {
    return; // Windows doesn't have a delete condition variable func
}

Semaphore make_semaphore(int initial, int max) {
    return CreateSemaphoreA(NULL, (long)initial, (long)max, NULL);
}

void wait(Semaphore &s) {
    WaitForSingleObject(s, infinite_ts);
}

void post(Semaphore &s) {
    ReleaseSemaphore(s);
}

void dispose(Semaphore &s) {
    CloseHandle(s);
}

TimeSpan from_ms(double milliseconds) {
    return static_cast<TimeSpan>(milliseconds);
}

TimeSpan from_s(double seconds) {
    return static_cast<TimeSpan>(seconds*1000.0);
}

TimeSpan from_min(double minutes) {
    return static_cast<TimeSpan>(minutes*60.0*1000.0);
}

TimeSpan from_hours(double hours) {
    return static_cast<TimeSpan>(hours*60.0*60.0*1000.0);
}

double to_ms(TimeSpan &sp) {
    return static_cast<double>(sp);
}

double to_s(TimeSpan &sp) {
    return static_cast<double>(sp)/1000.0;
}

double to_min(TimeSpan &sp) {
    return static_cast<double>(sp)/(1000.0*60.0);
}

double to_hours(TimeSpan &sp) {
    return static_cast<double>(sp)/(1000.0*60.0*60.0);
}
 
#endif

} // namespace sync
