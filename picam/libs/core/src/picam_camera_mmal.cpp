#include "picam_camera_mmal.hpp"

#include "bcm_host.h"
#include "interface/vmcs_host/vc_vchi_gencmd.h"
#include "jaw_exception.hpp"

#include <iostream>
#include <cmath>

using namespace Jaw;

namespace PiCam {

/*************************************************************************************************/

// Standard port setting for the camera component
#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2

/*************************************************************************************************/

// Checks if MMAL operation was successful, throwing an exception otherwise.
static void assert_mmal_status(MMAL_STATUS_T status, std::string message)
{
    if (status == MMAL_SUCCESS)
        return;

    switch (status) {
    case MMAL_ENOMEM:
        throw Exception(std::errc::not_enough_memory, "[MMAL_ENOMEM] " + message);
    case MMAL_ENOSPC:
        throw Exception(std::errc::no_space_on_device, "[MMAL_ENOSPC] " + message);
    case MMAL_EINVAL:
        throw Exception(std::errc::invalid_argument, "[MMAL_EINVAL] " + message);
    case MMAL_ENOSYS:
        throw Exception(std::errc::function_not_supported, "[MMAL_ENOSYS] " + message);
    case MMAL_ENOENT:
        throw Exception(std::errc::no_such_file_or_directory, "[MMAL_ENOENT] " + message);
    case MMAL_ENXIO:
        throw Exception(std::errc::no_such_device_or_address, "[MMAL_ENXIO] " + message);
    case MMAL_EIO:
        throw Exception(std::errc::io_error, "[MMAL_EIO] " + message);
    case MMAL_ESPIPE:
        throw Exception(std::errc::invalid_seek, "[MMAL_ESPIPE] " + message);
    case MMAL_EISCONN:
        throw Exception(std::errc::already_connected, "[MMAL_EISCONN] " + message);
    case MMAL_ENOTCONN:
        throw Exception(std::errc::not_connected, "[MMAL_ENOTCONN] " + message);
    case MMAL_EAGAIN:
        throw Exception(std::errc::resource_unavailable_try_again, "[MMAL_EAGAIN] " + message);
    case MMAL_EFAULT:
        throw Exception(std::errc::bad_address, "[MMAL_EFAULT] " + message);

    // NOT POSIX! Returning different error.
    case MMAL_ECORRUPT:
        throw Exception(std::errc::state_not_recoverable, "[MMAL_ECORRUPT | Data is corrupt] " + message);
    case MMAL_ENOTREADY:
        throw Exception(std::errc::state_not_recoverable, "[MMAL_ENOTREADY | Component is not ready] " + message);
    case MMAL_ECONFIG:
        throw Exception(std::errc::state_not_recoverable, "[MMAL_ECONFIG | Component is not configured] " + message);

    default:
        throw Exception(std::errc::state_not_recoverable, "[MMAL_UNKNOWN] " + message);
   }
}

/*************************************************************************************************/

int mmal_encoding_from_image_format(picam_image_format_t format)
{
    // Note: In Camera Module v2 the formats RGB and BGR are no longer inverted.
    switch (format) {
    case PICAM_IMAGE_FORMAT_GRAY:
        return MMAL_ENCODING_I420;
    case PICAM_IMAGE_FORMAT_RGB:
        return MMAL_ENCODING_RGB24;
    case PICAM_IMAGE_FORMAT_BGR:
        return MMAL_ENCODING_BGR24;
    default:
        throw Exception(std::errc::invalid_argument, "Invalid image format");
    }
}

/*************************************************************************************************/

Camera::Impl::Impl(const picam_config_t& config)
    : config_(config)
    , user_data_(nullptr)
    , callback_(nullptr)
    , callback_work_()
    , image_()
    , picam_parameters_()
    , mmal_parameters_()
    , camera_component_(nullptr)
    , video_pool_(nullptr)
    , video_port_(nullptr)
    , mutex_()
{
    // Image meta-data depends on config which cannot be changed after camera is created.
    // Initialize those parameters here and update just image data before passing to callback.
    image_.format = config_.format;
    image_.width = config_.width;
    image_.height = config_.height;
    image_.bytes_per_line = image_.width * (image_.format == PICAM_IMAGE_FORMAT_GRAY ? 1 : 3);
    image_.data_size = image_.height * image_.bytes_per_line;

    // Will be set during video_port_callback with MMAL buffer.
    image_.data = nullptr;

    MMAL_STATUS_T status;

    std::lock_guard<std::recursive_mutex> lock(mutex_);

    try {
        bcm_host_init();

        status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera_component_);
        assert_mmal_status(status, "Failed to create camera component");

        if (camera_component_->output_num == 0) {
            throw Exception(std::errc::state_not_recoverable, "Camera doesn't have output ports");
        }

        // Set up the camera configuration.

        MMAL_PARAMETER_CAMERA_CONFIG_T cam_config;
        cam_config.hdr.id = MMAL_PARAMETER_CAMERA_CONFIG;
        cam_config.hdr.size = sizeof(cam_config);
        cam_config.max_stills_w = config_.width;
        cam_config.max_stills_h = config_.height;
        cam_config.stills_yuv422 = 0;
        cam_config.one_shot_stills = 0;
        cam_config.max_preview_video_w = config_.width;
        cam_config.max_preview_video_h = config_.height;
        cam_config.num_preview_video_frames = 3;
        cam_config.stills_capture_circular_buffer_height = 0;
        cam_config.fast_preview_resume = 0;
        cam_config.use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC;

        status = mmal_port_parameter_set(camera_component_->control, &cam_config.hdr);
        assert_mmal_status(status, "Failed to set camera parameters");

        // Set the encode format on the video  port

        MMAL_FOURCC_T encoding = mmal_encoding_from_image_format(config_.format);

        video_port_ = camera_component_->output[MMAL_CAMERA_VIDEO_PORT];

        MMAL_ES_FORMAT_T* format = video_port_->format;
        format->encoding_variant = encoding;
        format->encoding = encoding;
        format->es->video.width = VCOS_ALIGN_UP(config_.width, 32);
        format->es->video.height = VCOS_ALIGN_UP(config_.height, 16);
        format->es->video.crop.x = 0;
        format->es->video.crop.y = 0;
        format->es->video.crop.width = config_.width;
        format->es->video.crop.height = config_.height;

        // Framerate is a rough approximation only.
        if (config_.framerate >= 1.0) {
            format->es->video.frame_rate.num = std::round(config_.framerate);
            format->es->video.frame_rate.den = 1;
        } else {
            format->es->video.frame_rate.num = 1;
            format->es->video.frame_rate.den = std::round(1.0 / config_.framerate);
        }

        std::cout << "Framerate " << format->es->video.frame_rate.num << " / " << format->es->video.frame_rate.den << std::endl; 
        status = mmal_port_format_commit(video_port_);
        assert_mmal_status(status, "Camera video format couldn't be set");

        // Enable component.
        status = mmal_component_enable(camera_component_);
        assert_mmal_status(status, "Camera component couldn't be enabled");

        // Set camera parameters.
        update_camera_parameters(mmal_parameters_, true);
        mmal_parameters_.export_to(picam_parameters_);

        // Default is to not crop the image.
	picam_parameters_.crop = { 0.0, 0.0, 1.0, 1.0 };

        // Ensure there are enough buffers to avoid dropping frames.
        video_port_->buffer_size = video_port_->buffer_size_recommended;
        video_port_->buffer_num = std::max<uint32_t>(3, video_port_->buffer_num_recommended);

        video_pool_ = mmal_port_pool_create(video_port_, video_port_->buffer_num, video_port_->buffer_size);
        if (!video_pool_) {
            throw Exception(std::errc::state_not_recoverable, "Failed to create buffer header pool for video output port");
        }

        // Set this instance as user data for the callback.
        video_port_->userdata = reinterpret_cast<struct MMAL_PORT_USERDATA_T*>(this);

        // Enable video port.
        status = mmal_port_enable(video_port_, &Impl::video_port_callback);
        assert_mmal_status(status, "Failed to enable video port.");

        // Send all the buffers to the camera video port
        int num_buffers = mmal_queue_length(video_pool_->queue);
        for (int i = 0; i < num_buffers; i++)
        {
            MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(video_pool_->queue);
            if (!buffer) {
                throw Exception(std::errc::no_buffer_space,
                        "Unable to get a required buffer " + std::to_string(i) + " from pool queue");
            }
            status = mmal_port_send_buffer(video_port_, buffer);
            assert_mmal_status(status, "Unable to send a buffer to camera video port");
        }

    } catch (...) {

        // Destroy component if something went wrong.
        if (camera_component_) {
            mmal_component_destroy(camera_component_);
        }

        // See if issue might be related to hardware problem.
        check_hardware();

        // Throw original exception otherwise.
        throw;
    }
}

