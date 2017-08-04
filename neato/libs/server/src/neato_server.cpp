#include "neato_server.h"

#include "neato_api.h"
#include "jaw_server.hpp"
#include "neato_protocol.hpp"

using namespace Neato;

namespace Jaw {

/*************************************************************************************************/

using NeatoServer = Server<Command>;

/*************************************************************************************************/

template<>
const NeatoServer::Config& NeatoServer::config()
{
    static Config NeatoServerConfig = {

        // Create Command
        { Command::CREATE, [](Handle& handle, InputBuffer args) {
            OutputBuffer reply;
            neato_config_t config;
            read(args, config);
            int error = neato_create(&handle.value, &config, nullptr);
            write(reply, error);
            return reply;
        }},

        // Destroy Command
        { Command::DESTROY, [](Handle& handle, InputBuffer) {
            OutputBuffer reply;
            int error = neato_destroy(handle.value);
            write(reply, error);
            return reply;
        }},

        // List other commands
        {
            { Command::POSE_GET, [](Handle& handle, InputBuffer) {
                OutputBuffer reply;
                neato_pose_t pose;
                int error = neato_pose_get(handle.value, &pose);
                write(reply, error, pose);
                return reply;
            }},

            { Command::LASER_SCAN_GET, [](Handle& handle, InputBuffer) {
                OutputBuffer reply;
                neato_laser_data_t laser_data;
                int error = neato_laser_scan_get(handle.value, &laser_data);
                write(reply, error, laser_data);
                return reply;
            }},

            { Command::SPEED_SET, [](Handle& handle, InputBuffer args) {
                OutputBuffer reply;
                double speed;
                read(args, speed);
                int error = neato_speed_set(handle.value, speed);
                write(reply, error);
                return reply;
            }},

            { Command::IS_HEADING_DONE, [](Handle&, InputBuffer) {
                OutputBuffer reply;
                write(reply, 0);
                return reply;
            }},

            { Command::DELTA_HEADING_SET, [](Handle& handle, InputBuffer args) {
                OutputBuffer reply;
                double delta;
                read(args, delta);
                int error = neato_delta_heading_set(handle.value, delta);
                write(reply, error);
                return reply;
            }},
        }
    };

    return NeatoServerConfig;
}

/*************************************************************************************************/

} // end namespace Jaw

/*************************************************************************************************/

int neato_server_start(neato_server_t* server, const char* address)
{
    return Jaw::NeatoServer::start(server, address);
}

/*************************************************************************************************/

int neato_server_stop(neato_server_t server)
{
    return Jaw::NeatoServer::stop(server);
}

/*************************************************************************************************/
