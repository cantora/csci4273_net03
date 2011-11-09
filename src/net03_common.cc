#include "net03_common.h"

extern "C" {
#include <fcntl.h>
}

void net03::set_nonblocking(int fd) {
	int opts;

	opts = fcntl(fd,F_GETFL);
	if (opts < 0) {
		FATAL(NULL);
	}

	opts = (opts | O_NONBLOCK);
	if (fcntl(fd,F_SETFL,opts) < 0) {
		FATAL(NULL);
	}

	return;
}