/*************************************************************************************************/

Camera::Impl::~Impl()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (video_pool_) {
        if (video_port_->is_enabled ) {
            mmal_port_disable(video_port_);
        }
        if (video_pool_) {
            mmal_port_pool_destroy(video_port_, video_pool_);
        }
    }
    if (camera_component_) {
        mmal_component_disable(camera_component_);
        mmal_component_destroy(camera_component_);
    }
}

/*************************************************************************************************/

void Camera::Impl::set_callback(void* user_data, picam_callback_t callback)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (callback) {
        // Start capturing.
        MMAL_STATUS_T status = mmal_port_parameter_set_boolean(video_port_, MMAL_PARAMETER_CAPTURE, true);
        assert_mmal_status(status, "Failed to start capturing on video port");

        // Save user data and callback.
        user_data_ = user_data;
        callback_ = callback;
    }  else {
        // Disable capture
        MMAL_STATUS_T status = mmal_port_parameter_set_boolean(video_port_, MMAL_PARAMETER_CAPTURE, false);
        assert_mmal_status(status, "Failed to stop capturing on video port");
        user_data_ = nullptr;
        callback_ = nullptr;
    }
}

/*************************************************************************************************/

const picam_params_t& Camera::Impl::parameters()
{
    // Returning a reference might break thread-safety...
    // Nevertheless, as it is a const reference, only read operations are allowed.
    return picam_parameters_;
}

