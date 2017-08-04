#ifndef NEATO_SERVER_H
#define NEATO_SERVER_H

#include "neato_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

// Define opaque neato server handle.
typedef void* neato_server_t;

// Start server to listen for requests on specified address.
// Initialize supplied handle that should be passed to stop method.
// Pass "*:port" to listen connections from all IP address using TCP on specified port.
int neato_server_start(neato_server_t* server, const char* address);

// Stop server
int neato_server_stop(neato_server_t server);

#ifdef __cplusplus
}
#endif

#endif // NEATO_SERVER_H
