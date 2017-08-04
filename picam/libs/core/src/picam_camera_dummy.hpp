#ifndef PICAM_CAMERA_IMPL_H
#define PICAM_CAMERA_IMPL_H

#include "picam_camera.hpp"

#include <thread>
#include <mutex>
#include <atomic>

namespace PiCam {

/*************************************************************************************************/

// Dummy camera implementation to use when MMAL is not found.
class Camera::Impl
{
public:
    // Construct camera using supplied configuration.
    Impl(const picam_config_t& config);

    // Stop capturing.
    ~Impl();

    // Set callback that will be called every time a new frame is availabe.
    void set_callback(void* user_data, picam_callback_t callback);

    // Get current parameters.
    const picam_params_t& parameters();

    // Update parameters to new value.
    void set_parameters(const picam_params_t& params);

private:

    // Loop that will periodically provide random images.
    void main_loop();

    // Configuration supplied during construction.
    picam_config_t config_;

    // Parameters for image. Ignore by this dummy implementation.
    picam_params_t params_;

    // Callback set by the user and associated mutex.
    void* user_data_;
    picam_callback_t user_callback_;
    std::mutex user_mutex_;

    // Used to indicate if thread should keep running or stop.
    std::atomic<bool> keep_running_;

    // Threads responsible for asynchronous execution.
    std::thread main_thread_;
};

/*************************************************************************************************/

}

#endif //PICAM_CAMERA_IMPL_H
