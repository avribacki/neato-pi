#ifndef PICAM_SERVER_H
#define PICAM_SERVER_H

#include "picam_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

// Define opaque neato server handle.
typedef void* picam_server_t;

// Start server to listen for requests on specified address.
// Initialize supplied handle that should be passed to stop method.
// Pass "*:port" to listen connections from all IP address using TCP on specified port.
int picam_server_start(picam_server_t* server, const char* address);

// Stop server
int picam_server_stop(picam_server_t server);

#ifdef __cplusplus
}
#endif

#endif // PICAM_SERVER_H
