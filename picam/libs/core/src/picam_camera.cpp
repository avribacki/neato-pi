#include "picam_camera.hpp"
#include "picam_camera_impl.hpp"

#include <iostream>

namespace PiCam {

/*************************************************************************************************/

Camera::Camera(const picam_config_t& config)
    : pimpl_(std::make_unique<Camera::Impl>(config))
{
    std::cout << "Created Camera" << std::endl;
}

/*************************************************************************************************/

Camera::~Camera()
{
    pimpl_.reset();
    std::cout << "Destroyed camera" << std::endl;
}

/*************************************************************************************************/

void Camera::set_callback(void* user_data, picam_callback_t callback)
{
    pimpl_->set_callback(user_data, callback);
}

/*************************************************************************************************/

const picam_params_t& Camera::parameters()
{
    return pimpl_->parameters();
}

/*************************************************************************************************/

void Camera::set_parameters(const picam_params_t& params)
{
    pimpl_->set_parameters(params);
}

/*************************************************************************************************/

}
