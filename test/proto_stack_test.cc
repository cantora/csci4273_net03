#include <iostream>

#include "per_proto.h"
#include "per_message.h"
#include "sock.h"
#include "net03_common.h"

using namespace std;
using namespace net03;
using namespace net02;
using namespace net01;

const char* msgs[] = {
	"Colombian President Juan Manuel Santos hails the killing of Farc leader Alfonso Cano, as the rebel group reportedly says its struggle will continue.", 
    "That is the theory anyway. But this is where the second loophole comes into play. We often assume that the detectors are actually detecting what we think they are detecting.",
	"In practice, there is no such thing as a single photon, single polarization detector. Instead, what we use is a filter that only allows a particular polarization of light to pass and an intensity detector to look for light. The filter doesn't care how many photons pass through, while the detector plays lots of games to try and be single photon sensitive when, ultimately, it is not. ",
	"It's this gap between",
	" theory and practice that allows a carefully manipulated classical light beam to fool a detector into reporting single photon clicks.",
	"Since Eve has measured the polarization state of the photon,",
	" she knows what polarization state to set on her classical light pulse in order to fake Bob into recording the same measurement result. When Bob and Alice compare notes, they get the right answers and assume everything is on the up and up. ",
	" The researchers demonstrated that this attack succeeds with standard (but not commercial) quantum cryptography equipment under a range of different circumstances. In fact, they could make the setup outperform the quantum implementation for some particular settings." };


int test_proto_stack(proto_stack *ps) {
	int i;
	message *msg;

	sleep(4);
	cout << "test per_proto" << endl;

	cout << "\n\n\n\n\n";
	for(i = 0; i < 10; i++) {
		sleep(0.1);	
		cout.flush();
	}

	//for(i = 0; i < (sizeof(msgs)/4); i++) {
	for(i = 0; i < 1; i++) {
		int len = strlen(msgs[i]);
		char *buf = new char[len]; /* gets deleted by message */
		memcpy(buf, msgs[i], len); 
		
		cout << "send msg " << i+1 << endl;
		msg = new message(buf, len); /* gets deleted by eth proto */
		
		ps->send(PI_ID_FTP, msg);
	}

	sleep(1);
	return 0;
}

int main() {
	struct sockaddr_in sin;
	socklen_t sinlen = sizeof(sin);
	sock::host_sin("localhost", 3456, &sin);
	int recv_socket = sock::bound_udp_sock(&sin, &sinlen);  
	int send_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	proto_stack *ps;

#if 0
	per_proto pp(send_socket, sin, recv_socket);
	ps = &pp;
#else
	per_message pm(send_socket, sin, recv_socket);
	ps = &pm;
#endif
	
	test_proto_stack(ps);	
}
