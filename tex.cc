#include <cstdlib>
#include <iostream>
#include "thread.h"
using namespace std;

int num = 1;
int start = 0;
int stop;
int sleepThreads=0;

int mutex = 1;
int mutex2 = 2;
int mutex3 =3;
int swapcond = 1;

int nolock = 2;
int nocond = 1;

void catch_errors() {
    if (thread_lock(mutex) == 0)
        cout << "Lock: mutex already owned\n";
    else
        cout << "errorLock: unable to get Lock: mutex\n";

    if (thread_unlock(mutex2) == 0)
        cout << "unlocked Lock: mutex2\n";
    else
        cout << "errorUnlock: unable to unlock Lock: mutex2\n";

    if (thread_unlock(nolock) == 0)
        cout << "unlocked Lock: nolock\n";
    else
        cout << "errorUnlock: unable to unlock Lock: nolock\n";

    if (thread_wait(nolock, nocond) == 0)
        cout <<"waiting on Lock: nolock and Condition: nocond\n";
    else
        cout <<"errorWait: unable to wait on Lock: nolock and Condition: nocond\n";

    if (thread_wait(mutex2, nocond) == 0)
        cout <<"waiting on Lock: mutex2, Condition: nocond\n";
    else
        cout <<"errorWait: unable to wait on Lock: mutex2, Condition: nocond\n";

    if (thread_wait(nolock, swapcond) == 0)
        cout <<"waiting on Lock: nolock, Condition: swapcond\n";
    else
        cout <<"errorWait: unable to wait on Lock: nolock, Condition: swapcond\n";

    if (thread_wait(nolock, nocond) == 0)
        cout <<"waiting on Lock: nolock and Condition: nocond\n";
    else
        cout <<"errorWait: unable to wait on Lock: nolock and Condition: nocond\n";

    if (thread_wait(mutex2, swapcond) == 0)
        cout <<"waiting on Lock: mutex2, Condition: swapcond\n";
    else
        cout <<"errorWait: unable to wait on Lock: mutex2, Condition: swapcond\n";

    if (thread_signal(mutex, nocond)!=0)
        cout << "errorSignal: condition non-existent for the Lock: mutex\n";
    else
        cout << "signaling Lock: mutex, Condition: nocond\n";
    if (thread_signal(nolock, swapcond)!=0)
        cout << "errorSignal: condition non-existent for the Lock: nolock\n";
    else
        cout << "signaling Lock: nolock, Condition: swapcond\n";
    if (thread_signal(mutex2, swapcond)!=0)
        cout << "errorSignal: condition non-existent for the Lock: mutex2\n";
    else
        cout << "signaling Lock: mutex2, Condition: swapcond\n";

    if (thread_broadcast(mutex, nocond)!=0)
        cout << "errorBroadcast: condition non-existent for the Lock: mutex\n";
    else
        cout << "broadcasting Lock: mutex, Condition: nocond\n";

    if (thread_broadcast(nolock, swapcond)!=0)
        cout << "errorBroadcast: condition non-existent for the Lock: nolock\n";
    else
        cout << "broadcasting Lock: nolock, Condition: swapcond\n";
        
    if (thread_broadcast(mutex2, swapcond)!=0)
        cout << "errorBroadcast: condition non-existent for the Lock: mutex2\n";
    else
        cout << "broadcasting Lock: mutex2, Condition: swapcond\n";
}

void t1(void *arg) {
    cout << "t1 started\n";
    while (start < stop) {
        thread_lock(mutex);
        sleepThreads++;
        cout << "number of sleeping threads: " << sleepThreads << endl;
        catch_errors();
        while (num != 1){
            thread_wait(mutex, swapcond);
        }
        cout << "t1 acquires lock\n";
        sleepThreads--;
        start++;
        num = 2;

        cout << "number of sleeping threads: " << sleepThreads << endl;
        cout << "current threadnumber: " << num << " current round: " << start << endl;
        
        thread_broadcast(mutex, swapcond);
        thread_unlock(mutex);
        thread_yield();
    }
    cout << "t1 finished\n";
}

void t2(void *arg) {
    cout << "t2 started\n";
    while (start < stop) {
        thread_lock(mutex);
        sleepThreads++;
        cout << "number of sleeping threads: " << sleepThreads << endl;
        while (num != 2){
            catch_errors();
            thread_wait(mutex, swapcond);
        }
        cout << "t2 acquires lock\n";
        sleepThreads--;
        start++;
        num = 3;

        cout << "number of sleeping threads: " << sleepThreads << endl;
        cout << "current threadnumber: " << num << " current round: " << start << endl;
        
        thread_broadcast(mutex, swapcond);
        thread_unlock(mutex);
        thread_yield();
    }
    cout << "t2 finished\n";
}

void t3(void *arg) {
    cout << "t3 started\n";
    while (start < stop) {
        sleepThreads++;
        cout << "number of sleeping threads: " << sleepThreads << endl;
        thread_lock(mutex);
        while (num != 3){
            thread_wait(mutex, swapcond);
        }

        cout << "t3 acquires lock\n";
        sleepThreads--;
        start++;
        num = 1;

        cout << "number of sleeping threads: " << sleepThreads << endl;
        cout << "current threadnumber: " << num << " current round: " << start << endl;
        
        catch_errors();
        thread_broadcast(mutex, swapcond);
        thread_unlock(mutex);
        thread_yield();
    }
    cout << "t3 finished\n";
}

void t4(void *arg) {
    cout << "t4 started\n";

    thread_lock(mutex2);
    cout << "t4 acquires mutex\n";
    thread_lock(mutex);
    cout << "t4 acquires mutex2\n";

    cout << "acquire mutex2 again "<< thread_lock(mutex2)<<"\n";
    
    while (start < stop - 1) {
        thread_wait(mutex, swapcond);
    }

    thread_broadcast(mutex, swapcond);
    thread_unlock(mutex);
    thread_unlock(mutex2);
    cout << "acquire mutex2 again "<< thread_unlock(mutex2)<<"\n";
    thread_yield();

    cout << "t4 finished\n";
}

void testLock(int (*funcptr)(unsigned int)) {
    if (funcptr(mutex))
        cout << "thread lib not initialized\n";
    else
        cout << "function success with uninitialized library\n";
}

void testWake(int (*funcptr)(unsigned int, unsigned int)) {
    if (funcptr(mutex, swapcond))
        cout << "thread lib not initialized\n";
    else
        cout << "function success with uninitialized library\n";
}

void testYield(int (*funcptr)(void)) {
    if (funcptr())
        cout << "thread lib not initialized\n";
    else
        cout << "function success with uninitialized library\n";
}

void not_init() {
    testLock(&thread_lock);
    testLock(&thread_unlock);
    testWake(&thread_wait);
    testWake(&thread_signal);
    testWake(&thread_broadcast);
    testYield(&thread_yield);

}

void libinit2(void* arg) {
    cout << "Initialized threadlib twice!!\n";
}

void makeThreads(void* arg) {
    stop = (int)(intptr_t) arg;
    thread_create((thread_startfunc_t) t4, (void*) "nothing");
    thread_create((thread_startfunc_t) t1, (void*) "nothing");
    thread_create((thread_startfunc_t) t2, (void*) "nothing");
    thread_create((thread_startfunc_t) t3, (void*) "nothing");

    thread_libinit((thread_startfunc_t) libinit2, NULL);
}

int main() {
    not_init();
    thread_libinit((thread_startfunc_t) makeThreads, (void *) 10);
}