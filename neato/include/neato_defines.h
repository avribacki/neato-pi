#ifndef NEATO_DEFINES_H
#define NEATO_DEFINES_H

#ifdef __cplusplus
extern "C" {
#endif

// Define opaque neato robot handle.
typedef void* neato_robot_t;

// Represent a 2D robot pose.
typedef struct {
    double x;
    double y;
    double theta;
} neato_pose_t;

// Number of lasear readings.
#define NEATO_NUM_LASER_READINGS 360

// Data from a 360 degrees laser scan.
typedef struct {
    neato_pose_t pose_taken;
    int distance[NEATO_NUM_LASER_READINGS];
} neato_laser_data_t;

// Configuration specified during robot creation.
typedef struct {

    // Interval between each odometry and motors update.
    // Should be greater than 50 ms.
    int update_interval_ms;

} neato_config_t;

#ifdef __cplusplus
}
#endif

#endif // NEATO_DEFINES_H
