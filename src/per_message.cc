#include "per_message.h"

#include <cassert>
#include <cerrno>

extern "C" {
#include <unistd.h>
}

#include "net03_common.h"

using namespace std;
using namespace net03;

#define PROTO_DOWN_FN(ID) proto_process_down_ ## ID
#define PROTO_UP_FN(ID) proto_process_up_ ## ID

#define PROTO_DOWN_FN_DEF(ID) static proto_id_t PROTO_DOWN_FN(ID)(proto_id_t hlp, net02::message *msg) {\
	net03::add_proto_header(ID, hlp, msg); \	
	return proto_id_to_llp_id[ID]; \
}

#define PROTO_UP_FN_DEF(ID) static proto_id_t PROTO_UP_FN(ID)(net02::message *msg) { \
	return net03::strip_proto_header(ID, msg); \
}

PROTO_DOWN_FN_DEF(8)
PROTO_DOWN_FN_DEF(7)
PROTO_DOWN_FN_DEF(6)
PROTO_DOWN_FN_DEF(5)
PROTO_DOWN_FN_DEF(4)
PROTO_DOWN_FN_DEF(3)
PROTO_DOWN_FN_DEF(2)
PROTO_DOWN_FN_DEF(1)

PROTO_UP_FN_DEF(8)
PROTO_UP_FN_DEF(7)
PROTO_UP_FN_DEF(6)
PROTO_UP_FN_DEF(5)
PROTO_UP_FN_DEF(4)
PROTO_UP_FN_DEF(3)
PROTO_UP_FN_DEF(2)
PROTO_UP_FN_DEF(1)

#define PROTO_ARR_INIT(ID) m_proto_down[ID-1] = PROTO_DOWN_FN(ID); \
	m_proto_up[ID-1] = PROTO_UP_FN(ID);


per_message::per_message(int send_socket, struct sockaddr_in sin, int recv_socket, void (*on_msg_fn)(void *on_msg_data), void *args) : proto_stack(on_msg_fn, args), m_pool(new net02::thread_pool(100 + 1)), m_ifc(send_socket, sin, recv_socket, m_pool, per_message::recv_from_ifc, this) {
	int i;
	assert(m_pool != NULL);

	PROTO_ARR_INIT(1);
	PROTO_ARR_INIT(2);
	PROTO_ARR_INIT(3);
	PROTO_ARR_INIT(4);
	PROTO_ARR_INIT(5);
	PROTO_ARR_INIT(6);
	PROTO_ARR_INIT(7);
	PROTO_ARR_INIT(8);
}

per_message::~per_message() {
	NET03_LOG("per_message: destroy\n");
	delete m_pool;

}

void per_message::send(proto_id_t proto_id, net02::message *msg) {
	per_msg_data_t *pmd;

	assert(proto_id > 0);
	assert(proto_id <= PI_NUM_PROTOS);
	
	NET03_LOG("send %d byte message over protocol %s\n", msg->len(), net03::proto_id_to_name[proto_id]);

	pmd = new per_msg_data_t; /* gets deleted by process_down */
	pmd->instance = this;
	pmd->id = proto_id;
	pmd->msg = msg;

	
	while(m_pool->dispatch_thread(per_message::process_down, pmd, NULL) < 0) {
		usleep(1000);
	}	
	NET03_LOG("dispatched thread to send message\n");
}

void per_message::process_down(void *per_msg_data) {
	per_msg_data_t *pmd = (per_msg_data_t *) per_msg_data;
	proto_id_t next_id, hlp, tmp;
	const char *next_name;
	char prefix[32];

	assert(pmd->id > 0);
	assert(pmd->id <= PI_NUM_PROTOS);
	assert(pmd->msg->len() > 0);

	/*sprintf(prefix, "process_down(0x%08x)", pmd->msg);
	NET03_LOG("%s: process %d byte outgoing message\n", prefix, pmd->msg->len());*/

	next_id = pmd->id;
	hlp = PI_ID_NONE;

	while(next_id != PI_ID_NONE) {
		next_name = proto_id_to_name[next_id];
		//NET03_LOG("%s: %s (%d)\n", prefix, next_name, next_id);
		tmp = next_id;
		next_id = pmd->instance->m_proto_down[next_id-1](hlp, pmd->msg);
		hlp = tmp;
	}

	pmd->instance->m_ifc.transfer(pmd->msg);

	/* delete the msg now */
	delete pmd->msg;
	delete pmd;
}

void per_message::process_up(proto_id_t proto_id, per_message *instance, net02::message *msg) {
	proto_id_t next_id, llp, tmp;
	const char *next_name;
	char prefix[32];
	on_msg_t on_msg_data = {msg, instance->m_on_msg_args, proto_id};
	assert(proto_id > 0);
	assert(proto_id <= PI_NUM_PROTOS);
	assert(msg->len() > 0);

	/*sprintf(prefix, "process_up(0x%08x)", msg);
	NET03_LOG("%s: process %d byte incoming message\n", prefix, msg->len());*/

	next_id = proto_id;
	llp = PI_ID_NONE;

	while(next_id != PI_ID_NONE) {
		next_name = proto_id_to_name[next_id];
		//NET03_LOG("%s: %s (%d)\n", prefix, next_name, next_id);
		tmp = next_id;
		next_id = instance->m_proto_up[next_id-1](msg);
		llp = tmp;
	}

#ifdef NET03_ON_MSG_CALLBACK
	assert(instance->m_on_msg_fn != NULL);
	on_msg_data.proto_id = llp;
	instance->m_on_msg_fn(&on_msg_data);
#else
	/* the "application" level. just print the msg. */
	print_msg(llp, msg);
#endif

	delete msg;
}

void per_message::recv_from_ifc(void *recv_data) {
	net_iface::recv_fn_arg_t *data = (net_iface::recv_fn_arg_t *) recv_data;
	per_message *instance = (per_message *) data->args;
	int len;
	
	len = data->msg->len();
	assert(len < net_iface::MAX_UDP_MSG_LEN);

	NET03_LOG("pass %d byte message from vn to eth proto\n", len);

	process_up(PI_ID_ETH, instance, data->msg);
}
