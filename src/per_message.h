#ifndef PER_MESSAGE_H
#define PER_MESSAGE_H

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

class per_message : public proto_stack {
	public:
		per_message(int send_socket, struct sockaddr_in sin, int recv_socket);
		~per_message();
		
		void send(proto_id_t proto_id, net02::message *msg);
		
	private:
	
		struct per_msg_data_t {
			net02::message *msg;
			per_message *instance;
			proto_id_t id;
		};	
	
		static void recv_from_ifc(void *recv_data);
		static void process_down(void *per_msg_data);
		static void process_up(proto_id_t proto_id, per_message *instance, net02::message *msg);

		net02::thread_pool *m_pool;
		net_iface m_ifc;

		proto_id_t (*m_proto_down[PI_NUM_PROTOS])(proto_id_t hlp, net02::message *msg);
		proto_id_t (*m_proto_up[PI_NUM_PROTOS])(net02::message *msg);
		
}; /* per_message */

}; /* net03 */
#endif /* PER_MESSAGE_H */