/*************************************************************************************************/

void Camera::Impl::set_parameters(const picam_params_t& params)
{
    // Save desired values to temporary parameters.
    static Parameters temp_params(mmal_parameters_);
    temp_params.import_from(params);

    std::lock_guard<std::recursive_mutex> lock(mutex_);
    update_camera_parameters(temp_params, false);
}

/*************************************************************************************************/

void Camera::Impl::video_port_callback(MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer)
{
    // We passed a reference to this as userdata.
    Impl* camera = (Impl*) port->userdata;
    if (!camera) {
        mmal_buffer_header_release(buffer);
        return;
    }

    // Callback was called for clean-up.
    if (!port->is_enabled) {
        if (camera->callback_work_.valid()) {
           MMAL_BUFFER_HEADER_T* old_buffer = camera->callback_work_.get();
           mmal_buffer_header_release(old_buffer);
        }
        mmal_buffer_header_release(buffer);
        return; 
    }

    // For the first frame the std::future is empty, no point checking it.
    if (!camera->callback_work_.valid()) {
        // Request callback execution and don't release this buffer.
        camera->callback_work_ = std::async(std::launch::async, &Camera::Impl::call_callback, camera, buffer);
        return;
    } else {
        // Verify if the last callback execution have finished.
        auto status = camera->callback_work_.wait_for(std::chrono::seconds(0));
        if (status == std::future_status::ready) {
            // Get buffer that was used on last execution.
            MMAL_BUFFER_HEADER_T* old_buffer = camera->callback_work_.get();
            camera->callback_work_ = std::async(std::launch::async, &Camera::Impl::call_callback, camera, buffer);

            // Set buffer to the one returned so we can release it and not the one that will be used by the callback.
            buffer = old_buffer;
        } else {
            std::cout << "Already running callback, skip buffer." << std::endl;
        }
    }

    // Release buffer back to the pool.
    mmal_buffer_header_release(buffer);

    // Send one back to the port.
    MMAL_STATUS_T status = MMAL_SUCCESS;
    MMAL_BUFFER_HEADER_T* new_buffer = mmal_queue_get(camera->video_pool_->queue);

    if (new_buffer) {
        status = mmal_port_send_buffer(port, new_buffer);
    }
    if (!new_buffer || status != MMAL_SUCCESS) {
        std::cerr << "Unable to return a buffer to the camera port" << std::endl;
    }
}

