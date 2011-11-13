#ifndef PER_PROTO_H
#define PER_PROTO_H

extern "C" {
#include <pthread.h>
#include <stdint.h>
}

#include "proto.h"
#include "thread_pool.h"
#include "message.h"
#include "net_iface.h"
#include "proto_stack.h"

namespace net03 {

class per_proto : public proto_stack {
	public:
		per_proto(int send_socket, struct sockaddr_in sin, int recv_socket);
		~per_proto();
		
		void send(proto_id_t proto_id, net02::message *msg);
		
	private:
	
		static void send_on_pipe(int pipe, pthread_mutex_t* pipe_mtx, proto_id_t hlp, net02::message *msg);
		static void recv_from_ifc(void *recv_data);
			
		enum proto_desc_type_t {PDESC_UP, PDESC_DOWN};
		static const char *PDESC_UP_STR;
		static const char *PDESC_DOWN_STR;
		
		struct proto_desc_t {
			pthread_mutex_t write_pipe_mtx;
			proto_id_t proto_id;
			proto_desc_type_t type; /* up or down? */
			net_iface *ifc; /* the virtual network interface */
			proto_desc_t* netstack; /* a reference to the array of protocols */

			/* ordering is important here. pipe(&struct.read_pipe) <= pipe(int fildes[2]); */
			int read_pipe; /* fildes[0] */
			int write_pipe; /* fildes[1] */		
		};

		struct ipc_msg_t {
			proto_id_t hlp;
			net02::message *msg;
		};

		static void proto_process(void *p_desc);
		static void proto_process_up(proto_desc_t *pd, ipc_msg_t &new_msg);
		static void proto_process_down(proto_desc_t *pd, ipc_msg_t &new_msg);

		net02::thread_pool *m_pool;

		net_iface m_ifc;

		proto_desc_t m_protos_up[PI_NUM_PROTOS];
		proto_desc_t m_protos_down[PI_NUM_PROTOS];
				
}; /* per_proto */

}; /* net02 */
#endif /* PER_PROTO_H */
