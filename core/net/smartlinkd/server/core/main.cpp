#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "Server.h"
//#include "log.h"
int debug_enable = 0;
//extern "C" void check_chip();
void usage() {
	printf("-h: show help message\n");
	printf("-d: show debug message\n");
}
void parse_cmd(int argc, char* argv[]){
	for (;;) {
		int c = getopt(argc, argv, "dh");
		if (c < 0) {
			break;
		}

		switch (c) {
			case 'd':
				debug_enable = 1;
				break;
			case 'h':
				usage();
				exit(0);
			default:
				break;
		}
	}
}
int main(int argc, char* argv[])
{
	parse_cmd(argc, argv);
	LOGD("test");
	Server s;
	if( 0 == s.init()){
		s.run();
	}

	return 0;
}