/*************************************************************************************************/

MMAL_BUFFER_HEADER_T* Camera::Impl::call_callback(MMAL_BUFFER_HEADER_T* buffer)
{
    // Tries to acquire lock to avoid changes in the camera during execution.
    std::unique_lock<std::recursive_mutex> lock(mutex_, std::try_to_lock);
    if (!lock) {
       return buffer;
    }

    if (callback_ && buffer->length >= image_.data_size) {
        mmal_buffer_header_mem_lock(buffer);

        try {
            image_.data = buffer->data;
            callback_(user_data_, &image_);
        } catch (...) {
            std::cerr << "Bad, bad boy. Callback triggered an exception." << std::endl;
        }

        mmal_buffer_header_mem_unlock(buffer);
    }

    return buffer;
}

/*************************************************************************************************/

void Camera::Impl::check_hardware()
{
    char response[80] = "";

    // Check how much memory is allocated to GPU.
    int gpu_mem = 0;
    if (vc_gencmd(response, sizeof response, "get_mem gpu") == 0) {
       vc_gencmd_number_property(response, "gpu", &gpu_mem);
    }

    // Check if camera is enabled and detected.
    int supported = 0, detected = 0;
    if (vc_gencmd(response, sizeof response, "get_camera") == 0) {
       vc_gencmd_number_property(response, "supported", &supported);
       vc_gencmd_number_property(response, "detected", &detected);
    }

    if (!supported) {
        throw Exception(std::errc::not_supported,
            "Camera is not enabled in this build");
    }
    if (gpu_mem < 128) {
        throw Exception(std::errc::not_enough_memory,
            "Only " + std::to_string(gpu_mem) + "M is configured. The minimum required is 128M");
    }
    if (!detected) {
        throw Exception(std::errc::no_such_device,
            "Camera is not detected. Please check carefully the camera module is installed correctly");
    }
}

/*************************************************************************************************/

// Helper to verify if two picam_roi_t are differents.
static bool operator != (const picam_roi_t& left, const picam_roi_t&right)
{
    return (left.x != right.x || left.y != right.y || left.width != right.width || left.height != right.height);
}

// Helper to verify if two MMAL_PARAM_COLOURFX_T are differents.
static bool operator != (const MMAL_PARAM_COLOURFX_T& left, const MMAL_PARAM_COLOURFX_T&right)
{
    return (left.enable != right.enable || left.u != right.u || left.v != right.v);
}

// Validate if all ROI parameters are normalized.
static bool is_valid_roi(const picam_roi_t& roi)
{
    return (roi.x >= 0.0 && roi.x <= 1.0 &&
            roi.y >= 0.0 && roi.y <= 1.0 &&
            roi.width >= 0.0 && roi.width <= 1.0 &&
            roi.height >= 0.0 && roi.height <= 1.0);
}

/*************************************************************************************************/

