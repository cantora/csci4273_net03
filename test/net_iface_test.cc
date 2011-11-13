#include <iostream>

#include "net_iface.h"

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

void recv(void *recv_data) {
	net_iface::recv_fn_arg_t *data = (net_iface::recv_fn_arg_t *) recv_data;
	char buf[2048];

	data->msg->flatten(buf);
	buf[data->msg->len()] = 0x00;

	printf("received message: %s\n", buf);

	delete data->msg;
}

int main() {
	thread_pool tp(20);
	struct sockaddr_in sin;
	socklen_t sinlen = sizeof(sin);
	sock::host_sin("localhost", 3456, &sin);
	int recv_socket = sock::bound_udp_sock(&sin, &sinlen);  
	int send_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	net_iface *ifc;
	int sent,i;
	message *msg;

	assert(recv_socket > 0);
	assert(send_socket > 0);
	//msgs[0] = new char[sizeof(msg1)];
	//memcpy(msgs[0], msg1, sizeof(msg1));
	//message m(msgs[0], sizeof(msg1));
	
	cout << "test net_iface" << endl;

	sleep(1);
	ifc = new net_iface(send_socket, sin, recv_socket, &tp, recv, NULL);
	
	for(i = 0; i < (sizeof(msgs)/4); i++) {
		int len = strlen(msgs[i]);
		char *buf = new char[len];
		memcpy(buf, msgs[i], len);
		
		cout << "send msg " << i+1 << endl;
		msg = new message(buf, len);

		ifc->transfer(msg);
		delete msg;
	}

	sleep(5);
	return 0;
}