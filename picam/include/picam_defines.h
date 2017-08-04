#ifndef PICAM_DEFINES_H
#define PICAM_DEFINES_H

#ifdef __cplusplus
extern "C" {
#endif

// Define opaque picam camera handle.
typedef void* picam_camera_t;

// Image formats
typedef enum {

    PICAM_IMAGE_FORMAT_GRAY,
    PICAM_IMAGE_FORMAT_BGR,
    PICAM_IMAGE_FORMAT_RGB,

} picam_image_format_t;

// Image structure.
typedef struct {

    picam_image_format_t format;
    unsigned int width;
    unsigned int height;
    unsigned int bytes_per_line;
    unsigned int data_size;
    unsigned char* data;

} picam_image_t;

// Callback used to receive frames from camera.
typedef void (*picam_callback_t)(void*, picam_image_t*);

// Represent a Region of Interest that should be normalized between [0.0, 1.0].
typedef struct {

    float x;
    float y;
    float width;
    float height;

} picam_roi_t;

// Configurable camera parameters that can be changed after creation.
typedef struct {

    int sharpness;             // -100 to 100
    int contrast;              // -100 to 100
    int brightness;            //  0 to 100
    int saturation;            // -100 to 100
    int exposure_compensation; // -25 to 25

    // Specified ROI will be fitted into original width and height, performing a zoom like operation.
    picam_roi_t zoom;

    // Specified ROI will be used to crop image after capture, producing a different width and height.
    picam_roi_t crop;

} picam_params_t;

// Configuration specified during camera creation.
typedef struct {

    picam_image_format_t format;
    unsigned int width;
    unsigned int height;
    double framerate;

} picam_config_t;

#ifdef __cplusplus
}
#endif

#endif // PICAM_DEFINES_H
