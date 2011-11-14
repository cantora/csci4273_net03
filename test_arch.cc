#include <iostream>

#include "per_proto.h"
#include "per_message.h"
#include "sock.h"
#include "net03_common.h"

extern "C" {
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/time.h>
}

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

#if COUNT_WITH_SEMS
sem_t dns_recd_count;
sem_t rdp_recd_count;
sem_t tel_recd_count;
sem_t ftp_recd_count;
#else
int dns_recd_count = 0;
int rdp_recd_count = 0;
int tel_recd_count = 0;
int ftp_recd_count = 0;
#endif

void recv_message(void *on_msg_data) {
	proto_stack::on_msg_t *data = (proto_stack::on_msg_t *) on_msg_data;
	//printf("recd %s msg\n", proto_id_to_name[data->proto_id]);
	
	switch(data->proto_id) {
		case PI_ID_FTP :
#if COUNT_WITH_SEMS
			sem_post(&ftp_recd_count);
#else
			ftp_recd_count++;
#endif
			break;
		case PI_ID_DNS :
#if COUNT_WITH_SEMS
			sem_post(&dns_recd_count);
#else
			dns_recd_count++;
#endif
			break;
		case PI_ID_TEL :
#if COUNT_WITH_SEMS
			sem_post(&tel_recd_count);
#else
			tel_recd_count++;
#endif
			break;
		case PI_ID_RDP :
#if COUNT_WITH_SEMS
			sem_post(&rdp_recd_count);
#else
			rdp_recd_count++;
#endif
			break;
		default :
			FATAL(NULL); 
	}
}

/* 
 * note: with process-per-message this can cause 
 * problems when the thread pool is too small.
 * if all the threads are being used then when
 * this thread goes to try and send a message
 * the per_message class will spin forever trying 
 * to dispatch a thread.
 */
void echo_message(void *on_msg_data) {
	proto_stack::on_msg_t *data = (proto_stack::on_msg_t *) on_msg_data;
	proto_stack *ps = *( (proto_stack **)data->args); 
	char *buf;
	int len = data->msg->len();
	
	//printf("recd %s msg\n", proto_id_to_name[data->proto_id]);

	buf = new char[len];
	data->msg->flatten(buf);

	ps->send(data->proto_id, new message(buf, len) );
}

