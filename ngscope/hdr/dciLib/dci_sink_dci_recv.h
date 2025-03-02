#ifndef DCI_SINK_DCI_RECV_HH
#define DCI_SINK_DCI_RECV_HH
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>

#include "dci_sink_ring_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif


int ngscope_dci_sink_recv_buffer(ngscope_dci_sink_CA_t* q, char* recvBuf, int idx, int recvLen);

#ifdef __cplusplus
}
#endif

#endif
