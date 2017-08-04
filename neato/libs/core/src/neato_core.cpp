#include "neato_api.h"

#include <memory>
#include <iostream>
#include <exception>
#include <functional>

#include "jaw_member_call.hpp"
#include "neato_robot.hpp"

using namespace Neato;
using namespace Jaw;

/*************************************************************************************************/

int neato_create(neato_robot_t* robot, const neato_config_t* config, const char*)
{
    if (!robot || !config) {
        return static_cast<int>(std::errc::invalid_argument);
    }
    return protected_call([&robot, &config](){
        *robot = static_cast<neato_robot_t>(new Robot(*config));
        return 0;
    });
}

/*************************************************************************************************/

int neato_destroy(neato_robot_t robot)
{
    Robot* probot = static_cast<Robot*>(robot);
    if (probot == nullptr) {
        return static_cast<int>(std::errc::invalid_argument);
    }
    return protected_call([&probot]() {
        delete probot;
        return 0;
    });
}

/*************************************************************************************************/

int neato_pose_get(neato_robot_t robot, neato_pose_t* pose)
{
    return member_call(robot, &Robot::get_pose, pose);
}

/*************************************************************************************************/

int neato_laser_scan_get(neato_robot_t robot, neato_laser_data_t* laser_data)
{
    return member_call(robot, &Robot::get_laser_scan, laser_data);
}

/*************************************************************************************************/

int neato_speed_set(neato_robot_t robot, double speed)
{
    return member_call(robot, &Robot::set_speed, speed);
}

/*************************************************************************************************/

int neato_is_heading_done(neato_robot_t robot, bool* done)
{
    return member_call(robot, &Robot::is_heading_done, done);
}

/*************************************************************************************************/

int neato_delta_heading_set(neato_robot_t robot, double delta)
{
    return member_call(robot, &Robot::set_delta_heading, delta);
}

/*************************************************************************************************/
