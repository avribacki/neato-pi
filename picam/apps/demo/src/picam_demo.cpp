#include "picam_api.h"

#include <iostream>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <atomic>
#include <thread>

#ifdef USE_OPENCV // Use frame queue to display images using cv::imshow.

#include <opencv2/opencv.hpp>
#include "jaw_queue.hpp"

static Jaw::Queue<cv::Mat> frame_queue(1);

static void camera_callback(void*, picam_image_t* image)
{
    int type = (image->format == PICAM_IMAGE_FORMAT_GRAY) ? CV_8UC1 : CV_8UC3;
    cv::Mat frame(image->height, image->width, type, image->data);
    cv::Mat copy;
    if (image->format == PICAM_IMAGE_FORMAT_RGB) {
        cv::cvtColor(frame, copy, CV_RGB2BGR);
    } else {
        copy = frame.clone();
    }
    frame_queue.push(copy, std::chrono::seconds(0));
}

#else // Provide simple callback.

static void camera_callback(void*, picam_image_t* image)
{
    std::cout << "Got new image!" << std::endl;
    std::cout << "W : " << image->width << " H : " << image->height << " L : " << image->bytes_per_line << std::endl;
    std::cout << "Data Size : " << image->data_size << std::endl;
    std::cout << std::endl;
}

#endif

int main(int argc, char* argv[])
{
    std::string address = "localhost:50123";

    if (argc > 1) {
        address = argv[1];
    }

    std::cout << "Connecting to camera" << std::endl;

    picam_config_t config = {};
    config.format = PICAM_IMAGE_FORMAT_BGR;
    config.width = 640;
    config.height = 640;
    config.framerate = 2;

    // Create camera
    picam_camera_t camera = nullptr;
    int error = picam_create(&camera, &config, address.c_str());

    // Update parameters
    picam_params_t params;
    error = error || picam_params_get(camera, &params);

    params.sharpness = 0;
    params.contrast = 0;
    params.brightness = 50;
    params.saturation = 0;
    params.exposure_compensation = 0;

    error = error || picam_params_set(camera, &params);

    // Enable camera callback
    error = error || picam_callback_set(camera, nullptr, &camera_callback);

    if (error) {
        std::cout << "Error " << error << " connecting to camera!" << std::endl;
        std::cout << "Press enter to exit..." << std::endl;
        std::cin.ignore();
        return -1;
    }

    std::cout << "Press ENTER to stop camera..." << std::endl;

#ifdef USE_OPENCV

    std::atomic_bool keep_running;
    keep_running = true;
    std::atomic_int frame_counter;
    frame_counter = 0;

    std::thread frame_monitor([&keep_running, &frame_counter]() {
        // Save current time
        auto wake_at = std::chrono::steady_clock::now();
        while (keep_running) {
            wake_at += std::chrono::seconds(1);
            std::this_thread::sleep_until(wake_at);
            int num_frames = frame_counter.exchange(0);
            std::cout << "FPS " << num_frames << std::endl;
        }
    });

    while (true) {
        char key = cv::waitKey(1);

        // Stops if ENTER was pressed. Checks Windows and Linux values.
        if (key == 13 || key == 10) {
            keep_running = false;
            break;
        }

        if (key == 'w' || key == 'q') {
            picam_params_t params;
            picam_params_get(camera, &params);
            params.brightness += (key == 'w') ? 10 : -10;
            std::cout << "Setting brightness " << params.brightness << std::endl;
            picam_params_set(camera, &params);
        }

        cv::Mat frame;
        if (frame_queue.pop(frame, std::chrono::milliseconds(100))) {
            cv::imshow("Frame", frame);
            frame_counter.fetch_add(1);
        }

    }
    frame_monitor.join();

#else

    // Waits for ENTER to be pressed.
    std::cin.ignore();

#endif

    picam_destroy(camera);

    return 0;
}
