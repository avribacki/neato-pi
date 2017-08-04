#include "picam_api.h"

#include <system_error>

#include "jaw_member_call.hpp"
#include "picam_camera.hpp"

using namespace PiCam;
using namespace Jaw;


/*************************************************************************************************/

int picam_create(picam_camera_t* camera, const picam_config_t* config, const char*)
{
    if (!camera || !config) {
        return static_cast<int>(std::errc::invalid_argument);
    }
    return protected_call([&camera, &config](){
        *camera = static_cast<picam_camera_t>(new Camera(*config));
        return 0;
    });
}

/*************************************************************************************************/

int picam_destroy(picam_camera_t camera)
{
    Camera* pcamera = static_cast<Camera*>(camera);
    if (pcamera == nullptr) {
        return static_cast<int>(std::errc::invalid_argument);
    }
    return protected_call([&pcamera]() {
        delete pcamera;
        return 0;
    });
}

/*************************************************************************************************/

int picam_callback_set(picam_camera_t camera, void* user_data, picam_callback_t callback)
{
    return member_call(camera, &Camera::set_callback, user_data, callback);
}

/*************************************************************************************************/

int picam_params_get(picam_camera_t camera, picam_params_t* params)
{
    return member_call(camera, &Camera::parameters, params);
}

/*************************************************************************************************/

int picam_params_set(picam_camera_t camera, picam_params_t* params)
{
    return member_call(camera, &Camera::set_parameters, *params);
}

/*************************************************************************************************/
