#ifndef NEATO_API_H
#define NEATO_API_H

#include "neato_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

// Creates a new robot instance using specified configuration.
// Local version supplied only by neato_core library ignores address.
// Remote version supplied only by neato_client library uses address as follow:
//   - Should be in the form "[transport://]address:port"
//   - If transport is omitted, TCP is used.
int neato_create(neato_robot_t* robot, const neato_config_t* config, const char* address);

// Destroys a robot instance disconnecting.
int neato_destroy(neato_robot_t robot);

// Get the current robot pose.
int neato_pose_get(neato_robot_t robot, neato_pose_t* pose);

// Executes a laser scan and returns the result.
int neato_laser_scan_get(neato_robot_t robot, neato_laser_data_t* laser);

// Changes the current robot speed in millimeters per second.
int neato_speed_set(neato_robot_t robot, double speed);

// Returns true if robot is still performing a change in the heading.
int neato_is_heading_done(neato_robot_t robot, bool* done);

// Changes the robot heading by delta degrees.
int neato_delta_heading_set(neato_robot_t robot, double delta);

#ifdef __cplusplus
}
#endif

#endif // NEATO_API_H