int test_proto_stack(proto_stack *ps, proto_id_t app_proto) {
	int i;
	message *msg;
	char buf[256];

	//sleep(4);
	//printf("test proto_stack %s\n", proto_id_to_name[app_proto]);

	//printf("\n\n\n\n\n");
	for(i = 0; i < 10; i++) {
		sleep(0.1);	
		fflush(stdout);
	}

	for(i = 0; i < 100; i++) {
		msg = create_msg();
		//msg->flatten(buf);
		//buf[msg->len()] = 0x00;
		//printf("send msg %d\n", i+1);
						
		ps->send(app_proto, msg); /* eth proto will delete msg */
	}

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
const char act_name[] = "active";
const char echo_name[] = "echo";
const char usage[] = "usage: %s ARCH_TYPE COMM_TYPE LISTEN_PORT HOST_PORT [HOST]\n\tARCH_TYPE := %s | %s\n\tCOMM_TYPE := %s | %s\n\n";
	
void usage_exit(const char *argv0) {
	printf(usage, argv0, pm_name, pp_name, act_name, echo_name);
	exit(1);
}

int main(int argc, char *argv[]) {
	struct sockaddr_in sin;
	socklen_t sinlen = sizeof(struct sockaddr_in);
	struct sockaddr_in bound_sin;
	int recv_socket;
	int send_socket;
	short listen_port, host_port;
	const char* host = "localhost";
	proto_stack *ps;
	thread_pool tp(4);
	int arch_type, comm_type, i;
	arch_test_t tests[4];
	proto_id_t test_ids[4] = {PI_ID_FTP, PI_ID_TEL, PI_ID_RDP, PI_ID_DNS};
	void (*rec_msg_fn)(void *);
	void *rec_msg_args;
	struct timeval t_start, t_end;
	double d_start, d_end;
	
	if(argc < 5 || argc > 6) {
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

		if(strncmp(argv[2], act_name, sizeof(act_name) ) == 0) {
			comm_type = 0;
			rec_msg_fn = recv_message;
			rec_msg_args = NULL;
		}
		else if(strncmp(argv[2], echo_name, sizeof(echo_name) ) == 0) {
			comm_type = 1;
			rec_msg_fn = echo_message;
			rec_msg_args = &ps;
		}
		else {
			usage_exit(argv[0]);
		}

		listen_port = strtol(argv[3], NULL, 10);
		host_port = strtol(argv[4], NULL, 10);
		
		if(argc == 6) {
			host = argv[5];
		}
	}

	sock::host_sin(host, host_port, &sin);
	sock::host_sin("0.0.0.0", listen_port, &bound_sin);
	recv_socket = sock::bound_udp_sock(&bound_sin, &sinlen);
	send_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	/*
	int target_size = (1 << 15);
	int sock_buf_size;
	socklen_t len;
	
	if(setsockopt(recv_socket, SOL_SOCKET, SO_RCVBUF, &target_size, sizeof(int)) != 0) {
		FATAL(NULL);
	}
	if(getsockopt(recv_socket, SOL_SOCKET, SO_RCVBUF, &sock_buf_size, &len) != 0) {
		FATAL(NULL);
	}
	printf("sock_buf_size: %d\n", sock_buf_size);
	*/

	srand(time(NULL));

#if COUNT_WITH_SEMS
	sem_init(&dns_recd_count, 0, 0);
	sem_init(&rdp_recd_count, 0, 0);
	sem_init(&tel_recd_count, 0, 0);
	sem_init(&ftp_recd_count, 0, 0);
#endif

	if(arch_type == 0) {
		ps = new per_message(send_socket, sin, recv_socket, rec_msg_fn, rec_msg_args);
	}
	else {
		ps = new per_proto(send_socket, sin, recv_socket, rec_msg_fn, rec_msg_args);
	}

	gettimeofday(&t_start, NULL);

	if(comm_type == 0) {
		for(i = 0; i < 4; i++) {
			tests[i].ps = ps;
			tests[i].app_proto_id = test_ids[i];
			while(tp.dispatch_thread(arch_test_thread, &tests[i], NULL) < 0) {
				usleep(10000);
			}
		}
	}

	if(comm_type == 0) {
#ifdef NET03_ON_MSG_CALLBACK
		int dns, rdp, tel, ftp;
		while(1) {
#if COUNT_WITH_SEMS
			sem_getvalue(&dns_recd_count, &dns);
			sem_getvalue(&rdp_recd_count, &rdp);
			sem_getvalue(&tel_recd_count, &tel);
			sem_getvalue(&ftp_recd_count, &ftp);
#else
			dns = dns_recd_count;
			rdp = rdp_recd_count;
			tel = tel_recd_count;
			ftp = ftp_recd_count;
#endif
			printf("dns: %d, rdp: %d, tel: %d, ftp: %d\r", dns, rdp, tel, ftp);
			fflush(stdout);
			if(dns >= 100 && rdp >= 100 && tel >= 100 && ftp >= 100) {
				break;
			}
			usleep(1000);
		}
		gettimeofday(&t_end, NULL);
		d_start = t_start.tv_sec*1000000 + (t_start.tv_usec);
    	d_end = t_end.tv_sec*1000000  + (t_end.tv_usec);
		printf("\n");
    	printf("Total Time Taken: %f\n", d_end - d_start);
    		
#else
		sleep(10);
#endif
	}
	else {
		while(1) {
			char c;
			printf("echo mode... press any key to exit\n");
			scanf("%c", &c);
			break;
		}
	}
	
	printf("\n");

#if COUNT_WITH_SEMS
	sem_destroy(&dns_recd_count);
	sem_destroy(&rdp_recd_count);
	sem_destroy(&tel_recd_count);
	sem_destroy(&ftp_recd_count);
#endif

	delete ps;
}
