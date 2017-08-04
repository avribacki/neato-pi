#include "picam_server.h"

#include <csignal>
#include <cerrno>
#include <cstring>

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <string>

static std::mutex mutex;
static std::condition_variable condition;
std::atomic_bool interrupted;

void handle_interruption(int)
{
    interrupted = true;
    condition.notify_one();
}

int main(int argc, char* argv[])
{
    const char* port = "50123";

    if (argc > 1) {
        port = argv[1];
    }

    std::string address = std::string("*:") + port;
    std::cout << "Starting PiCam Daemon on address " << address << std::endl;

    auto result = signal(SIGINT, handle_interruption);
    if (result == SIG_ERR) {
        std::cout << "Failed to install signal handler" << strerror(errno) << std::endl;
        return -1;
    }

    interrupted = false;

    // Start Neato server
    picam_server_t server;
    int error = picam_server_start(&server, address.c_str());

    if (error) {
        std::cout << "Failed to start daemon." << std::endl;
        return -1;
    }

    // Wait for Ctrl-C interruption
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, []{ return interrupted.load(); });

    // Stop server
    picam_server_stop(server);

    std::cout << "Finished!" << std::endl;
    return 0;
}
