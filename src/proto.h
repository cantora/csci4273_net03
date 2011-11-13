#ifndef PROTO_H
#define PROTO_H

extern "C" {
#include <stdint.h>
}

#include "message.h"

#define PI_ID_NONE 0
#define PI_ID_ETH 1
#define PI_ID_IP 2
#define PI_ID_TCP 3
#define PI_ID_UDP 4
#define PI_ID_FTP 5
#define PI_ID_TEL 6
#define PI_ID_RDP 7
#define PI_ID_DNS 8

#define PI_NUM_PROTOS PI_ID_DNS

namespace net03 {
	typedef uint8_t  proto_id_t;

	static const char* proto_id_to_name[] = {"none", "ethernet", "ip", "tcp", "udp", "ftp", "telnet", "rdp", "dns"};
	static const size_t proto_id_to_hdr_len[] = {0,8,12,4,4,8,8,12,8};
	
	static const size_t proto_id_to_llp_id[] = {0,0,1,2,2,3,3,4,4};

	void add_proto_header(proto_id_t proto_id, proto_id_t hlp, net02::message* msg);
	proto_id_t strip_proto_header(proto_id_t proto_id, net02::message* msg);

}

#endif /* PROTO_H */