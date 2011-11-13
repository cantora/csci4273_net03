#ifndef PROTO_STACK_H
#define PROTO_STACK_H

#include "message.h"
#include "proto.h"

namespace net03 {

class proto_stack {
	public:
		
		virtual void send(proto_id_t proto_id, net02::message *msg) = 0;

	protected:
		static void print_msg(net02::message *msg);
						
}; /* proto_stack */

}; /* net03 */
#endif /* PROTO_STACK_H */
