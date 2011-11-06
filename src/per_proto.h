#ifndef PER_PROTO_H
#define PER_PROTO_H

extern "C" {
#include <pthread.h>
#include <stdint.h>
}

#include "proto_info.h"
#include "thread_pool.h"
#include "message.h"

namespace net03 {

class per_proto {
	public:
		per_proto();
		~per_proto();
		
		void send(proto_id_t proto_id, net02::message *msg);
		
	private:
	
		struct proto_desc_t {
			pthread_mutex_t write_pipe_mtx;
			proto_id_t proto_id;
		
			/* ordering is important here. pipe(&struct.read_pipe) <= pipe(int fildes[2]); */
			int read_pipe; /* fildes[0] */
			int write_pipe; /* fildes[1] */		
		};

		struct ipc_msg_t {
			proto_id_t hlp;
			net02::message *msg;		
		};

		static void proto_process_up(void *p_desc);
		static void proto_process_down(void *p_desc);

		net02::thread_pool *m_pool;

		proto_desc_t m_protos_up[PI_NUM_PROTOS];
		proto_desc_t m_protos_down[PI_NUM_PROTOS];
				
}; /* per_proto */

}; /* net02 */
#endif /* PER_PROTO_H */
