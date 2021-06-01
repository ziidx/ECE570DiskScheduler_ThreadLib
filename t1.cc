#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>
#include "thread.h"

using namespace std;

struct pid {
    int filenum;
    int tracknum;
};

int mutex = 1;
int qFull = 2;
int qOpen = 3;


int qsize = 3;
int numThreads = 5;
int currTrack = 0;

int* state;
vector<pid*> diskq;

unsigned int inpNums[5][2] = {
    {53, 785},
    {914, 350},
    {827, 567},
    {302, 230},
    {631, 11}
};

bool sortq(pid* a, pid* b) {
    return (abs(a->tracknum - currTrack) < abs(b->tracknum - currTrack));
}

void rThread(void* f) {
    int index = (int)(intptr_t) f;
    int i = 0;
    pid* p;

    while (i < 2) {
        p = new pid;
        p->filenum = index;
        p->tracknum = inpNums[p->filenum][i];
        
        thread_lock(mutex);
        while (diskq.size() >= qsize || state[p->filenum] == 1) {
            thread_wait(mutex, qOpen);
        }
        cout << "requester " << p->filenum << " track " << p->tracknum << endl;
        diskq.push_back(p);
        sort(diskq.begin(), diskq.end(), sortq);
        state[p->filenum] = 1;
        i++;
        thread_broadcast(mutex, qFull);
    }

    while (state[p->filenum] == 1) {
        thread_wait(mutex, qOpen);
    }

    numThreads--;
    if (numThreads < qsize) {
        qsize--;
        thread_broadcast(mutex, qFull);
    }
    thread_unlock(mutex);
}

void sThread(void* arg) {
    thread_lock(mutex); 
    while (qsize > 0) {
        while (!(diskq.size() >= qsize)) {
            thread_wait(mutex, qFull);
        }
        if (!diskq.empty()){
            pid* p = diskq.front();
            diskq.erase(diskq.begin());
            cout << "service requester " << p->filenum << " track " << p->tracknum << endl;
            state[p->filenum] = 0;
            currTrack = p->tracknum;
        }
        thread_broadcast(mutex, qOpen);
    }
    thread_unlock(mutex);
}

void makeThreads(void* arg) {
    thread_create(sThread, arg);
    for (int i = 0; i < numThreads; i++) {
        thread_create(rThread, (void *)(intptr_t) i);
    }
}

int main(int argc, char** argv) {
    state = new int[numThreads];
    for (int i = 0; i < 5; i++) {
        state[i] = 0;
    }

    thread_libinit(&makeThreads, NULL);
}