#include "per_proto.h"

#include <cassert>
#include <cerrno>

extern "C" {
#include <unistd.h>
}

#include "net03_common.h"

using namespace std;
using namespace net03;

const char *per_proto::PDESC_UP_STR = "up";
const char *per_proto::PDESC_DOWN_STR = "down";

per_proto::per_proto(int send_socket, struct sockaddr_in sin, int recv_socket) : m_pool(new net02::thread_pool(PI_NUM_PROTOS*2 + 10)), m_ifc(send_socket, sin, recv_socket, m_pool, per_proto::recv_from_ifc, &m_protos_up[PI_ID_ETH-1]) {
	assert(m_pool != NULL);

	for(int i = 0; i < PI_NUM_PROTOS; i++) {

		if( pipe(&m_protos_up[i].read_pipe) != 0) {
			FATAL(NULL);
		}
		pthread_mutex_init(&m_protos_up[i].write_pipe_mtx, NULL);
		m_protos_up[i].proto_id = i+1;
		m_protos_up[i].type = PDESC_UP;
		m_protos_up[i].ifc = &m_ifc;
		m_protos_up[i].netstack = m_protos_up;
		while(m_pool->dispatch_thread(per_proto::proto_process, &m_protos_up[i], NULL) < 0) {
			usleep(10000); /* 0.01 secs */
		}
		
		if( pipe(&m_protos_down[i].read_pipe) != 0) {
			FATAL(NULL);
		}
		pthread_mutex_init(&m_protos_down[i].write_pipe_mtx, NULL);
		m_protos_down[i].proto_id = i+1;
		m_protos_down[i].type = PDESC_DOWN;
		m_protos_down[i].ifc = &m_ifc;
		m_protos_down[i].netstack = m_protos_down;;
		while(m_pool->dispatch_thread(per_proto::proto_process, &m_protos_down[i], NULL) < 0) {
			usleep(10000); /* 0.01 secs */
		}

	}
}

per_proto::~per_proto() {
	NET03_LOG("per_proto: destroy\n");
	delete m_pool;

	for(int i = 0; i < PI_NUM_PROTOS; i++) {
		pthread_mutex_destroy(&m_protos_up[i].write_pipe_mtx);
		pthread_mutex_destroy(&m_protos_down[i].write_pipe_mtx);
		
		close(m_protos_up[i].read_pipe);
		close(m_protos_up[i].write_pipe);
	}
}

void per_proto::send(proto_id_t proto_id, net02::message *msg) {
	assert(proto_id > 0);
	assert(proto_id < PI_NUM_PROTOS);
	
	NET03_LOG("send %d byte message over protocol %s\n", msg->len(), net03::proto_id_to_name[proto_id]);

	send_on_pipe(m_protos_down[proto_id-1].write_pipe, &m_protos_down[proto_id-1].write_pipe_mtx, 0, msg);	
}

void per_proto::send_on_pipe(int pipe, pthread_mutex_t* pipe_mtx, proto_id_t hlp, net02::message *msg) {
	int status, written;
	
	while(1) {
		if( (status = pthread_mutex_trylock(pipe_mtx) ) == 0) {
			ipc_msg_t m = {hlp, msg};

			if( (written = write(pipe, &m, sizeof(ipc_msg_t) ) ) != sizeof(ipc_msg_t) ) {
				FATAL(NULL);
			}

			if(pthread_mutex_unlock(pipe_mtx) != 0) {
				FATAL(NULL);
			}
			break;
		}		
		else if(status != EBUSY) {
			FATAL(NULL);
		}
		
		usleep(10000);
	}
}

void per_proto::proto_process(void *p_desc) {
	proto_desc_t *pd = (proto_desc_t *) p_desc;
	const char *name = net03::proto_id_to_name[pd->proto_id];
	const char *type_name;
	ipc_msg_t new_msg;
	int amt_read;
	char err[64];
	char prefix[32];
	
	type_name = (pd->type == PDESC_UP)? PDESC_UP_STR : PDESC_DOWN_STR;
	sprintf(prefix, "process_%s(%d) %s", type_name, pd->proto_id, name);
	NET03_LOG("%s: enter\n", prefix);

	net03::set_nonblocking(pd->read_pipe);

	NET03_LOG("%s: enter read loop\n", prefix);
	while(1) {		
		amt_read = read(pd->read_pipe, &new_msg, sizeof(ipc_msg_t) );

		if(amt_read != sizeof(ipc_msg_t) ) {
			if(amt_read < 0) {
				if(errno == EAGAIN) {
					usleep(1000);
					continue; /* try again */
				}
				
				sprintf(err, "%s: read_pipe error", prefix);
			}
			else {
				sprintf(err, "%s: only read %d bytes", prefix, amt_read);
			}
			FATAL(err);
		}
		
		/* if we get here we got a message from above or below */
		
		if(pd->type == PDESC_DOWN) {
			NET03_LOG("%s: read one msg from hlp %s (%d)\n", prefix, net03::proto_id_to_name[new_msg.hlp], new_msg.hlp);
			proto_process_down(pd, new_msg);
		} else { /* up */
			proto_id_t llp = proto_id_to_llp_id[pd->proto_id];
			NET03_LOG("%s: read one msg from llp %s (%d)\n", prefix, net03::proto_id_to_name[llp], llp);
			
			proto_process_up(pd, new_msg);			
		}		
	}
	
	assert(false); /* shouldnt get here */
}

void per_proto::proto_process_up(proto_desc_t *pd, ipc_msg_t &new_msg) {
	proto_id_t dmux_id;
	
	dmux_id = net03::strip_proto_header(pd->proto_id, new_msg.msg);

	if(dmux_id == PI_ID_NONE) { /* this level is the "application" level; just print the msg */
		print_msg(new_msg.msg);
		delete new_msg.msg; /* message is no longer needed after this */
	} 
	else if(dmux_id > 0 && dmux_id <= PI_NUM_PROTOS) { /* send this down to the next lower level */

		send_on_pipe(pd->netstack[dmux_id-1].write_pipe, &pd->netstack[dmux_id-1].write_pipe_mtx, pd->proto_id, new_msg.msg);
	}
	else {
		FATAL(NULL);
	}
}

void per_proto::proto_process_down(proto_desc_t *pd, ipc_msg_t &new_msg) {

	net03::add_proto_header(pd->proto_id, new_msg.hlp, new_msg.msg);
	
	if(pd->proto_id == PI_ID_ETH) { /* this level is the "hardware" level; send over virtual network */
		pd->ifc->transfer(new_msg.msg);
		/* whoever passed this to per_proto should delete this */
		//delete new_msg.msg; /* message is no longer needed after this */
	} 
	else { /* send this down to the next lower level */
		proto_id_t llp = proto_id_to_llp_id[pd->proto_id];
		assert(llp > 0); assert(llp < PI_NUM_PROTOS);

		send_on_pipe(pd->netstack[llp-1].write_pipe, &pd->netstack[llp-1].write_pipe_mtx, pd->proto_id, new_msg.msg);
	}
}

void per_proto::recv_from_ifc(void *recv_data) {
	net_iface::recv_fn_arg_t *data = (net_iface::recv_fn_arg_t *) recv_data;
	int len;
	proto_desc_t *hw_proto = (proto_desc_t *) data->args;

	len = data->msg->len();
	assert(len < net_iface::MAX_UDP_MSG_LEN);

	NET03_LOG("pass %d byte message from vn to eth proto\n", len);

	send_on_pipe(hw_proto->write_pipe, &hw_proto->write_pipe_mtx, 0, data->msg);
}
