#ifndef PICAM_CAMERA_H
#define PICAM_CAMERA_H

#include "picam_defines.h"
#include <memory>

namespace PiCam {

/*************************************************************************************************/

// Access camera connected to Neato Robot.
class Camera
{
public:
    // Construct camera using supplied configuration.
    // Start capturing right away.
    Camera(const picam_config_t& config);

    // Close camera and release resources.
    ~Camera();

    // Set callback that will be called every time a new frame is availabe.
    void set_callback(void* user_data, picam_callback_t callback);

    // Get current parameters.
    const picam_params_t& parameters();

    // Update parameters to new value.
    void set_parameters(const picam_params_t& params);

private:
    // Hides class implementation
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/*************************************************************************************************/

}

#endif // PICAM_CAMERA_H
