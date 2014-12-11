#ifndef _WINSOCK_EXTRAS_H_
#define _WINSOCK_EXTRAS_H_

char *getenv(const char *var);

struct servent* getservbyname(const char* /*name*/, const char* /*proto*/);

#endif /* _WINSOCK_EXTRAS_H_ */