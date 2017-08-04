#ifndef NEATO_ROBOT_H
#define NEATO_ROBOT_H

#include "neato_defines.h"

#include "jaw_exception.hpp"
#include "neato_serial_port.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

namespace Neato  {

/*************************************************************************************************/

class Robot
{
public:
    // Create new Robot using supplied configuration.
    // Open serial connection and start threads.
    Robot(const neato_config_t& config);

    // Can't copy NeatoRobot instance.
    Robot(const Robot&) = delete;

    // Close serial connection and stop threads.
    ~Robot();

    // Get current robot pose (x, y, theta).
    neato_pose_t get_pose();

    // Perform a laser scan saving the results on the supplied structure.
    void get_laser_scan(neato_laser_data_t* laser_data);

    // Set the current translational speed.
    void set_speed(double speed);

    // Return true if robot has only translational movement (finished changing heading).
    bool is_heading_done();

    // Request a change in heading direction by delta degrees.
    void set_delta_heading(double delta);

private:

    // Read left and right wheel distance in millimeters.
    void read_odometry(int& left_distance, int& right_distance);

    // Read laser information
    void read_laser(neato_laser_data_t* laser_data);

    // Method executed by main thread
    void main_loop();

    // Serial connection used to communicate with the robot.
    std::shared_ptr<SerialPort> serial_;

    // Current robot pose.
    neato_pose_t pose_;

    // Current robot speed.
    std::atomic<double> speed_;

    // Difference in defined by set_delta_heading.
    std::atomic<double> delta_heading_;

    // Pointer to array of laser data supplied by get_laser_scan.
    neato_laser_data_t* laser_data_;

    // The interval in miliseconds between each loop call.
    std::chrono::milliseconds interval_;

    // Mutex used to protect attributes access.
    std::mutex pose_mutex_;
    std::mutex laser_mutex_;

    // Conditionals used synchronize events.
    std::condition_variable laser_ready_;

    // Current displacement of left and right wheels measured by the robot.
    int left_wheel_distance_;
    int right_wheel_distance_;

    // Used to indicate if thread should keep running or stop.
    std::atomic<bool> keep_running_;

    // Threads responsible for asynchronous execution.
    std::unique_ptr<std::thread> main_thread_;
};

/*************************************************************************************************/

}

#endif // NEATO_ROBOT_H
