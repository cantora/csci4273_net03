#ifndef NET_IFACE_H
#define NET_IFACE_H

extern "C" {
#include <pthread.h>
#include <stdint.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h> 
}

#include "thread_pool.h"
#include "message.h"

namespace net03 {

/* thread safe wrapper for a virtual hardware layer */
class net_iface {
	public:
		net_iface(int udp_socket, struct sockaddr_in sin, net02::thread_pool *pool, void (*recv_fn)(void *), void *recv_args);
		~net_iface();
		
		void transfer(net02::message *msg) const;

		struct recv_fn_arg_t {
			net02::message *msg;
			void *args;
		};
		
	private:
		static void recv_loop(void *this_ptr);
		static void recv_msg_fn_at_exit(void *thread_data);

		const int m_socket;
		const struct sockaddr_in m_sin;
		mutable pthread_mutex_t m_socket_mtx;

		net02::thread_pool *m_pool;
		void (*m_recv_fn)(void *);
		void *m_recv_args;
				
}; /* net_iface */

}; /* net02 */
#endif /* NET_IFACE_H */
