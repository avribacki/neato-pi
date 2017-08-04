#include "picam_camera.hpp"

#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"

#include <mutex>
#include <future>

namespace PiCam {

/*************************************************************************************************/

// There isn't actually a MMAL structure for the following, so make one
struct MMAL_PARAM_COLOURFX_T
{
   int enable;       // Turn colourFX on or off
   int u,v;          // U and V to use
};


/*************************************************************************************************/

// The actual camera implementation.
// It hides MMAL avoiding unecessary include files to the final user.
class Camera::Impl
{
public:
    // Construct camera using supplied configuration.
    Impl(const picam_config_t& config);

    // Stop capturing.
    ~Impl();

    // Set callback that will be called every time a new frame is availabe.
    void set_callback(void* user_data, picam_callback_t callback);

    // Get current parameters.
    const picam_params_t& parameters();

    // Update parameters to new value.
    void set_parameters(const picam_params_t& params);

private:
    // Struct holding camera parameters for MMAL.
    struct Parameters
    {
        // Set default values.
        Parameters();

        // Update values based on supplied PiCam parameters performing necessary convertions.
        void import_from(const picam_params_t& params);

        // Save current values in supplied PiCam parameters performing necessary convertions.
        void export_to(picam_params_t& params) const;

        int sharpness;              // -100 to 100
        int contrast;               // -100 to 100
        int brightness;             //  0 to 100
        int saturation;             //  -100 to 100
        int iso;                    //  TODO : what range?
        int video_stabilisation;    // 0 or 1 (false or true)
        int exposure_compensation;  // -10 to +10 ?
        picam_roi_t zoom;          // region of interest to use on the sensor. Normalised [0,1] values in the rect.
        int rotation;               // 0-359
        int hflip;                  // 0 or 1
        int vflip;                  // 0 or 1

        int shutter_speed;           // 0 = auto, otherwise the shutter speed in ms

        // Used only with MMAL_PARAM_AWBMODE_OFF.
        float awb_gains_red;         // AWB red gain
        float awb_gains_blue;        // AWB blue gain

        MMAL_PARAM_EXPOSUREMODE_T exposure_mode;
        MMAL_PARAM_EXPOSUREMETERINGMODE_T exposure_meter_mode;
        MMAL_PARAM_AWBMODE_T awb_mode;
        MMAL_PARAM_IMAGEFX_T image_effect;
        MMAL_PARAM_COLOURFX_T color_effects;
    };

    // Callback called when a new buffer is available.
    static void video_port_callback(MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer);

    // Method used to execute set callback asynchronously.
    MMAL_BUFFER_HEADER_T* call_callback(MMAL_BUFFER_HEADER_T* buffer);

    // Check that hardware is properly configured.
    // An exception is thrown if something is wrong.
    void check_hardware();

    // Update camera component with new parameters.
    // Only the ones that are different from current configuration are applied.
    // If force is true, apply all parameters regardless of previous state.
    void update_camera_parameters(const Parameters& new_params, bool force);

    // Configurations defined during camera creation.
    picam_config_t config_;

    // User supplied data that will be passed to callback.
    void* user_data_;

    // Callback registered with this Camera.
    picam_callback_t callback_;

    // Store result of of asynchronous callback execution.
    std::future<MMAL_BUFFER_HEADER_T*> callback_work_;

    // Image that will be passed to callback.
    picam_image_t image_;

    // Camera parameters store in PiCam and MMAL formats.
    picam_params_t picam_parameters_;
    Parameters mmal_parameters_;

    // Underlying MMAL structures
    MMAL_COMPONENT_T* camera_component_;
    MMAL_POOL_T* video_pool_;
    MMAL_PORT_T* video_port_;

    // Mutex used to synchronize state access.
    std::recursive_mutex mutex_;
};

/*************************************************************************************************/

}
