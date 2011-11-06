#include "proto_info.h"

void net03::add_proto_header(proto_id_t proto_id, proto_id_t hlp, net02::message* msg) {
	size_t hdr_len = net03::proto_id_to_hdr_len[proto_id];
	char *header = new char[hdr_len]; /* msg will destroy this when ready */

	*((proto_id_t *) header) = hlp;
	memset(header+2, 0xaa, hdr_len - sizeof(proto_id_t) );

	msg->add_header(header, hdr_len);
}

/* returns protocol id from this header */
net03::proto_id_t net03::strip_proto_header(proto_id_t proto_id, net02::message* msg) {
	char *header = msg->strip_header(net03::proto_id_to_hdr_len[proto_id]);

	return *((proto_id_t *) header);
}
	