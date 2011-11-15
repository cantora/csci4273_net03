#include "proto.h"

#include <cassert>

extern "C" {
#include <arpa/inet.h>
}

void net03::add_proto_header(proto_id_t proto_id, proto_id_t hlp, net02::message* msg) {
	size_t hdr_len = net03::proto_id_to_hdr_len[proto_id];
	char *header = new char[1+hdr_len+2]; /* msg will destroy this when ready */
	int len;

	len = msg->len();
	assert(len > 0 && len < (1 << 16) );
	assert(sizeof(proto_id_t) == 1);

	*((proto_id_t *) header) = hlp;
	memset(header+1, 0xaa, hdr_len);
	*((uint16_t *) (header + 1 + hdr_len)) = htons(len); 

	msg->add_header(header, 1 + hdr_len + 2);
}

/* returns protocol id from this header */
net03::proto_id_t net03::strip_proto_header(proto_id_t proto_id, net02::message* msg) {
	int other_data_len = net03::proto_id_to_hdr_len[proto_id];
	char *header = msg->strip_header(1 + other_data_len + 2);
	int len = msg->len();
	
	assert(sizeof(proto_id_t) == 1);
	assert(len == ntohs(*((uint16_t *) (header + 1 + other_data_len) ) ) ); 
 
	return *((proto_id_t *) header);
}
	