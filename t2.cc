#include <cstdlib>
#include <iostream>
#include "thread.h"
using namespace std;

int threadnumber = 1;
int start = 0;
int stop;

int mutex = 2;
int swapcond = 2;
int mutex2 = 4;

int nolock = 4;
int nocond = 2;


void catch_errors() {
    if (thread_lock(mutex) == 0){
        cout << "error: thread already holding lock\n";
    }
    if (thread_unlock(mutex2) == 0 || thread_unlock(nolock) == 0){
        cout << "error: lock improperly unlocked\n";
    }
    if (thread_wait(nolock, nocond) == 0 || thread_wait(nolock, swapcond) == 0 ){
        cout << "error: lock does not exist\n";
    }
    if (thread_wait(mutex2, nocond) == 0 || thread_wait(mutex2, swapcond) == 0){
        cout << "error: lock is owned by another thread\n";
    }
    if (thread_signal(mutex, nocond) != 0 || thread_signal(nolock, swapcond) != 0 || thread_signal(mutex2, swapcond) != 0){
        cout << "errorSignal: lock or condition is non-existent\n";
    }
    if (thread_broadcast(mutex, nocond) != 0 || thread_broadcast(nolock, swapcond) != 0 || thread_broadcast(mutex2, swapcond) != 0){
        cout << "errorBroadcast: lock or condition is non-existent\n";
    }
}

void testThread(){
    cout << thread_broadcast(mutex, swapcond) << endl;
    cout << thread_unlock(mutex) << endl;
    cout << thread_yield() << endl;
}

void thread1(void *arg) {
    cout << "thread1 initialized\n";
    while (start < stop) {
        cout<< thread_lock(mutex) << endl;
        catch_errors();
        while (threadnumber != 1){
            thread_wait(mutex, swapcond);
        }
        cout << "thread1 gets mutex\n";
        start++;
        cout << "current threadnumber: " << threadnumber << " current round: " << start << endl;
        threadnumber = 2;
        testThread();
    }
    cout << "thread1 finished\n";
}

void thread2(void *arg) {
    cout << "thread2 initialized\n";
    while (start < stop) {
        cout << thread_lock(mutex) << endl;
        while (threadnumber != 2){
            catch_errors();
            thread_wait(mutex, swapcond);
        }
        cout << "thread2 gets mutex\n";
        start++;
        cout << "current threadnumber: " << threadnumber << " current round: " << start << endl;
        threadnumber = 3;
        testThread();
    }
    cout << "thread2 finished\n";
}

void thread3(void *arg) {
    cout << "thread3 initialized\n";
    while (start < stop) {
        cout << thread_lock(mutex) << endl;
        while (threadnumber != 3){
            thread_wait(mutex, swapcond);
        }
        catch_errors();
        cout << "thread3 gets mutex\n";
        start++;
        cout << "current threadnumber: " << threadnumber << " current round: " << start << endl;
        threadnumber = 1;
        testThread();
    }
    cout << "thread3 finished\n";
}

void thread4(void *arg) {
    cout << "thread4 initialized\n";
    cout << thread_lock(mutex2) << endl;
    cout << "thread4 get mutex2\n";
    cout << thread_lock(mutex) << endl;
    while (start < stop - 1) {
        thread_wait(mutex, swapcond);
    }
    cout << "thread4 get mutex\n";
    testThread();
    cout << thread_unlock(mutex2) << endl;
    cout << "thread4 finished\n";
}

void testLock(int (*func)(unsigned int)) {
    if (func(mutex))
        cout << "Thread library not initialized\n";
    else
        cout << "Lock succeeded when should have failed\n";
}

void testWake(int (*func)(unsigned int, unsigned int)) {
    if (func(mutex, swapcond))
        cout << "Thread library not initialized\n";
    else
        cout << "Wake succeeded when should have failed\n";
}

void testYield(int (*func)(void)) {
    if (func())
        cout << "Thread library not initialized\n";
    else
        cout << "Yield succeeded when should have failed\n";
}

void notInit() {
    testYield(&thread_yield);
    testLock(&thread_lock);
    testLock(&thread_unlock);
    testWake(&thread_wait);
    testWake(&thread_signal);
    testWake(&thread_broadcast);
}

void libinit2(void* arg) {
    cout << "Thread library already initialized\n";
}

void makeThreads(void* arg) {
    stop = (int)(intptr_t) arg;
    cout << "number of rounds "<< stop << endl;
    thread_libinit((thread_startfunc_t) libinit2, NULL);
    thread_create((thread_startfunc_t) thread1, NULL);
    thread_create((thread_startfunc_t) thread2, NULL);
    thread_create((thread_startfunc_t) thread3, NULL);
    thread_create((thread_startfunc_t) thread4, NULL);
}

int main() {
    notInit();
    thread_libinit((thread_startfunc_t) makeThreads, (void *) 10);
}