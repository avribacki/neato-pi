#include "picam_server.h"

#include "picam_api.h"
#include "jaw_server.hpp"
#include "picam_protocol.hpp"

using namespace PiCam;

namespace Jaw {

/*************************************************************************************************/

using PiCamServer = Server<Command>;

/*************************************************************************************************/

template<>
const PiCamServer::Config& PiCamServer::config()
{
    static std::map<void*, picam_roi_t> crop_map;
    static Config PiCamServerConfig = {

        // Create Command
        { Command::CREATE, [](Handle& handle, InputBuffer args) {
            OutputBuffer reply;
            picam_config_t config;
            read(args, config);
            int error = picam_create(&handle.value, &config, nullptr);
            write(reply, error);
            return reply;
        }},

        // Destroy Command
        { Command::DESTROY, [](Handle& handle, InputBuffer) {
            OutputBuffer reply;
            int error = picam_destroy(handle.value);
            write(reply, error);
            return reply;
        }},

        // List other commands
        {
            { Command::CALLBACK_SET, [this](Handle& handle, InputBuffer args) {
                OutputBuffer reply;
                bool enable;
                read(args, enable);
                int error = 0;

                static auto image_callback = [](void* user_data, picam_image_t* image)
                {
                    Handle* handle = static_cast<Handle*>(user_data);
                    OutputBuffer message;
                    auto found = crop_map.find(handle->value);
                    if (found == crop_map.end()) {
                        write(message, Command::CALLBACK_SET, *image);
                    } else {
                        write(message, Command::CALLBACK_SET, *image, found->second);
                    }
                    handle->publish(std::move(message));
                };

                if (enable) {
                    error = picam_callback_set(handle.value, &handle, image_callback);
                } else {
                    error = picam_callback_set(handle.value, nullptr, nullptr);
                }

                write(reply, error);
                return reply;
            }},

            { Command::PARAMETERS_GET, [](Handle& handle, InputBuffer) {
                OutputBuffer reply;
                picam_params_t params;
                int error = picam_params_get(handle.value, &params);
                write(reply, error, params);
                return reply;
            }},

            { Command::PARAMETERS_SET, [](Handle& handle, InputBuffer args) {
                OutputBuffer reply;
                picam_params_t params;
                read(args, params);
                // Highjack crop parameter to avoid extra copies of image buffer.
                if (params.crop.x != 0.0 || params.crop.y != 0.0 ||
                        params.crop.width != 1.0 || params.crop.height != 1.0) {
                    crop_map[handle.value] = params.crop;
                    params.crop = { 0.0, 0.0, 1.0, 1.0 };
                }
                int error = picam_params_set(handle.value, &params);
                write(reply, error);
                return reply;
            }},
        }
    };

    return PiCamServerConfig;
}

/*************************************************************************************************/

} // end namespace Jaw

/*************************************************************************************************/

int picam_server_start(picam_server_t* server, const char* address)
{
    return Jaw::PiCamServer::start(server, address);
}

/*************************************************************************************************/

int picam_server_stop(picam_server_t server)
{
    return Jaw::PiCamServer::stop(server);
}

/*************************************************************************************************/
