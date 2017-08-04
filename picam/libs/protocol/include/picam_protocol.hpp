#ifndef PICAM_PROTOCOL_H
#define PICAM_PROTOCOL_H

#include <cmath>

#include "picam_defines.h"
#include "jaw_serialization.hpp"

namespace PiCam {

/**************************************************************************************************
 * Commands *
 *************************************************************************************************/

enum class Command
{
    CREATE = 0,
    DESTROY,
    CALLBACK_SET,
    PARAMETERS_GET,
    PARAMETERS_SET,
};

/*************************************************************************************************/

}

// Serialization should be placed under Jaw namespace.
namespace Jaw {

/**************************************************************************************************
 * Serialization *
 *************************************************************************************************/

template<class... Args>
void write(OutputBuffer& buffer, const picam_config_t& value, const Args&... args)
{
    write(buffer, value.format, value.width, value.height, value.framerate);
    write(buffer, args...);
}

template<class... Args>
void read(InputBuffer& buffer, picam_config_t& value, Args&... args)
{
    read(buffer, value.format, value.width, value.height, value.framerate);
    read(buffer, args...);
}

/*************************************************************************************************/

template<class... Args>
void write(OutputBuffer& buffer, const picam_image_t& value, const Args&... args)
{
    // Write image meta-data
    write(buffer, value.format, value.width, value.height, value.bytes_per_line, value.data_size);
    // Write the image bufer
    buffer.write(value.data, value.data_size);

    write(buffer, args...);
}

template<class... Args>
void write(OutputBuffer& buffer, const picam_image_t& value, const picam_roi_t& crop, const Args&... args)
{
    const int new_width = static_cast<int>(std::round(value.width * crop.width));
    const int new_height = static_cast<int>(std::round(value.width * crop.width));
    const int bytes_per_pixel = value.bytes_per_line / value.width;
    const int new_bytes_per_line =  bytes_per_pixel * new_width;
    const int new_data_size = new_bytes_per_line * new_height;
    const int new_x = static_cast<int>(std::round(value.width * crop.x));
    const int new_y = static_cast<int>(std::round(value.height * crop.y));

    // Write image meta-data
    write(buffer, value.format, new_width, new_height, new_bytes_per_line, new_data_size);

    // Write the image bufer
    for (int j = new_y; j < new_y + new_height; j++) {
        buffer.write(value.data + bytes_per_pixel * (new_x + j * value.width), new_bytes_per_line);
    }

    write(buffer, args...);
}

// WARNING: The image buffer is *not* copied from the InputBuffer
// If the neato_image_t outlives the buffer, a manual copy will be required.
template<class... Args>
void read(InputBuffer& buffer, picam_image_t& value, Args&... args)
{
    // Read image meta-data
    read(buffer, value.format, value.width, value.height, value.bytes_per_line, value.data_size);
    // Get the memory position of the buffer inside the InputBuffer.
    value.data = static_cast<unsigned char*>(buffer.read(0));

    read(buffer, args...);
}

/**************************************************************************************************/

template<class... Args>
void write(OutputBuffer& buffer, const picam_roi_t& value, const Args&... args)
{
    write(buffer, value.x, value.y, value.width, value.height);
    write(buffer, args...);
}

template<class... Args>
void read(InputBuffer& buffer, picam_roi_t& value, Args&... args)
{
    read(buffer, value.x, value.y, value.width, value.height);
    read(buffer, args...);
}

/**************************************************************************************************/

template<class... Args>
void write(OutputBuffer& buffer, const picam_params_t& value, const Args&... args)
{
    write(buffer,
          value.sharpness,
          value.contrast,
          value.brightness,
          value.saturation,
          value.exposure_compensation,
          value.zoom,
          value.crop);
    write(buffer, args...);
}

template<class... Args>
void read(InputBuffer& buffer, picam_params_t& value, Args&... args)
{
    read(buffer,
         value.sharpness,
         value.contrast,
         value.brightness,
         value.saturation,
         value.exposure_compensation,
         value.zoom,
         value.crop);
    read(buffer, args...);
}

/**************************************************************************************************/

}

#endif // PICAM_PROTOCOL_H
