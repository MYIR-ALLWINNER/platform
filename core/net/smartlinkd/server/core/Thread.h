#ifndef __THREAD_H__
#define __THREAD_H__

#include <list>
#include <stdio.h>
#include <sys/epoll.h>
#include "locker.h"
#include "log.h"
using namespace std;

template< typename T >
class Thread{
public:
	Thread();
	virtual ~Thread();

public:
	static const int THREAD_EXIT = 0xff;
	static const int THREAD_CONTINUE = 0xf0;

public:
	pthread_t getTid();
	int run();

	virtual int loop();
	virtual int process(T d);
	int append(T d);

protected:
	//threads
	static const int MAX_CMD_NUMBER = 20;
	bool mStop;


private:
	static void* run(void* arg);

private:
	pthread_t mTid;

	list< T > mWorkqueue;
	locker mQueuelocker;
	sem mQueuestat;

};

//#define LOGE printf
//#define LOGD printf

template< typename T >
Thread< T >::Thread(){
	mStop = false;
	//LOGD("Thread comstruct\n");
}

template< typename T >
Thread< T >::~Thread(){
	//LOGD("Thread discomstruct\n");
};

template< typename T >
void* Thread< T >::run(void* arg){
	Thread* t = (Thread*)arg;
	while(1){
		if(THREAD_EXIT == t->loop()) break;
	}
	return (void*)0;
}

template< typename T >
int Thread< T >::run(){

	if(pthread_create( &mTid, NULL, run, this ) != 0){
		perror("create read thread failed!\n");
		return -1;
	}
	if( pthread_detach( mTid ) ){
		perror("detach read thread failed!\n");
		return -1;
	}
	return 0;
}

template< typename T >
pthread_t Thread< T >::getTid(){
	return mTid;
}

template< typename T >
int Thread< T >::append(T d)
{
	//LOGE("append\n");
	mQueuelocker.lock();
	if ( mWorkqueue.size() > MAX_CMD_NUMBER )
	{
		mQueuelocker.unlock();
		return -1;
	}

	mWorkqueue.push_back( d );
	mQueuelocker.unlock();

	mQueuestat.post();
	return 0;
}

template< typename T >
int Thread< T >::loop(){
	//LOGD("loop start\n");
	mQueuestat.wait();
	mQueuelocker.lock();
	if ( mWorkqueue.empty() ){
		mQueuelocker.unlock();
		return 0;
	}
	T d = mWorkqueue.front();
	mWorkqueue.pop_front();
	mQueuelocker.unlock();

	return process(d);
}

template< typename T >
int Thread< T >::process(T d){
	//base class virtual function process do nothings;
	//LOGD("do nothings\n");
	return THREAD_EXIT;
}
#endif
