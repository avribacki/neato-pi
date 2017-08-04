#include "picam_camera_dummy.hpp"

#include <vector>
#include <algorithm>
#include <random>
#include <iostream>
#include <cstdint>
#include <cstring>

namespace PiCam {

/*************************************************************************************************/

Camera::Impl::Impl(const picam_config_t& config)
    : config_(config)
    , params_()
    , user_data_(nullptr)
    , user_callback_(nullptr)
    , user_mutex_()
    , keep_running_(true)
    , main_thread_(&Impl::main_loop, this)
{
    params_.sharpness = 0;
    params_.contrast = 0;
    params_.brightness = 50;
    params_.saturation = 0;
    params_.exposure_compensation = 0;
    params_.crop = { 0.f, 0.f, 1.f, 1.f };
    params_.zoom = { 0.f, 0.f, 1.f, 1.f };
}

/*************************************************************************************************/

Camera::Impl::~Impl()
{
    keep_running_ = false;
    main_thread_.join();
}

/*************************************************************************************************/

void Camera::Impl::set_callback(void* user_data, picam_callback_t callback)
{
    std::lock_guard<std::mutex> lock(user_mutex_);
    user_data_ = user_data;
    user_callback_ = callback;
}

/*************************************************************************************************/

const picam_params_t& Camera::Impl::parameters()
{
    return params_;
}

/*************************************************************************************************/

void Camera::Impl::set_parameters(const picam_params_t& params)
{
    params_ = params;
}

/*************************************************************************************************/

void Camera::Impl::main_loop()
{
    using namespace std::chrono;

    // Create image that will be randonly filled.
    picam_image_t image;
    image.format = config_.format;
    image.width = config_.width;
    image.height = config_.height;

    std::vector<unsigned char> buffer;

    switch (config_.format) {
        case PICAM_IMAGE_FORMAT_GRAY: {
            buffer.resize(image.width * image.height);
            image.bytes_per_line = image.width;

            // Determine how large the gradient stripes will be
            int col_width = image.width / 10;

            // Image line that will be repeated
            unsigned char line[image.bytes_per_line];

            for (int j = 0; j < image.width; j++) {
                line[j] = 255 * (j % col_width) / (double) col_width;
            }
            for (int i = 0; i < image.height; i++) {
                std::memcpy(buffer.data() + i * image.bytes_per_line, line, image.bytes_per_line);
            }
            break;
        }
        case PICAM_IMAGE_FORMAT_RGB:
        case PICAM_IMAGE_FORMAT_BGR: {
            buffer.resize(image.width * image.height * 3);
            image.bytes_per_line = image.width * 3;

            // Image line that will be repeated.
            unsigned char line[image.bytes_per_line];

            int red = 0, green = 1, blue = 2;
            if (config_.format == PICAM_IMAGE_FORMAT_BGR) {
                std::swap(red, blue);
            }

            for (int j = 0; j < image.width; j++) {
                unsigned char r, g, b;
                double value = j / (double) image.width;

                // Calculate the hue based on a value between 0 and 1.0

                if (value < 0.3334) {
                    r = static_cast<unsigned char>(255.0 * (1.0 - value * 3.0));
                    g = static_cast<unsigned char>(255.0 * value * 3.0);
                    b = 0;
                }
                else if (value < 0.6667) {
                    r = 0;
                    g = static_cast<unsigned char>(255.0 * (0.6667 - value) * 3.0);
                    b = static_cast<unsigned char>(255.0 * (value - 0.3334) * 3.0);
                }
                else {
                    r = static_cast<unsigned char>(255.0 * (value - 0.6667) * 3.0);
                    g = 0;
                    b = static_cast<unsigned char>(255.0 * (1.0 - value) * 3.0);
                }

                line[j * 3 + red] = std::min<unsigned char>(127, r) * 2;
                line[j * 3 + green] = std::min<unsigned char>(127, g) * 2;
                line[j * 3 + blue] = std::min<unsigned char>(127, b) * 2;
            }
            for (int i = 0; i < image.height; i++) {
                std::memcpy(buffer.data() + i * image.bytes_per_line, line, image.bytes_per_line);
            }
            break;
        }
        default:
            std::cout << "Invalid image format..." << std::endl;
            break;
    }

    image.data_size = static_cast<unsigned int>(buffer.size());
    image.data = &buffer[0];

    // Interval to sleep derived from framerate.
    microseconds interval = microseconds(static_cast<int64_t>((1.0 / config_.framerate) * 1.0e6));
    auto wake_at = steady_clock::now();

    // Determine how many cols we shift per frame based on the framerate.
    const unsigned int bits_per_pixel = image.bytes_per_line / image.width;
    const unsigned int shift = bits_per_pixel * 60.0 / std::min(config_.framerate, 60.0);

    while (keep_running_) {

        unsigned char temp[shift];

        for (int i = 0; i < image.height; i++) {
            unsigned char* line = buffer.data() + i * image.bytes_per_line;
            std::memcpy(temp, line, shift);
            std::memmove(line, line + shift, image.bytes_per_line - shift);
            std::memcpy(line + image.bytes_per_line - shift, temp, shift);
        }

        std::lock_guard<std::mutex> lock(user_mutex_);
        if (user_callback_ ) {
            user_callback_(user_data_, &image);
        }

        wake_at += interval;
        std::this_thread::sleep_until(wake_at);
    }
}

/*************************************************************************************************/

}