void Camera::Impl::update_camera_parameters(const Parameters& new_params, bool force)
{
    if (camera_component_ == nullptr) {
        throw Exception(std::errc::operation_not_permitted, "Can't set parameters without component");
    }

    MMAL_COMPONENT_T* camera = camera_component_;
    Parameters& params = mmal_parameters_;
    bool debug = !force;

    try {
        // Update sharpness
        if (force || params.sharpness != new_params.sharpness) {
            if (new_params.sharpness < -100 || new_params.sharpness > 100) {
                throw Exception(std::errc::invalid_argument, "Invalid sharpness value");
            }
            if (debug) {
                std::cout << "Setting sharpness to " << new_params.sharpness << std::endl;
            }
            MMAL_RATIONAL_T value = { new_params.sharpness, 100 };
            MMAL_STATUS_T status = mmal_port_parameter_set_rational(camera->control, MMAL_PARAMETER_SHARPNESS, value);
            assert_mmal_status(status, "Failed to set camera sharpness");

            params.sharpness = new_params.sharpness;
        }

        // Update contrast
        if (force || params.contrast != new_params.contrast) {
            if (new_params.contrast < -100 || new_params.contrast > 100) {
                throw Exception(std::errc::invalid_argument, "Invalid contrast value");
            }
            if (debug) {
                std::cout << "Setting contrast to " << new_params.contrast << std::endl;
            }
            MMAL_RATIONAL_T value = { new_params.contrast, 100 };
            MMAL_STATUS_T status = mmal_port_parameter_set_rational(camera->control, MMAL_PARAMETER_CONTRAST, value);
            assert_mmal_status(status, "Failed to set camera contrast");

            params.contrast = new_params.contrast;
        }

        // Update brightness
        if (force || params.brightness != new_params.brightness) {
            if (new_params.brightness < 0 || new_params.contrast > 100) {
                throw Exception(std::errc::invalid_argument, "Invalid brightness value");
            }
            if (debug) {
                std::cout << "Setting brightness to " << new_params.brightness << std::endl;
            }
            MMAL_RATIONAL_T value = { new_params.brightness, 100 };
            MMAL_STATUS_T status = mmal_port_parameter_set_rational(camera->control, MMAL_PARAMETER_BRIGHTNESS, value);
            assert_mmal_status(status, "Failed to set camera brightness");

            params.brightness = new_params.brightness;
        }

        // Update saturation
        if (force || params.saturation != new_params.saturation) {
            if (new_params.saturation < -100 || new_params.saturation > 100) {
                throw Exception(std::errc::invalid_argument, "Invalid saturation value");
            }
            if (debug) {
                std::cout << "Setting saturation to " << new_params.saturation << std::endl;
            }
            MMAL_RATIONAL_T value = { new_params.saturation, 100 };
            MMAL_STATUS_T status = mmal_port_parameter_set_rational(camera->control, MMAL_PARAMETER_SATURATION, value);
            assert_mmal_status(status, "Failed to set camera saturation");

            params.saturation = new_params.saturation;
        }

        // Update ISO
        if (force || params.iso != new_params.iso) {

            // TODO : Check possible values.
            if (debug) {
                std::cout << "Setting ISO to " << new_params.iso << std::endl;
            }
            MMAL_STATUS_T status = mmal_port_parameter_set_uint32(camera->control, MMAL_PARAMETER_ISO, new_params.iso);
            assert_mmal_status(status, "Failed to set camera ISO");

            params.iso = new_params.iso;
        }

        // Update video stabilisation
        if (force || params.video_stabilisation != new_params.video_stabilisation) {
            if (new_params.video_stabilisation != 0 && new_params.video_stabilisation != 1) {
                throw Exception(std::errc::invalid_argument, "Invalid video stabilisation value. Expected 0 or 1");
            }
            if (debug) {
                std::cout << "Setting video stabilisation to " << new_params.video_stabilisation << std::endl;
            }
            MMAL_STATUS_T status = mmal_port_parameter_set_boolean(
                    camera->control, MMAL_PARAMETER_VIDEO_STABILISATION, new_params.video_stabilisation);
            assert_mmal_status(status, "Failed to set camera video stabilisation");

            params.video_stabilisation = new_params.video_stabilisation;
        }

        // Update exposure compensation
        if (force || params.exposure_compensation != new_params.exposure_compensation) {
            if (new_params.exposure_compensation < -25 || new_params.exposure_compensation > 25) {
                throw Exception(std::errc::invalid_argument, "Invalid exposure compensation value");
            }
            if (debug) {
                std::cout << "Setting exposure compensation to " << new_params.exposure_compensation << std::endl;
            }
            MMAL_STATUS_T status = mmal_port_parameter_set_int32(
                    camera->control, MMAL_PARAMETER_EXPOSURE_COMP , new_params.exposure_compensation);
            assert_mmal_status(status, "Failed to set exposure compensation saturation");

            params.exposure_compensation = new_params.exposure_compensation;
        }

        // Update ROI of the sensor
        if (force || params.zoom != new_params.zoom) {
            if (!is_valid_roi(new_params.zoom)) {
                throw Exception(std::errc::invalid_argument, "Invalid ROI value. Normalize between 0.0 and 1.0");
            }
            if (debug) {
                std::cout << "Setting zoom ROI to ";
                std::cout << new_params.zoom.x << ", ";
                std::cout << new_params.zoom.y << ", ";
                std::cout << new_params.zoom.width << ", ";
                std::cout << new_params.zoom.height << std::endl;
            }

            MMAL_PARAMETER_INPUT_CROP_T param = {{MMAL_PARAMETER_INPUT_CROP, sizeof(param)}};

            param.rect.x = (65536 * new_params.zoom.x);
            param.rect.y = (65536 * new_params.zoom.y);
            param.rect.width = (65536 * new_params.zoom.width);
            param.rect.height = (65536 * new_params.zoom.height);

            MMAL_STATUS_T status = mmal_port_parameter_set(camera->control, &param.hdr);
            assert_mmal_status(status, "Failed to set ROI for sensor");

            params.zoom = new_params.zoom;
        }

        // Update the rotation of the image
        if (force || params.rotation != new_params.rotation) {
            if (debug) {
                std::cout << "Setting rotation to " << new_params.rotation << std::endl;
            }

            int new_rotation = ((new_params.rotation % 360 ) / 90) * 90;
            MMAL_STATUS_T status;
            status = mmal_port_parameter_set_int32(camera->output[0], MMAL_PARAMETER_ROTATION, new_rotation);
            assert_mmal_status(status, "Failed to set rotation for camera output 0");
            status = mmal_port_parameter_set_int32(camera->output[1], MMAL_PARAMETER_ROTATION, new_rotation);
            assert_mmal_status(status, "Failed to set rotation for camera output 1");
            status = mmal_port_parameter_set_int32(camera->output[2], MMAL_PARAMETER_ROTATION, new_rotation);
            assert_mmal_status(status, "Failed to set rotation for camera output 2");

            params.rotation = new_params.rotation;
        }

        // Update image flip
        if (force || params.hflip != new_params.hflip || params.vflip != new_params.vflip) {

            MMAL_PARAMETER_MIRROR_T param = {{MMAL_PARAMETER_MIRROR, sizeof(param)}, MMAL_PARAM_MIRROR_NONE};

            const char* message;
            if (new_params.hflip && new_params.vflip) {
                message = "Enabling both hflip and vflip";
                param.value = MMAL_PARAM_MIRROR_BOTH;
            } else if (new_params.hflip) {
                message = "Enabling only hflip";
                param.value = MMAL_PARAM_MIRROR_HORIZONTAL;
            } else if (new_params.vflip) {
                message = "Enabling only vflip";
                param.value = MMAL_PARAM_MIRROR_VERTICAL;
            } else {
                message = "Disabling both hflip and vflip";
            }
            if (debug) {
                std::cout << message << std::endl;
            }

            MMAL_STATUS_T status;
            status = mmal_port_parameter_set(camera->output[0], &param.hdr);
            assert_mmal_status(status, "Failed to set flip for camera output 0");
            status = mmal_port_parameter_set(camera->output[1], &param.hdr);
            assert_mmal_status(status, "Failed to set flip for camera output 1");
            status = mmal_port_parameter_set(camera->output[2], &param.hdr);
            assert_mmal_status(status, "Failed to set flip for camera output 2");

            params.hflip = new_params.hflip;
            params.vflip = new_params.vflip;
        }

        // Update shutter speed
        if (force || params.shutter_speed != new_params.shutter_speed) {

            // Depending on the required shutter speed change FPS.
            if (params.shutter_speed > 6000000) {
                 MMAL_PARAMETER_FPS_RANGE_T params = {{MMAL_PARAMETER_FPS_RANGE, sizeof(params)}, {50, 1000}, {166, 1000}};
                 MMAL_STATUS_T status = mmal_port_parameter_set(video_port_, &params.hdr);
                 assert_mmal_status(status, "Failed to set FPS range for video port.");
            }
            else if(mmal_parameters_.shutter_speed > 1000000) {
                 MMAL_PARAMETER_FPS_RANGE_T params = {{MMAL_PARAMETER_FPS_RANGE, sizeof(params)}, {167, 1000}, {999, 1000}};
                 MMAL_STATUS_T status = mmal_port_parameter_set(video_port_, &params.hdr);
                 assert_mmal_status(status, "Failed to set FPS range for video port.");
            }

            if (debug) {
                std::cout << "Setting shutter speed to " << new_params.shutter_speed << std::endl;
            }
            MMAL_STATUS_T status = mmal_port_parameter_set_uint32(camera->control, MMAL_PARAMETER_SHUTTER_SPEED, new_params.shutter_speed);
            assert_mmal_status(status, "Failed to set shutter speed");

            params.shutter_speed = new_params.shutter_speed;
        }

        // Update AWB gains
        if (force || params.awb_gains_blue != new_params.awb_gains_blue || params.awb_gains_red != new_params.awb_gains_red) {

            if (new_params.awb_gains_blue != 0.0 && new_params.awb_gains_red != 0.0) {
                if (debug) {
                    std::cout << "Setting AWB gains to " << new_params.awb_gains_blue;
                    std::cout << " (blue) and " << new_params.awb_gains_red << " (red)" << std::endl;
                }
                MMAL_PARAMETER_AWB_GAINS_T param = {{MMAL_PARAMETER_CUSTOM_AWB_GAINS, sizeof(param)}, {0,0}, {0,0}};
                param.r_gain.num = (unsigned int)(new_params.awb_gains_red * 65536);
                param.b_gain.num = (unsigned int)(new_params.awb_gains_blue * 65536);
                param.r_gain.den = param.b_gain.den = 65536;

                MMAL_STATUS_T status = mmal_port_parameter_set(camera->control, &param.hdr);
                assert_mmal_status(status, "Failed to set custom AWB values");
            }
            params.awb_gains_blue = new_params.awb_gains_blue;
            params.awb_gains_red = new_params.awb_gains_red;
        }

        // Update exposure mode
        if (force || params.exposure_mode != new_params.exposure_mode) {
            if (debug) {
                std::cout << "Setting exposure mode to " << new_params.exposure_mode << std::endl;
            }
            MMAL_PARAMETER_EXPOSUREMODE_T param = {{MMAL_PARAMETER_EXPOSURE_MODE, sizeof(param)}, new_params.exposure_mode};
            MMAL_STATUS_T status = mmal_port_parameter_set(camera->control, &param.hdr);
            assert_mmal_status(status, "Failed to set exposure mode");

            params.exposure_mode = new_params.exposure_mode;
        }

        // Update exposure metering mode
        if (force || params.exposure_meter_mode != new_params.exposure_meter_mode) {
            if (debug) {
                std::cout << "Setting exposure metering mode to " << new_params.exposure_meter_mode << std::endl;
            }
            MMAL_PARAMETER_EXPOSUREMETERINGMODE_T param =
                    {{MMAL_PARAMETER_EXP_METERING_MODE, sizeof(param)}, new_params.exposure_meter_mode};

            MMAL_STATUS_T status = mmal_port_parameter_set(camera->control, &param.hdr);
            assert_mmal_status(status, "Failed to set exposure metering mode");

            params.exposure_meter_mode = new_params.exposure_meter_mode;
        }

        // Update AWB mode
        if (force || params.awb_mode != new_params.awb_mode) {
            if (debug) {
                std::cout << "Setting AWB mode to " << new_params.awb_mode << std::endl;
            }
            MMAL_PARAMETER_AWBMODE_T param = {{MMAL_PARAMETER_AWB_MODE,sizeof(param)}, new_params.awb_mode};
            MMAL_STATUS_T status = mmal_port_parameter_set(camera->control, &param.hdr);
            assert_mmal_status(status, "Failed to set AWB mode");

            params.awb_mode = new_params.awb_mode;
        }

        // Update image effect
        if (force || params.image_effect != new_params.image_effect) {
            if (debug) {
                std::cout << "Setting image effect to " << new_params.image_effect << std::endl;
            }
            MMAL_PARAMETER_IMAGEFX_T param = {{MMAL_PARAMETER_IMAGE_EFFECT,sizeof(param)}, new_params.image_effect};
            MMAL_STATUS_T status = mmal_port_parameter_set(camera->control, &param.hdr);
            assert_mmal_status(status, "Failed to set image effect");

            params.image_effect = new_params.image_effect;
        }

        // Update color effect
        if (force || params.color_effects != new_params.color_effects) {
            if (debug) {
                std::cout << "Setting color effects " << std::endl;
            }
            MMAL_PARAMETER_COLOURFX_T param = {{MMAL_PARAMETER_COLOUR_EFFECT, sizeof(param)}, 0, 0, 0};

            param.enable = new_params.color_effects.enable;
            param.u = new_params.color_effects.u;
            param.v = new_params.color_effects.v;

            MMAL_STATUS_T status = mmal_port_parameter_set(camera->control, &param.hdr);
            assert_mmal_status(status, "Failed to set color effect");

            params.color_effects.enable = new_params.color_effects.enable;
            params.color_effects.u = new_params.color_effects.u;
            params.color_effects.v = new_params.color_effects.v;
        }
    } catch (...) {
        // Save partially updated values.
        mmal_parameters_.export_to(picam_parameters_);
        throw;
    }

    // Save updated values.
    mmal_parameters_.export_to(picam_parameters_);
}

