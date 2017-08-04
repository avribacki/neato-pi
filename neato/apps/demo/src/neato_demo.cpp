#include "neato_api.h"

#include <iostream>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <chrono>

# define M_PI           3.14159265358979323846  /* pi */

void dump_laser(const neato_laser_data_t& laser_data)
{
    std::cout << "X : " << laser_data.pose_taken.x
             << " Y : " << laser_data.pose_taken.y
             << " Theta : " << laser_data.pose_taken.theta << std::endl;

    static const int width = 70;
    static const int height = 40;

    static char buffer[width * height];

    double max = 5000;

    std::memset(buffer, ' ', width * height);

    for (int theta = 0; theta < 360; theta++) {

        double value, distance = laser_data.distance[theta];
        if (distance > 0) {
            value = std::min(distance, max);
        } else {
            value = max;
        }
        int y = height / 2 - static_cast<int>(std::round(std::sin(theta * M_PI / 180.0) * value * (height / (max * 2.0))));
        int x = width / 2 + static_cast<int>(std::round(std::cos(theta * M_PI / 180.0) * value * (width / (max * 2.0))));

        buffer[y * width + x] = distance > 0 ? 'x' : 'E';
    }
    buffer[(height / 2) * width + width / 2] = 'O';

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            std::cout << buffer[i * width + j];
        }
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[])
{
    std::string address = "localhost:50124";

    if (argc > 1) {
        address = argv[1];
    }

    std::cout << "Connecting to robot" << std::endl;

    neato_config_t config = {};
    config.update_interval_ms = 100;

    // Create robot
    neato_robot_t robot = nullptr;
    int error = neato_create(&robot, &config, address.c_str());

    if (error) {
        std::cout << "Error " << error << " connecting to robot!" << std::endl;
        std::cout << "Press enter to exit..." << std::endl;
        std::cin.ignore();
        return -1;
    }

    double current_speed = 0;
    double target_speed = 0;
    neato_laser_data_t laser_data;
    neato_pose_t pose;

    char key;
    do {
        std::cin >> key;
        switch (key) {
            case 'a':
                error = neato_delta_heading_set(robot, -5.0);
                break;
            case 'd' :
                error = neato_delta_heading_set(robot, +5.0);
                break;
            case 'w' :
                target_speed = current_speed + 10.0;
                error = neato_speed_set(robot, current_speed);
                if (!error) {
                    current_speed = target_speed;
                }
                break;
            case 's' :
                target_speed = current_speed - 10;
                error = neato_speed_set(robot, current_speed);
                if (!error) {
                    current_speed = target_speed;
                }
                break;
            case 'q':
                target_speed = 0;
                error = neato_speed_set(robot, current_speed);
                if (!error) {
                    current_speed = target_speed;
                }
                break;
            case 'z':
                error = neato_laser_scan_get(robot, &laser_data);
                if (!error) {
                    dump_laser(laser_data);
                }
                break;
            case 'p':
                error = neato_pose_get(robot, &pose);
                if (!error) {
                    std::cout << "X : " << pose.x << " Y : " << pose.y << " Theta : " << pose.theta << std::endl;
                }
                break;
            default:
                error = 0;
                break;
        }
        if (error) {
            std::cout << "Got error " << strerror(error) << " (" << error << ")" << std::endl;
        }
    } while (key != 'k');

    neato_destroy(robot);
    std::cout << "Disconnected from robot" << std::endl;
    std::cin.ignore();

    return 0;
}
