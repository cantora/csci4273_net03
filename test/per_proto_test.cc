#include <iostream>

#include "per_proto.h"

using namespace std;
using namespace net03;
using namespace net02;

const char msg1[] = "Colombian President Juan Manuel Santos hails the killing of Farc leader Alfonso Cano, as the rebel group reportedly says its struggle will continue.";

int main() {
	per_proto pp;
	char *msgs[100];
	msgs[0] = new char[sizeof(msg1)];
	memcpy(msgs[0], msg1, sizeof(msg1));
	message m(msgs[0], sizeof(msg1));

	cout << "test per_proto" << endl;

	sleep(2);

	pp.send(PI_ID_FTP, &m);

	sleep(1);
	return 0;
}