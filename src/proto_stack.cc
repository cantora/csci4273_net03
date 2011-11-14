#include "proto_stack.h"

#include <cassert>
#include <cstdio>

using namespace net03;

proto_stack::proto_stack(void (*on_msg_fn)(void *on_msg_data), void *args) 
	: m_on_msg_fn(on_msg_fn), m_on_msg_args(args) {

}

proto_stack::~proto_stack() { 
	
}

void proto_stack::print_msg(proto_id_t from_proto, net02::message *msg) {
	char buf[2048];
	int len;
	
	len = msg->len();
	assert(len < 2048);

	msg->flatten(buf);
	buf[len] = 0x00;

	printf("%d byte %s msg received: '%s'\n", len, proto_id_to_name[from_proto], buf);	
}
