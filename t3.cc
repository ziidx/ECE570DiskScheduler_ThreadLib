#include <cstdlib>
#include <iostream>
#include "thread.h"
using namespace std;

int g = 0;
int blockThreads = 0;
int mutex = 1;
int cond = 1;

void loop(void *a) {
    char *id;
    int i;

    id = (char *) a;
    cout << "loop called with id " << (char *) id << endl;

    for (i = 0; i < 5; i++, g++) {
        cout << id << ":\t" << i << "\t" << g << endl;
        if (thread_yield()) {
            cout << "thread_yield failed\n";
            exit(1);
        }
    }
}

void loop2(void *arg) {
    thread_lock(mutex);
    blockThreads++;

    cout << "blocked threads: " << blockThreads << endl;
    thread_wait(mutex, cond);
    cout << "blocked threads: " << blockThreads << endl;
    blockThreads--;

    thread_unlock(mutex);
    thread_signal(mutex, cond);

}

void signal(void* arg)
{
    thread_signal(mutex, cond);
}

void broadcast(void* arg)
{
    thread_broadcast(mutex, cond);
}

void
parent(void *a) {
    int arg;
    arg = (intptr_t) a;
    cout << "parent called with arg " << arg << endl;
    for (int i = 0; i < 5; i++) {
        if (thread_create((thread_startfunc_t) loop, (void *) "child thread")) {
            cout << "thread_create failed\n";
            exit(1);
        }
    }

    loop((void *) "parent thread");

    cout<<"testing thread_signal"<<endl;
    for (int i = 0; i < 5; i++) {
        if (thread_create((thread_startfunc_t) loop2, (void *) "signal thread")) {
            cout << "thread_create failed\n";
            exit(1);
        }
    }
    thread_create((thread_startfunc_t) signal, (void *) "wake thread");
}

int main() {
    if (thread_libinit(parent, (void *) 100)) {
        cout << "thread_libinit failed\n";
        exit(1);
    }
}