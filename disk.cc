/*George Chiang
ECE 570 Assignment 1 part 1*/

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>
#include <vector>
#include "interrupt.h"
#include <stdlib.h>
#include <sstream>
#include "thread.h"
#include <algorithm>
#include <cstdlib>

using namespace std;

struct pid {
  int fnum;
  int tnum;
};

int mutex, qsize, numThreads, dtrack, qOpen, qFull;
vector<string> inpfiles;
vector<pid*> diskq;
int* state;

bool sortq(pid* a, pid* b) {
  return (abs(a->tnum - dtrack) < abs(b->tnum - dtrack));
}

void rThread(void* arg) {
  int filenum = (int)(intptr_t) arg;
  ifstream getfile(inpfiles.at(filenum).c_str());
  int req = 0;
  
  while (getfile) {
    getfile >> req;
    if(getfile.eof()){
    	break;
    }
    pid* p = new pid;
    p->fnum = filenum;
    p->tnum = req;
    thread_lock(mutex);
    
    while (diskq.size() >= qsize || state[p->fnum] == 1){
      thread_wait(mutex, qOpen);
    }

    cout << "requester " << p->fnum << " track " << p->tnum << endl;
    diskq.push_back(p);
    sort(diskq.begin(), diskq.end(), sortq);
    state[p->fnum] = 1;
    thread_broadcast(mutex, qFull);
  }
  getfile.close();
  
  while (state[filenum] == 1){
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
      pid* requester = diskq.front();
      diskq.erase(diskq.begin());
      cout << "service requester " << requester->fnum << " track " << requester->tnum << endl;
      state[requester->fnum] = 0;
      dtrack = requester->tnum;
    }
    thread_broadcast(mutex, qOpen);
  }
  thread_unlock(mutex);
}

void makeThread(void* arg) {
  thread_create(sThread, NULL);

  for (int i = 0; i < numThreads; i++){
    thread_create(rThread, (void *)(intptr_t) i);
  }
}

int main(int argc, char** argv) {
	ifstream readline;	
	numThreads = argc - 2;
 	dtrack = 0;
  mutex = 1;
  qOpen = 3;
  qFull = 2;
  state = new int[numThreads];

	istringstream inpchk(argv[1]);	
	inpchk >> qsize;	
	
	if(!inpchk.eof()| argc <= 2){ 
		return 0;
	}

	for(int i = 2; i < argc; i++){		
		readline.open(argv[i]);			
		if(readline){						
			inpfiles.push_back(argv[i]);
		}
		readline.close();
	}
  
  	for (int i = 0; i < numThreads; i++){
    state[i] = 0;
	}
  	thread_libinit(&makeThread, NULL);
  
  	return 0;
}