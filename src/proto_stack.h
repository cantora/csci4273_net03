#ifndef PROTO_STACK_H
#define PROTO_STACK_H

#include "message.h"
#include "proto.h"

namespace net03 {

class proto_stack {
	public:
		proto_stack(void (*on_msg_fn)(void *on_msg_data), void *args);
		virtual ~proto_stack();
		virtual void send(proto_id_t proto_id, net02::message *msg) = 0;

		struct on_msg_t {
			net02::message *msg;
			void *args;
			proto_id_t proto_id;
			
		};

	protected:
		static void print_msg(proto_id_t from_proto, net02::message *msg);

		void (*const m_on_msg_fn)(void *on_msg_data);
		void *const m_on_msg_args;	

}; /* proto_stack */

}; /* net03 */
#endif /* PROTO_STACK_H */
