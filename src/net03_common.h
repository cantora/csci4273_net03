#ifndef NET03_COMMON_H
#define NET03_COMMON_H

#include <cstdio>
#include <cstdlib>

#ifdef NET03_DEBUG_LOG
#define NET03_LOG(fmt, ...) printf(fmt, ## __VA_ARGS__ )
#else
#define NET03_LOG(fmt, ...) ;
#endif

#define FATAL(s) printf("fatal error at %s(%d)", __FILE__, __LINE__); \
	if(s != NULL) printf(" (%s): ", s); else printf(": "); \
	perror(NULL); \
	exit(1);

namespace net03 {
	void set_nonblocking(int fd);
}

#endif /* NET03_COMMON_H */