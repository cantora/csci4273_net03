#include "per_proto.h"

#include <cassert>
#include <cerrno>

extern "C" {
#include <unistd.h>
}

#include "net03_common.h"

using namespace std;
using namespace net03;

per_proto::per_proto() : m_pool(new net02::thread_pool(PI_NUM_PROTOS*2)) {
	assert(m_pool != NULL);

	for(int i = 0; i < PI_NUM_PROTOS; i++) {

		if( pipe(&m_protos_up[i].read_pipe) != 0) {
			FATAL(NULL);
		}
		pthread_mutex_init(&m_protos_up[i].write_pipe_mtx, NULL);
		m_protos_up[i].proto_id = i+1;
		m_protos_up[i].netstack = m_protos_up;
		while(m_pool->dispatch_thread(per_proto::proto_process_up, &m_protos_up[i]) < 0) {
			usleep(10000); /* 0.01 secs */
		}
		
		if( pipe(&m_protos_down[i].read_pipe) != 0) {
			FATAL(NULL);
		}
		pthread_mutex_init(&m_protos_down[i].write_pipe_mtx, NULL);
		m_protos_down[i].proto_id = i+1;
		m_protos_down[i].netstack = m_protos_down;;
		while(m_pool->dispatch_thread(per_proto::proto_process_down, &m_protos_down[i]) < 0) {
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
	int status;
	
	while(1) {
		if( (status = pthread_mutex_trylock(pipe_mtx) ) == 0) {
			ipc_msg_t m = {hlp, msg};
			write(pipe, &m, sizeof(ipc_msg_t) );
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

void per_proto::proto_process_up(void *p_desc) {
	proto_desc_t *pd = (proto_desc_t *) p_desc;
	int id = pd->proto_id;
	const char *name = net03::proto_id_to_name[id];

	NET03_LOG("process_up(%d) %s: enter\n", id, name);

	sleep(1);

	NET03_LOG("process_up(%d) %s: exit\n", id, name);
}

void per_proto::proto_process_down(void *p_desc) {
	proto_desc_t *pd = (proto_desc_t *) p_desc;
	int id = pd->proto_id;
	const char *name = net03::proto_id_to_name[id];
	ipc_msg_t new_msg;
	int amt_read;
	char err[64];
	char prefix[32];
	
	sprintf(prefix, "process_down(%d) %s", id, name);
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
		
		/* if we get here we got a message from above */
		
		NET03_LOG("%s: read one msg from hlp %s (%d)\n", prefix, net03::proto_id_to_name[new_msg.hlp], new_msg.hlp);
		net03::add_proto_header(id, new_msg.hlp, new_msg.msg);
		
		if(id == 1) { /* this level is the "hardware" level; send over virtual network */
			NET03_LOG("send %d byte message over the virtual network \n", new_msg.msg->len() );
		} 
		else { /* send this down to the next lower level */
			proto_id_t llp = proto_id_to_llp_id[id];
			assert(llp > 0); assert(llp < PI_NUM_PROTOS);

			send_on_pipe(pd->netstack[llp-1].write_pipe, &pd->netstack[llp-1].write_pipe_mtx, id, new_msg.msg);
		}
	}
	
	NET03_LOG("process_down(%d) %s: exit\n", id, name);
}
