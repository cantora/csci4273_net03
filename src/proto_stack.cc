#include "proto_stack.h"

#include <cassert>
#include <cstdio>

using namespace net03;

void proto_stack::print_msg(net02::message *msg) {
	char buf[2048];
	int len;
	
	len = msg->len();
	assert(len < 2048);

	msg->flatten(buf);
	buf[len] = 0x00;

	printf("%d byte application message received: '%s'\n", len, buf);	
}
