#include "neato_robot.hpp"

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

#include <sstream>
#include <iostream>

#include "jaw_exception.hpp"
using namespace  Jaw;

namespace Neato {

/*************************************************************************************************/

#define SIMULATED 1

static std::string dummy_laser =
"AngleInDegrees, DistInMM, Intensity, ErrorCodeHEX\n"
"0, 1134, 83, 0\n" "1, 1141, 86, 0\n" "2, 1829, 8, 0\n" "3, 0, 0, 8035\n" "4, 1334, 51, 0\n" "5, 0, 0, 8035\n"
"6, 0, 0, 8035\n" "7, 0, 0, 8035\n" "8, 1451, 132, 0\n" "9, 2263, 53, 0\n" "10, 2297, 35, 0\n" "11, 0, 0, 8035\n"
"12, 0, 0, 8035\n" "13, 986, 184, 0\n" "14, 987, 293, 0\n" "15, 0, 0, 8035\n" "16, 1890, 70, 0\n" "17, 3683, 13, 0\n"
"18, 3708, 13, 0\n" "19, 2486, 27, 0\n" "20, 0, 0, 8035\n" "21, 1743, 70, 0\n" "22, 0, 0, 8035\n" "23, 1593, 111, 0\n"
"24, 2777, 235, 0\n" "25, 0, 0, 8035\n" "26, 2129, 45, 0\n" "27, 2710, 18, 0\n" "28, 0, 0, 8035\n" "29, 0, 0, 8035\n"
"30, 0, 0, 8035\n" "31, 0, 0, 8035\n" "32, 0, 0, 8035\n" "33, 0, 0, 8035\n" "34, 1198, 56, 0\n" "35, 0, 0, 8035\n"
"36, 1081, 49, 0\n" "37, 1054, 34, 0\n" "38, 1054, 25, 0\n" "39, 0, 0, 8035\n" "40, 0, 0, 8035\n" "41, 0, 0, 8035\n"
"42, 978, 50, 0\n" "43, 974, 117, 0\n" "44, 967, 218, 0\n" "45, 943, 157, 0\n" "46, 928, 283, 0\n" "47, 917, 310, 0\n"
"48, 908, 336, 0\n" "49, 900, 324, 0\n" "50, 892, 329, 0\n" "51, 885, 341, 0\n" "52, 878, 327, 0\n" "53, 872, 347, 0\n"
"54, 865, 257, 0\n" "55, 859, 376, 0\n" "56, 853, 387, 0\n" "57, 846, 103, 0\n" "58, 729, 126, 0\n" "59, 722, 235, 0\n"
"60, 719, 292, 0\n" "61, 720, 283, 0\n" "62, 726, 180, 0\n" "63, 759, 62, 0\n" "64, 821, 415, 0\n" "65, 819, 440, 0\n"
"66, 816, 439, 0\n" "67, 814, 452, 0\n" "68, 812, 447, 0\n" "69, 811, 454, 0\n" "70, 809, 471, 0\n" "71, 808, 456, 0\n"
"72, 803, 223, 0\n" "73, 755, 73, 0\n" "74, 734, 139, 0\n" "75, 723, 174, 0\n" "76, 715, 227, 0\n" "77, 710, 285, 0\n"
"78, 705, 233, 0\n" "79, 0, 0, 8035\n" "80, 0, 0, 8035\n" "81, 0, 0, 8035\n" "82, 0, 0, 8035\n" "83, 0, 0, 8035\n"
"84, 0, 0, 8035\n" "85, 0, 0, 8035\n" "86, 832, 26, 0\n" "87, 823, 323, 0\n" "88, 822, 415, 0\n" "89, 824, 433, 0\n"
"90, 827, 413, 0\n" "91, 831, 436, 0\n" "92, 836, 421, 0\n" "93, 840, 408, 0\n" "94, 845, 419, 0\n" "95, 851, 404, 0\n"
"96, 857, 273, 0\n" "97, 862, 410, 0\n" "98, 868, 381, 0\n" "99, 874, 373, 0\n" "100, 881, 361, 0\n" "101, 888, 320, 0\n"
"102, 896, 332, 0\n" "103, 905, 352, 0\n" "104, 912, 308, 0\n" "105, 922, 281, 0\n" "106, 932, 280, 0\n" "107, 945, 256, 0\n"
"108, 955, 254, 0\n" "109, 967, 224, 0\n" "110, 982, 175, 0\n" "111, 998, 133, 0\n" "112, 1017, 100, 0\n" "113, 1031, 42, 0\n"
"114, 0, 0, 8035\n" "115, 0, 0, 8035\n" "116, 0, 0, 8035\n" "117, 0, 0, 8035\n" "118, 0, 0, 8035\n" "119, 1144, 48, 0\n"
"120, 1167, 83, 0\n" "121, 1194, 110, 0\n" "122, 1220, 123, 0\n" "123, 1311, 124, 0\n" "124, 0, 0, 8035\n" "125, 1263, 18, 0\n"
"126, 1252, 163, 0\n" "127, 1235, 184, 0\n" "128, 1219, 191, 0\n" "129, 1202, 187, 0\n" "130, 1188, 209, 0\n" "131, 1174, 212, 0\n"
"132, 1161, 217, 0\n" "133, 1148, 219, 0\n" "134, 1127, 51, 0\n" "135, 0, 0, 8035\n" "136, 1116, 71, 0\n" "137, 1103, 227, 0\n"
"138, 1094, 229, 0\n" "139, 1084, 241, 0\n" "140, 1075, 247, 0\n" "141, 1066, 251, 0\n" "142, 1058, 253, 0\n" "143, 1050, 259, 0\n"
"144, 1043, 261, 0\n" "145, 1036, 278, 0\n" "146, 0, 278, 8021\n" "147, 1024, 284, 0\n" "148, 1017, 272, 0\n" "149, 1012, 279, 0\n"
"150, 1007, 280, 0\n" "151, 1002, 281, 0\n" "152, 997, 291, 0\n" "153, 994, 284, 0\n" "154, 989, 280, 0\n" "155, 986, 290, 0\n"
"156, 984, 293, 0\n" "157, 981, 295, 0\n" "158, 978, 297, 0\n" "159, 977, 299, 0\n" "160, 975, 301, 0\n" "161, 973, 289, 0\n"
"162, 969, 291, 0\n" "163, 956, 94, 0\n" "164, 0, 0, 8035\n" "165, 0, 0, 8035\n" "166, 0, 0, 8035\n" "167, 0, 0, 8035\n"
"168, 0, 0, 8035\n" "169, 0, 0, 8035\n" "170, 0, 0, 8035\n" "171, 997, 7, 0\n" "172, 986, 157, 0\n" "173, 982, 281, 0\n"
"174, 982, 288, 0\n" "175, 983, 291, 0\n" "176, 986, 286, 0\n" "177, 989, 278, 0\n" "178, 993, 291, 0\n" "179, 996, 287, 0\n"
"180, 1001, 293, 0\n" "181, 1006, 282, 0\n" "182, 1011, 281, 0\n" "183, 1017, 289, 0\n" "184, 1022, 286, 0\n" "185, 1029, 283, 0\n"
"186, 1034, 273, 0\n" "187, 1041, 268, 0\n" "188, 1028, 285, 0\n" "189, 1037, 288, 0\n" "190, 1042, 81, 0\n" "191, 789, 73, 0\n"
"192, 762, 102, 0\n" "193, 735, 109, 0\n" "194, 710, 122, 0\n" "195, 689, 133, 0\n" "196, 670, 123, 0\n" "197, 653, 114, 0\n"
"198, 637, 98, 0\n" "199, 623, 68, 0\n" "200, 608, 39, 0\n" "201, 0, 0, 8035\n" "202, 0, 0, 8035\n" "203, 0, 0, 8035\n"
"204, 0, 0, 8035\n" "205, 0, 0, 8035\n" "206, 16885, 28, 0\n" "207, 493, 64, 0\n" "208, 486, 113, 0\n" "209, 479, 176, 0\n"
"210, 473, 223, 0\n" "211, 467, 284, 0\n" "212, 459, 328, 0\n" "213, 451, 348, 0\n" "214, 443, 322, 0\n" "215, 436, 343, 0\n"
"216, 429, 360, 0\n" "217, 423, 377, 0\n" "218, 0, 377, 8021\n" "219, 412, 385, 0\n" "220, 405, 377, 0\n" "221, 400, 414, 0\n"
"222, 395, 428, 0\n" "223, 390, 419, 0\n" "224, 385, 407, 0\n" "225, 381, 418, 0\n" "226, 377, 484, 0\n" "227, 372, 463, 0\n"
"228, 369, 484, 0\n" "229, 365, 519, 0\n" "230, 362, 497, 0\n" "231, 358, 543, 0\n" "232, 355, 559, 0\n" "233, 352, 572, 0\n"
"234, 349, 568, 0\n" "235, 346, 557, 0\n" "236, 343, 545, 0\n" "237, 341, 575, 0\n" "238, 339, 603, 0\n" "239, 336, 624, 0\n"
"240, 334, 631, 0\n" "241, 332, 600, 0\n" "242, 331, 617, 0\n" "243, 328, 595, 0\n" "244, 327, 653, 0\n" "245, 326, 679, 0\n"
"246, 324, 699, 0\n" "247, 323, 674, 0\n" "248, 319, 386, 0\n" "249, 16698, 43, 0\n" "250, 0, 0, 8035\n" "251, 0, 0, 8035\n"
"252, 0, 0, 8035\n" "253, 0, 0, 8035\n" "254, 0, 0, 8035\n" "255, 0, 0, 8035\n" "256, 0, 0, 8035\n" "257, 0, 0, 8035\n"
"258, 318, 341, 0\n" "259, 316, 763, 0\n" "260, 315, 842, 0\n" "261, 315, 905, 0\n" "262, 315, 870, 0\n" "263, 315, 892, 0\n"
"264, 315, 858, 0\n" "265, 316, 881, 0\n" "266, 316, 855, 0\n" "267, 317, 845, 0\n" "268, 318, 816, 0\n" "269, 319, 783, 0\n"
"270, 320, 753, 0\n" "271, 321, 745, 0\n" "272, 322, 756, 0\n" "273, 323, 719, 0\n" "274, 325, 732, 0\n" "275, 326, 697, 0\n"
"276, 328, 759, 0\n" "277, 330, 672, 0\n" "278, 332, 627, 0\n" "279, 333, 615, 0\n" "280, 336, 620, 0\n" "281, 338, 631, 0\n"
"282, 340, 607, 0\n" "283, 344, 546, 0\n" "284, 350, 445, 0\n" "285, 356, 289, 0\n" "286, 362, 174, 0\n" "287, 16752, 34, 0\n"
"288, 0, 0, 8035\n" "289, 0, 0, 8035\n" "290, 0, 0, 8035\n" "291, 360, 116, 0\n" "292, 368, 236, 0\n" "293, 375, 348, 0\n"
"294, 383, 468, 0\n" "295, 389, 466, 0\n" "296, 394, 456, 0\n" "297, 399, 490, 0\n" "298, 406, 436, 0\n" "299, 412, 419, 0\n"
"300, 418, 428, 0\n" "301, 425, 402, 0\n" "302, 432, 389, 0\n" "303, 440, 369, 0\n" "304, 449, 378, 0\n" "305, 457, 363, 0\n"
"306, 466, 360, 0\n" "307, 476, 338, 0\n" "308, 486, 334, 0\n" "309, 497, 323, 0\n" "310, 509, 311, 0\n" "311, 521, 272, 0\n"
"312, 534, 280, 0\n" "313, 548, 253, 0\n" "314, 563, 249, 0\n" "315, 578, 244, 0\n" "316, 595, 211, 0\n" "317, 614, 192, 0\n"
"318, 634, 169, 0\n" "319, 655, 148, 0\n" "320, 679, 123, 0\n" "321, 703, 98, 0\n" "322, 729, 47, 0\n" "323, 982, 147, 0\n"
"324, 970, 135, 0\n" "325, 942, 18, 0\n" "326, 0, 0, 8035\n" "327, 0, 0, 8035\n" "328, 0, 0, 8035\n" "329, 0, 0, 8035\n"
"330, 0, 0, 8035\n" "331, 0, 0, 8035\n" "332, 1502, 14, 0\n" "333, 1514, 28, 0\n" "334, 1516, 11, 0\n" "335, 0, 0, 8035\n"
"336, 0, 0, 8035\n" "337, 0, 0, 8035\n" "338, 0, 0, 8035\n" "339, 0, 0, 8035\n" "340, 1498, 8, 0\n" "341, 1503, 19, 0\n"
"342, 1532, 32, 0\n" "343, 1535, 33, 0\n" "344, 1506, 13, 0\n" "345, 1489, 38, 0\n" "346, 1588, 40, 0\n" "347, 1657, 54, 0\n"
"348, 1685, 31, 0\n" "349, 3325, 40, 0\n" "350, 0, 0, 8035\n" "351, 1975, 14, 0\n" "352, 0, 0, 8035\n" "353, 0, 0, 8035\n"
"354, 0, 0, 8035\n" "355, 0, 0, 8035\n" "356, 1116, 100, 0\n" "357, 1119, 102, 0\n" "358, 1125, 94, 0\n" "359, 1129, 96, 0\n"
"ROTATION_SPEED, 5.02";

/*************************************************************************************************/

#ifndef M_PI
# define M_PI           3.14159265358979323846  /* pi */
#endif

/*************************************************************************************************/

Robot::Robot(const neato_config_t& config)
    : serial_()
    , pose_()
    , speed_(0.0)
    , delta_heading_(0.0)
    , laser_data_(nullptr)
    , interval_(config.update_interval_ms)
    , pose_mutex_()
    , laser_mutex_()
    , laser_ready_()
    , left_wheel_distance_(0)
    , right_wheel_distance_(0)
    , keep_running_(true)
    , main_thread_()
{
    std::cout << "Creating Robot" << std::endl;

    // Only enable console access if update interval was set.
    if (config.update_interval_ms <= 50) {
        throw Exception(std::errc::invalid_argument, "Update interval should be greater than 50 ms");
    }

    // Try to access serial in constructor so we don't even start main thread if it fails.
    serial_ = SerialPort::Create("/dev/ttyACM0");
    main_thread_ = std::make_unique<std::thread>(&Robot::main_loop, this);
}

/*************************************************************************************************/

Robot::~Robot()
{
    keep_running_ = false;

    if (main_thread_) {
        main_thread_->join();
        main_thread_.reset();
    }

    std::cout << "Destroyed Robot" << std::endl;
}

/*************************************************************************************************/

neato_pose_t Robot::get_pose()
{
    neato_pose_t current;
    {
        std::lock_guard<std::mutex> lock(pose_mutex_);
        current = pose_;
    }
    current.theta = current.theta * 180.0 / M_PI;
    return current;
}

/*************************************************************************************************/

void Robot::get_laser_scan(neato_laser_data_t* laser_data)
{
    if (laser_data) {
        std::unique_lock<std::mutex> lock(laser_mutex_);
        laser_data_ = laser_data;
        laser_ready_.wait(lock);
    }
}

/*************************************************************************************************/

void Robot::set_speed(double speed)
{
    //std::cout << "Set speed to " << speed << std::endl;
    speed_ = speed;
}

/*************************************************************************************************/

bool Robot::is_heading_done()
{
    return (delta_heading_ == 0.0);
}

/*************************************************************************************************/

void Robot::set_delta_heading(double delta)
{
    std::cout << "Set delta heading to " << delta << std::endl;
    delta_heading_ = delta * M_PI / 180.0;
}

/*************************************************************************************************/

void Robot::read_odometry(int& left_distance, int& right_distance)
{
#if SIMULATED
    // TODO : actually simulate based on configured speed.
    left_distance = 0;
    right_distance = 0;
    return;
#else
    // Tags used to find odmetry values inside GetMotors result.
    const char* left_tag =  "LeftWheel_PositionInMM,";
    const char* right_tag = "RightWheel_PositionInMM,";
    const std::size_t left_tag_len = std::strlen(left_tag);
    const std::size_t right_tag_len = std::strlen(right_tag);

    // Execute command in serial
    std::string result = serial_->execute("GetMotors LeftWheel RightWheel");

    // Search for left tag
    const char* c_result = result.c_str();
    const char* left_str = std::strstr(c_result, left_tag);
    if (left_str == nullptr) {
        throw Exception(std::errc::io_error, "Error reading wheel position");
    }
    left_str += left_tag_len;
    left_distance = std::strtol(left_str, nullptr, 10);

    // Search for right tag
    const char* right_str = std::strstr(left_str, right_tag);
    if (right_str == nullptr) {
        throw Exception(std::errc::io_error, "Error reading wheel position");
    }
    right_str += right_tag_len;
    right_distance = std::strtol(right_str, nullptr, 10);
#endif
}

/*************************************************************************************************/

void Robot::read_laser(neato_laser_data_t* laser_data)
{
    // Get laser scan result
#if SIMULATED
    std::string result = dummy_laser;
#else
    std::string result = serial_->execute("GetLDSScan");
#endif

    char* c_result = const_cast<char*>(result.c_str());

    // Skip header
    char* entry_str = std::strstr(c_result, "\n");

    int max = 5000;

    // Record pose when scan was taken.
    laser_data->pose_taken = get_pose();

    for (int i = 0; i < 360; i++) {
        int angle = std::strtol(++entry_str, &entry_str, 10);
        if (angle != i) {
            throw Exception(std::errc::io_error, "Error reading laser information");
        }
        int distance = std::strtol(++entry_str, &entry_str, 10);
        int intensity = std::strtol(++entry_str, &entry_str, 10);
        int error = std::strtol(++entry_str, &entry_str, 10);

        if (error == 0) {
            laser_data->distance[angle] = std::min(distance, max);
        } else {
            laser_data->distance[angle] = 0;
        }

    }
}

/*************************************************************************************************/

void Robot::main_loop()
{
    std::cout << "Inside main loop" << std::endl;

    try {
        static double wheel_distance = 235.0; // or 248??
        static double max_speed = 100.0;

        // Flag used to stop motors if requested
        bool stopped = true;

        serial_->execute("PlaySound 1");
        serial_->execute("TestMode On");
        serial_->execute("SetLDSRotation On");

        // Save current time
        auto wake_at = std::chrono::steady_clock::now();

        // Read starting values for wheels from serial
        read_odometry(left_wheel_distance_, right_wheel_distance_);

        while (keep_running_) {

            wake_at += interval_;

            // Update wheels distance
            int current_left = 0;
            int current_right = 0;

            // Read current values from serial
            read_odometry(current_left, current_right);

//            std::cout << "L : " << current_left << " R : " << current_right << std::endl;

            int delta_left = current_left - left_wheel_distance_;
            int delta_right = current_right - right_wheel_distance_;

            // Update saved values
            left_wheel_distance_ = current_left;
            right_wheel_distance_ = current_right;

            // Calculate displacement

            double delta_theta = (delta_right - delta_left) / wheel_distance;
            double delta_distance = (delta_right + delta_left) / 2.0;

            double delta_x = delta_distance * std::cos(pose_.theta + delta_theta / 2.0);
            double delta_y = delta_distance * std::sin(pose_.theta + delta_theta / 2.0);

            // Update pose
            {
                std::unique_lock<std::mutex> lock(pose_mutex_);
                pose_.x += delta_x;
                pose_.y += delta_y;
                pose_.theta += delta_theta;

                //std::cout << "X : " << pose_.x << " Y : " << pose_.y << " Theta : " << pose_.theta * 180.0 / M_PI << std::endl;
            }

            // Verify if laser information was requested and update if needed
            {
                std::unique_lock<std::mutex> lock(laser_mutex_);
                if (laser_data_) {
                    read_laser(laser_data_);
                    laser_data_ = nullptr;
                    laser_ready_.notify_all();
                }
            }

            // Update wheels speed

            if (delta_heading_ != 0.0) {
                if (std::abs(delta_heading_) > std::abs(delta_theta)) {
                    delta_heading_ = delta_heading_ - delta_theta;
                } else {
                    // Finished updating heading.
                    delta_heading_ = 0.0;
                }
            }

            int lwheeldist = static_cast<int>(speed_ - delta_heading_ * wheel_distance / 2.0);
            int rwheeldist = static_cast<int>(speed_ + delta_heading_ * wheel_distance / 2.0);

            // Execute SetMotor command
            int final_speed = static_cast<int>(std::abs(speed_));
            if (std::abs(delta_heading_) > std::numeric_limits<double>::epsilon() && final_speed == 0) {
                final_speed = 10; // Fixed speed for turning in place
            }

            if (final_speed > 0) {
                std::ostringstream speed_cmd;
                speed_cmd << "SetMotor Speed " << final_speed << " LWheelDist " << lwheeldist << " RWheelDist " << rwheeldist;
                serial_->execute(speed_cmd.str());
                stopped = false;
            } else if (!stopped) {
                serial_->execute("SetMotor Speed 1 LWheelDist 1 RWheelDist 1");
                stopped = true;
            }

            std::this_thread::sleep_until(wake_at);
        }

        serial_->execute("SetLDSRotation Off");
        serial_->execute("TestMode Off");
        serial_->execute("PlaySound 2");

    } catch (...) {
        std::cout << "Exception raised in main loop." << std::endl;
    }
}

/*************************************************************************************************/

}