/*************************************************************************************************/

Camera::Impl::Parameters::Parameters()
    : sharpness(0)
    , contrast(0)
    , brightness(50)
    , saturation(0)
    , iso(0) // auto
    , video_stabilisation(0)
    , exposure_compensation(0)
    , zoom({0.0, 0.0, 1.0, 1.0}) // full sensor
    , rotation(0)
    , hflip(0)
    , vflip(0)
    , shutter_speed(0) // auto
    , awb_gains_red(0)
    , awb_gains_blue(0)
    , exposure_mode(MMAL_PARAM_EXPOSUREMODE_AUTO)
    , exposure_meter_mode(MMAL_PARAM_EXPOSUREMETERINGMODE_AVERAGE)
    , awb_mode(MMAL_PARAM_AWBMODE_AUTO)
    , image_effect(MMAL_PARAM_IMAGEFX_NONE)
    , color_effects({0, 128, 128}) // disabled
{}

/*************************************************************************************************/

void Camera::Impl::Parameters::import_from(const picam_params_t& params)
{
    sharpness = params.sharpness;
    contrast = params.contrast;
    brightness = params.brightness;
    saturation = params.saturation;
    zoom = params.zoom;
    exposure_compensation = params.exposure_compensation;
}

/*************************************************************************************************/

void Camera::Impl::Parameters::export_to(picam_params_t& params) const
{
    params.sharpness = sharpness;
    params.contrast = contrast;
    params.brightness = brightness;
    params.saturation = saturation;
    params.zoom = zoom;
    params.exposure_compensation = exposure_compensation;
}

/*************************************************************************************************/

}
