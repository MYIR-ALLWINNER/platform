#include <stdio.h>
#include <sys/epoll.h>
#include "Thread.h"
#include "Proto.h"
class Server : public Thread< epoll_event >
{
public:
	Server();
	virtual ~Server();

public:
	virtual int process(epoll_event d);

public:
	int init();
	int run();

private:
	int read(int fd,void* buf,int size);
	int write(int fd, void* buf, int size);
	int handleRequest(int fd, char* buf, int length);
	int responseFail(int fd);
	void sigroutine(int dunno);

private:
	int mEpollfd;
	int mListenfd;
	int mConnectCount;
	int mTargetfd;
	int mWifiModule;
	Proto* mProto;
};
