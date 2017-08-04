#include "neato_api.h"

#include "jaw_client.hpp"
#include "neato_protocol.hpp"

using namespace Jaw;
using namespace Neato;

/*************************************************************************************************/

using NeatoClient = Client<Command>;

/*************************************************************************************************/

// Maybe we want different times for each operation?
static std::chrono::seconds kTimeout = std::chrono::seconds(3);

/*************************************************************************************************/

int neato_create(neato_robot_t* robot, const neato_config_t* config, const char* address)
{
    if (!config) {
        return static_cast<int>(std::errc::invalid_argument);
    }
    return NeatoClient::create(robot, Command::CREATE, kTimeout, address, std::forward_as_tuple(*config));
}

/*************************************************************************************************/

int neato_destroy(neato_robot_t robot)
{
    return NeatoClient::destroy(robot, Command::DESTROY, kTimeout);
}

/*************************************************************************************************/

int neato_pose_get(neato_robot_t robot, neato_pose_t* pose)
{
    if (!pose) {
        return static_cast<int>(std::errc::invalid_argument);
    }
    return NeatoClient::request(robot, Command::POSE_GET, kTimeout, *pose);
}

/*************************************************************************************************/

int neato_laser_scan_get(neato_robot_t robot, neato_laser_data_t* laser_data)
{
    if (!laser_data) {
        return static_cast<int>(std::errc::invalid_argument);
    }
    return NeatoClient::request(robot, Command::LASER_SCAN_GET, kTimeout, *laser_data);
}

/*************************************************************************************************/

int neato_speed_set(neato_robot_t robot, double speed)
{
    return NeatoClient::request(robot, Command::SPEED_SET, kTimeout, std::forward_as_tuple(speed));
}

/*************************************************************************************************/

int neato_is_heading_done(neato_robot_t robot, bool* done)
{
    return 0;
}

/*************************************************************************************************/

int neato_delta_heading_set(neato_robot_t robot, double delta)
{
    return NeatoClient::request(robot, Command::DELTA_HEADING_SET, kTimeout, std::forward_as_tuple(delta));
}

/*************************************************************************************************/
