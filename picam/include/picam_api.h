#ifndef PICAM_API_H
#define PICAM_API_H

#include "picam_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

// Creates a new camera instance using specified configuration and parameters.
// Local version supplied only by picam_core library ignores address.
// Remote version supplied only by picam_client library uses address as follow:
//   - Should be in the form "[transport://]address:port"
//   - If transport is omitted, TCP is used.
int picam_create(picam_camera_t* camera, const picam_config_t* config, const char* address);

// Destroys a camera instance disconnecting.
int picam_destroy(picam_camera_t camera);

// Set callback that will be called every time a new frame is availabe.
int picam_callback_set(picam_camera_t camera, void* user_data, picam_callback_t callback);

// Get the current parameters used by camera.
int picam_params_get(picam_camera_t camera, picam_params_t* params);

// Update camera parameters to new value.
int picam_params_set(picam_camera_t camera, picam_params_t* params);

#ifdef __cplusplus
}
#endif

#endif // PICAM_API_H
