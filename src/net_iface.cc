#include "net_iface.h"
#include "net03_common.h"

extern "C" {
#include <errno.h>
#include <unistd.h>
}

#include <cassert>

using namespace net03;

net_iface::net_iface(int send_udp_socket, struct sockaddr_in send_sin, int recv_udp_socket, net02::thread_pool *pool, void (*recv_fn)(void *), void *recv_args) 
	: m_send_socket(send_udp_socket), m_sin(send_sin), m_recv_socket(recv_udp_socket), m_pool(pool), m_recv_fn(recv_fn), m_recv_args(recv_args) {

	pthread_mutex_init(&m_send_socket_mtx, NULL);
	
	net03::set_nonblocking(m_recv_socket);

	NET03_LOG("start recv_loop thread\n");
	
	while(m_pool->dispatch_thread(net_iface::recv_loop, this, NULL) < 0) {
		usleep(10000);
	}
}

net_iface::~net_iface() {
	pthread_mutex_destroy(&m_send_socket_mtx);
	close(m_send_socket);
	close(m_recv_socket);
}

void net_iface::transfer(net02::message *msg) const {
	int status, sent, msg_len;
	char *flat_msg;
	
	assert(msg != NULL);

	msg_len = msg->len();

	if(msg_len < 1) {
		return;
	}

	flat_msg = new char[msg_len];
	msg->flatten(flat_msg);

	NET03_LOG("send %d byte message on network iface\n", msg_len );
	while(1) {
		if( (status = pthread_mutex_trylock(&m_send_socket_mtx) ) == 0) {
#ifdef NET03_SLOW_DOWN_UDP_SEND_RATE
			usleep(1000);
#endif
			sent = sendto(m_send_socket, flat_msg, msg_len, 0, (const sockaddr *) &m_sin, sizeof(m_sin) );
	
			if(pthread_mutex_unlock(&m_send_socket_mtx) != 0) {
				FATAL(NULL);
			}
			break;
		}
		else if(status != EBUSY) {
			FATAL(NULL);
		} /* trylock */
		
		usleep(10000);
	} /* while(1) */

	if(sent != msg_len ) {
		if( sent > 0) {
			NET03_LOG("net_iface: failed to write full msg to udp socket. only wrote %d/%d\n", sent, msg_len);
		}
		FATAL(NULL);
	}
}

/* this function may lose 1024 bytes of heap memory if this thread
 * is cancelled. this could be fixed by adding thread cleanup. 
 * for this project, this thread will only get cancelled at exit
 * so im not gonna bother...
 */
void net_iface::recv_loop(void *this_ptr) {
	net_iface *instance = (net_iface *) this_ptr;
	int msg_len, status, bufsize;
	recv_fn_arg_t *for_recv_fn;
	char *buf;
	
	NET03_LOG("net_iface (listen): start recv loop\n");

	bufsize = MAX_UDP_MSG_LEN;
	buf = new char[bufsize];
	
	while(1) {
		//NET03_LOG("net_iface (listen): block on recvfrom\n");
		if( (msg_len = recvfrom(instance->m_recv_socket, buf, bufsize, 0, NULL, NULL)) < 1) {
			if(errno != EAGAIN) {
				FATAL(NULL);	
			}
			//usleep(100);
		}			
		else {
			NET03_LOG("net_iface (listen): received %d byte message\n", msg_len);

			/* this gets deleted by the thread cleanup function */
			for_recv_fn = new recv_fn_arg_t; 

			/* this gets deleted by the guy who receives this message */
			for_recv_fn->msg = new net02::message(buf, msg_len); 
			for_recv_fn->args = instance->m_recv_args;

			while(instance->m_pool->dispatch_thread(instance->m_recv_fn, 
						for_recv_fn, net_iface::recv_msg_fn_at_exit) < 0) {
				usleep(100); /* 0.01 secs */
			}

			buf = new char[bufsize];
			msg_len = 0;
		} /* msg_len > 0 */
	} /* while(1) */

}

void net_iface::recv_msg_fn_at_exit(void *recv_data) {
	NET03_LOG("message cleanup\n");

	delete recv_data;
}
