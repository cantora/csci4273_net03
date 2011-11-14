#include <iostream>

#include "per_proto.h"
#include "per_message.h"
#include "sock.h"
#include "net03_common.h"

using namespace std;
using namespace net03;
using namespace net02;
using namespace net01;

#include "word_list.h"

message *create_msg() {
	message *msg;
	char *buf;
	char flat[256];
	int i;
	buf = new char[100]; /* gets deleted by message */

	for(i = 0; i < 100; ) {
		const char *word = word_list[rand() % WORD_LIST_SIZE];
		int wlen = strlen(word);
		//printf("%s ", word);
		int amt = (i + wlen < 100-1)? wlen : 100-i-1;
		
		if(amt > 0) {
			memcpy(buf + i, word, amt);
			i += amt;
		}
		buf[i++] = 0x20;
	}

	//printf("\n");
	msg = new message(buf, i);
	/*msg->flatten(flat);
	flat[i] = 0x00;
	printf("new message: %s\n", flat);*/

	return msg;
}

int test_proto_stack(proto_stack *ps, proto_id_t app_proto) {
	int i;
	message *msg;
	char buf[256];

	sleep(4);
	printf("test proto_stack\n");

	printf("\n\n\n\n\n");
	for(i = 0; i < 10; i++) {
		sleep(0.1);	
		fflush(stdout);
	}

	for(i = 0; i < 100; i++) {
		msg = create_msg();
		//msg->flatten(buf);
		//buf[msg->len()] = 0x00;
		printf("send msg %d\n", i+1);
						
		ps->send(app_proto, msg); /* eth proto will delete msg */
	}

	sleep(1);
	return 0;
}

struct arch_test_t {
	proto_stack *ps;
	proto_id_t app_proto_id;	
};

void arch_test_thread(void *arch_test) {
	arch_test_t *at = (arch_test_t *) arch_test;

	test_proto_stack(at->ps, at->app_proto_id);
}

const char pm_name[] = "per-message";
const char pp_name[] = "per-protocol";
const char usage[] = "usage: %s ARCH_TYPE PORT [HOST]\n\tARCH_TYPE := %s | %s\n\n";
	
void usage_exit(const char *argv0) {
	printf(usage, argv0, pm_name, pp_name);
	exit(1);
}

int main(int argc, char *argv[]) {
	struct sockaddr_in sin;
	socklen_t sinlen = sizeof(struct sockaddr_in);
	struct sockaddr_in bound_sin;
	int recv_socket;
	int send_socket;
	short port;
	const char* host = "localhost";
	proto_stack *ps;
	thread_pool tp(4);
	int arch_type,i;
	arch_test_t tests[4];
	proto_id_t test_ids[4] = {PI_ID_FTP, PI_ID_TEL, PI_ID_RDP, PI_ID_DNS };

	if(argc < 3 || argc > 4) {
		usage_exit(argv[0]);
	}
	else {		
		if(strncmp(argv[1], pm_name, sizeof(pm_name) ) == 0) {
			arch_type = 0;
		}
		else if(strncmp(argv[1], pp_name, sizeof(pp_name) ) == 0) {
			arch_type = 1;
		}
		else {
			usage_exit(argv[0]);
		}
		port = strtol(argv[2], NULL, 10);
		
		if(argc == 4) {
			host = argv[3];
		}
	}

	sock::host_sin(host, port, &sin);
	sock::host_sin("localhost", port, &bound_sin);
	recv_socket = sock::bound_udp_sock(&sin, &sinlen);
	send_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	srand(time(NULL));

	if(arch_type == 0) {
		ps = new per_message(send_socket, sin, recv_socket);

		for(i = 0; i < 4; i++) {
			tests[i].ps = ps;
			tests[i].app_proto_id = test_ids[i];
			while(tp.dispatch_thread(arch_test_thread, &tests[i], NULL) < 0) {
				usleep(10000);
			}
		}

		sleep(45);
	}
	else {
		
	}

	delete ps;
}
