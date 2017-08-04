#include "picam_api.h"

#include "jaw_client.hpp"
#include "picam_protocol.hpp"

using namespace Jaw;
using namespace PiCam;

/*************************************************************************************************/

using PiCamClient = Client<Command>;

/*************************************************************************************************/

// Maybe we want different times for each operation?
static std::chrono::seconds kTimeout = std::chrono::seconds(3);

/*************************************************************************************************/

int picam_create(picam_camera_t* camera, const picam_config_t* config, const char* address)
{
    if (!config) {
        return static_cast<int>(std::errc::invalid_argument);
    }

    return PiCamClient::create(camera, Command::CREATE, kTimeout, address, std::forward_as_tuple(*config));
}

/*************************************************************************************************/

int picam_destroy(picam_camera_t camera)
{
    return PiCamClient::destroy(camera, Command::DESTROY, kTimeout);
}

/*************************************************************************************************/

int picam_callback_set(picam_camera_t camera, void *user_data, picam_callback_t callback)
{
    return PiCamClient::set_callback(camera, Command::CALLBACK_SET, kTimeout,
        [user_data, callback](InputBuffer message) {
            picam_image_t image;
            read(message, image);
            callback(user_data, &image);
        });
}

/*************************************************************************************************/

int picam_params_get(picam_camera_t camera, picam_params_t *params)
{
    if (!params) {
        return static_cast<int>(std::errc::invalid_argument);
    }
    return PiCamClient::request(camera, Command::PARAMETERS_GET, kTimeout, *params);
}

/*************************************************************************************************/

int picam_params_set(picam_camera_t camera, picam_params_t* params)
{
    if (!params) {
        return static_cast<int>(std::errc::invalid_argument);
    }
    return PiCamClient::request(camera, Command::PARAMETERS_SET, kTimeout, std::forward_as_tuple(*params));
}

/*************************************************************************************************/
