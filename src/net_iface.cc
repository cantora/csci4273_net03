#include "net_iface.h"

net_iface::net_iface(int udp_socket, struct sockaddr_in sin, void (*recv_fn)(void *), void *recv_args) 
	: m_socket(udp_socket), m_sin(sin), m_recv_fn(recv_fn), m_recv_args(recv_args) {

	pthread_mutex_init(&m_socket_mtx, NULL);
	
	net03::set_nonblocking(m_socket);

	while(m_pool->dispatch_thread(recv_loop, this) < 0) {
		usleep(10000); /* 0.01 secs */
	}
}

net_iface::~net_iface() {
	pthread_mutex_destroy(&m_socket_mtx);
	close(m_socket);
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
		if( (status = pthread_mutex_trylock(m_socket_mtx) ) == 0) {
			sent = sendto(m_socket, flat_msg, msg_len, 0, (const sockaddr *) &m_sin, sizeof(m_sin) );

			if(sent != msg_len ) {
				if( sent > 0) {
					NET03_LOG("net_iface: failed to write full msg to udp socket. only wrote %d/%d\n", sent, msg_len);
				}
				FATAL(NULL);
			}

			if(pthread_mutex_unlock(m_socket_mtx) != 0) {
				FATAL(NULL);
			}
			break;
		}		
		else if(status != EBUSY) {
			FATAL(NULL);
		}
		
		usleep(10000);
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
	socklen_t sinlen;
	recv_fn_arg_t for_recv_fn;
	char *buf;
	
	bufsize = 1024;
	buf = new char[bufsize];
	
	while(1) {
		sinlen = sizeof(instance->m_sin);

		if( (status = pthread_mutex_trylock(m_socket_mtx) ) == 0) {
			if( (msg_len = recvfrom(instance->m_socket, buf, bufsize, 0, (sockaddr *)& instance->m_sin, &sinlen)) < 1) {
				if(errno != EAGAIN) {
					FATAL(NULL);	
				}
			}			

			if(pthread_mutex_unlock(m_socket_mtx) != 0) {
				FATAL(NULL);
			}

			if(msg_len > 0) {
				for_recv_fn.msg = new net02::message(buf, msg_len);
				for_recv_fn.args = instance->m_recv_args;

				while(m_pool->dispatch_thread(instance->m_recv_fn, ) < 0) {
					usleep(10000); /* 0.01 secs */
				}

				buf = new char[bufsize];
				msg_len = 0;
			}
		}		
		else if(status != EBUSY) {
			FATAL(NULL);
		}
		
		usleep(10000);
	}	

}