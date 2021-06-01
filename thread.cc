#include <cstdlib>
#include <ucontext.h>
#include <iterator>
#include <map>
#include <iostream>
#include "interrupt.h"
#include "thread.h"
#include <vector>

using namespace std;

struct Thread {
  char* stack;
  ucontext_t* contptr;
  bool complete;
  unsigned int threadnum;
};

struct Lock {
  Thread* held;
  vector<Thread*>* stopThreads;
};

typedef void (*thread_startfunc_t)(void *);

static Thread* running;
static ucontext_t* nextcont;
static vector<Thread*> threadq;
static bool initLib = false;
static int threadID = 0;

static map<unsigned int, vector<Thread*>*> threadmap;
static map<unsigned int, Lock*> lockmap;

void switchThread(){
  Thread* thr = threadq.front();
  threadq.erase(threadq.begin());
  running = thr;
}

static int initfunc(thread_startfunc_t func, void *arg) {
  interrupt_enable();
  func(arg);
  interrupt_disable();

  running->complete = true;
  swapcontext(running->contptr, nextcont);
  return 0;
}

void deleteThread() {
  delete running->stack;
  running->contptr->uc_stack.ss_sp = NULL;
  running->contptr->uc_stack.ss_size = 0;
  running->contptr->uc_stack.ss_flags = 0;
  running->contptr->uc_link = NULL;
  delete running->contptr;
  delete running;
  running = NULL;
}

int unlock(unsigned int lock) {
  map<unsigned int, Lock*>::const_iterator lock_iter = lockmap.find(lock);
  Lock* l;
  if(lock_iter != lockmap.end()){
    l = (*lock_iter).second;
    if(l->held == NULL){
      return -1;
    }
    if(l->held->threadnum != running->threadnum){
      return -1;
    }
    
    if(!l->stopThreads->empty()){
      l->held = l->stopThreads->front();
      l->stopThreads->erase(l->stopThreads->begin());
      threadq.push_back(l->held);
    }
    else{
        l->held = NULL;
    }
    return 0;
  }
  return -1;
}

void makeNewLock(Lock* arg, unsigned int lock){
  arg->held = running;
  lockmap.insert(make_pair(lock, arg));
}

void makeNewCond(vector<Thread*>* arg, unsigned int cond){
  arg->push_back(running);
  threadmap.insert(make_pair(cond, arg));
}

void wakeThread(vector<Thread*>* arg){
  Thread* thr = arg->front();
  arg->erase(arg->begin());
  threadq.push_back(thr);
}

int thread_libinit(thread_startfunc_t func, void *arg) {
  if (initLib == true) {
    return -1;
  }

  initLib = true;

  if (thread_create(func, arg) != 0){
    return -1;
  }

  try{
    nextcont = new ucontext_t;
  }
  catch (std:: bad_alloc nomem){
    delete nextcont;
    return -1;
  }
  getcontext(nextcont);
  switchThread();
  
  interrupt_disable();
  swapcontext(nextcont, running->contptr);

  while (!threadq.empty()) {
    if (running->complete == true){
      deleteThread();
    }
    switchThread();
    swapcontext(nextcont, running->contptr);
  }

  if (running != NULL) {
    deleteThread();
  }

  cout << "Thread library exiting.\n";
  exit(0);
}

int thread_create(thread_startfunc_t func, void *arg) {
  if (initLib == false) {
    return -1;
  }
  
  interrupt_disable();
  Thread* make;
  try{
  make = new Thread;
  make->contptr = new ucontext_t;
  make->threadnum = threadID;
  make->complete = false;
  threadID++;
  getcontext(make->contptr);
    
  make->stack = new char [STACK_SIZE];
  make->contptr->uc_stack.ss_sp = make->stack;
  make->contptr->uc_stack.ss_size = STACK_SIZE;
  make->contptr->uc_stack.ss_flags = 0;
  make->contptr->uc_link = NULL;
  makecontext(make->contptr, (void (*)())initfunc, 2, func, arg);
  threadq.push_back(make);
  }
  catch (std:: bad_alloc nomem){
    delete make->contptr;
    delete make->stack;
    delete make;
    interrupt_enable();
    return -1;
  }
  interrupt_enable();
  return 0;
}

int thread_yield(void) {
  if (initLib == false) {
    return -1;
  }
  
  interrupt_disable();
  threadq.push_back(running);
  swapcontext(running->contptr, nextcont);
  interrupt_enable();
  return 0;
}

int thread_lock(unsigned int lock) {
  if (initLib == false){ 
    return -1;
  }

  interrupt_disable();
  map<unsigned int, Lock*>::const_iterator lock_iter = lockmap.find(lock);
  Lock* l;
  if (lock_iter == lockmap.end()) {
    try{
      l = new Lock;
      l->stopThreads = new vector<Thread*>;
    }
    catch (std::bad_alloc nomem){
      delete l;
      delete l->stopThreads;
      interrupt_enable();
      return -1;
    }
    makeNewLock(l, lock);
    interrupt_enable();
    return 0;
  } 

  l = (*lock_iter).second;
  if (l->held == NULL){
    l->held = running;
    interrupt_enable();
    return 0;
  }

  if (l->held->threadnum == running->threadnum) {
    interrupt_enable();
    return -1;
  } 
  l->stopThreads->push_back(running);
  swapcontext(running->contptr, nextcont);

  interrupt_enable();
  return 0;
}

int thread_unlock(unsigned int lock) {
  if (initLib == false) return -1;

  interrupt_disable();
  int result = unlock(lock);
  interrupt_enable();
  return result;
}

int thread_wait(unsigned int lock, unsigned int cond) {
  if (initLib == false) {
    return -1;
  }

  interrupt_disable();
  if (unlock(lock) == 0) {
    map<unsigned int, vector<Thread*>*>::const_iterator thread_iter = threadmap.find(cond);
    if (thread_iter != threadmap.end()) {
      (*thread_iter).second->push_back(running);
    } 
    else{
      vector<Thread*>* waiting;
      try{
        waiting = new vector<Thread*>;
      }
      catch(std::bad_alloc nomem){
        delete waiting;
        interrupt_enable();
        return -1;
      }
      makeNewCond(waiting, cond);
    }
    swapcontext(running->contptr, nextcont);
    interrupt_enable();
    return thread_lock(lock);
  }
  interrupt_enable();
  return -1;
}

int thread_signal(unsigned int lock, unsigned int cond) {
  if (initLib == false) {
    return -1;
  }

  interrupt_disable();
  map<unsigned int, vector<Thread*>*>::const_iterator thread_iter = threadmap.find(cond);
  if (thread_iter != threadmap.end()) { 
    vector<Thread*>* waitingThreads = (*thread_iter).second;
    if (!waitingThreads->empty()) { 
      wakeThread(waitingThreads);
    }
  }

  interrupt_enable();
  return 0;
}

int thread_broadcast(unsigned int lock, unsigned int cond) {
  if (initLib == false) {
    return -1;
  }

  interrupt_disable();
  map<unsigned int, vector<Thread*>*>::const_iterator thread_iter = threadmap.find(cond);
  if (thread_iter != threadmap.end()) { 
    vector<Thread*>* waitingThreads = (*thread_iter).second;
    while (!waitingThreads->empty()) {
      wakeThread(waitingThreads);
    }
  }

  interrupt_enable();
  return 0;
}
