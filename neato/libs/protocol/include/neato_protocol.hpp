#ifndef NEATO_PROTOCOL_H
#define NEATO_PROTOCOL_H

#include "neato_defines.h"

#include "jaw_serialization.hpp"

namespace Neato {

/**************************************************************************************************
 * Commands *
 *************************************************************************************************/

enum class Command
{
    CREATE = 0,
    DESTROY,
    POSE_GET,
    LASER_SCAN_GET,
    SPEED_SET,
    IS_HEADING_DONE,
    DELTA_HEADING_SET,
};

/*************************************************************************************************/

}

// Serialization should be placed under Jaw namespace.
namespace Jaw {

/**************************************************************************************************
 * Serialization *
 *************************************************************************************************/

template<class... Args>
void write(OutputBuffer& buffer, const neato_config_t& value, const Args&... args)
{
    write(buffer, value.update_interval_ms);
    write(buffer, args...);
}


template<class... Args>
void read(InputBuffer& buffer, neato_config_t& value, Args&... args)
{
    read(buffer, value.update_interval_ms);
    read(buffer, args...);
}

/*************************************************************************************************/

template<class... Args>
void write(OutputBuffer& buffer, const neato_pose_t& value, const Args&... args)
{
    write(buffer, value.x, value.y, value.theta);
    write(buffer, args...);
}

template<class... Args>
void read(InputBuffer& buffer, neato_pose_t& value, Args&... args)
{
    read(buffer, value.x, value.y, value.theta);
    read(buffer, args...);
}

/*************************************************************************************************/

template<class... Args>
void write(OutputBuffer& buffer, const neato_laser_data_t& value, const Args&... args)
{
    write(buffer, value.pose_taken, value.distance);
    write(buffer, args...);
}

template<class... Args>
void read(InputBuffer& buffer, neato_laser_data_t& value, Args&... args)
{
    read(buffer, value.pose_taken, value.distance);
    read(buffer, args...);
}

/**************************************************************************************************/

}

#endif // NEATO_PROTOCOL_H
