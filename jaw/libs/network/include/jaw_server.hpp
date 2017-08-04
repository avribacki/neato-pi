#ifndef JAW_SERVER_H
#define JAW_SERVER_H

#include <string>
#include <thread>
#include <map>
#include <regex>
#include <iostream>

#include "jaw_guid.hpp"
#include "jaw_protected_call.hpp"
#include "jaw_serialization.hpp"
#include "jaw_socket.hpp"

namespace Jaw {

/*************************************************************************************************
 * RPC Server *
 *************************************************************************************************/

// Server for RPC (Remote Procedure Call).
template<class Command>
class Server
{
public:

    // Creates new Server instance storing it as opaque handler in handle parameter.
    // Returned handle is valid, and therefore stoppable, if start returns zero
    static int start(void** handle, const char* address);

    // Stop server deleting instance pointed by handle.
    static int stop(void* handle);

private:

    // Encapsulate handle for handle-based C APIs.
    struct Handle
    {
        // Default constructor.
        Handle();

        // Handle to core library.
        void* value;

        // Publishing method used for callbackas notification.
        std::function<void(OutputBuffer)> publish;
    };

    // Define procedure signature
    using Procedure = std::function<OutputBuffer(Handle&, InputBuffer)>;

    // Define server configuration.
    struct Config
    {
        // Define tasks that handle each command.
        // They receive the handle and arguments and should return outputs as OutputBuffer.
        struct Task
        {
            Command cmd;
            Procedure execute;
        };

        // Task called to create handle.
        // For this case, the first parameter is pointer to handle (void**);
        Task task_create;

        // Task called to destroy the handle.
        Task task_destroy;

        // Other tasks ordered by priority.
        std::vector<Task> task_list;
    };

    // Construct new server that will listen on specified address.
    Server(const std::string& address);

    // Stop server and destroys it.
    ~Server();

    // Return reference to this Server configuration.
    // Should be defined using template specialization.
    const Config& config();

    // Method executed by main thread
    void main_loop();

    // Process requests for server socket.
    OutputBuffer process_request(InputBuffer request);

    // Socket that will process requests.
    std::unique_ptr<ServerSocket> socket_;

    // Socket used for callbacks
    std::shared_ptr<PublisherSocket> publisher_;

    // Port that was assigned by the publisher
    int callback_port_;

    // Thread responsible for asynchronous execution.
    std::unique_ptr<std::thread> main_thread_;

    // List of handles currently managed by this server
    std::map<Guid, Handle> handles_;
};

/*************************************************************************************************/

template<class Command>
int Server<Command>::start(void** handle, const char* address)
{
    if (!handle || !address) {
        return static_cast<int>(std::errc::invalid_argument);
    }

    return protected_call([&handle, &address]() {
        *handle = static_cast<void*>(new Server(address));
        return 0;
    });
}

/*************************************************************************************************/

template<class Command>
int Server<Command>::stop(void* handle)
{
    Server* server = static_cast<Server*>(handle);
    if (server == nullptr) {
        return static_cast<int>(std::errc::invalid_argument);
    }

    return protected_call([&server]() {
        delete server;
        return 0;
    });
}

/*************************************************************************************************/

template<class Command>
Server<Command>::Server(const std::string& address)
    : socket_()
    , publisher_()
    , callback_port_(0)
    , main_thread_()
    , handles_()
{
    // Create server socket
    socket_ = std::make_unique<ServerSocket>(address);

    // Obtain address for publisher by replacing server port with "*"
    std::string publisher_addr = std::regex_replace(socket_->address(), std::regex(":\\d+"), ":*");

    // Create publisher with random port
    publisher_ = std::make_shared<PublisherSocket>(publisher_addr);

    // Obtain the port publisher is listening
    std::string listening_addr = publisher_->address();

    // Extract port from address and convert to string
    std::size_t pos = listening_addr.find_last_of(':');
    callback_port_ = std::stoi(&listening_addr[pos + 1]);

    // Start main thread
    main_thread_ = std::make_unique<std::thread>(&Server::main_loop, this);
}

/*************************************************************************************************/

template<class Command>
Server<Command>::~Server()
{
    for (auto& handle : handles_) {
        config().task_destroy.execute(handle.second, InputBuffer(nullptr, 0));
    }

    socket_->close();
    main_thread_->join();
}

/*************************************************************************************************/

template<class Command>
void Server<Command>::main_loop()
{
    // Exit when process is aborted.
    while (true) {
        try {
            socket_->process([this](InputBuffer in) { return process_request(std::move(in)); });
        }
        catch (const ConnectionException&) {
            // Aborted, exit main loop.
            break;
        }
        catch (const std::exception& e) {
            std::cout << "Process failed: " << e.what() << std::endl;
        }
    }
}

/*************************************************************************************************/

template<class Command>
OutputBuffer Server<Command>::process_request(InputBuffer request)
{
    try {
        // Read robot identifier and command to execute.
        Guid identifier;
        Command cmd;
        read(request, identifier, cmd);

        // Look for robots registered by this server.
        Handle& handle = handles_[identifier];

        // Make sure that specified robot was already created or is being created now.
        if (handle.value == nullptr && cmd != config().task_create.cmd) {
            OutputBuffer reply;
            write(reply, std::errc::operation_not_supported);
            return reply;
        }

        // First try ordinary commands as they should be more frequent.
        for (auto& task : config().task_list) {
            if (cmd == task.cmd) {
                return task.execute(handle, std::move(request));
            }
        }

        // Now try create/destroy commands:

        if (cmd == config().task_create.cmd) {

            // This instance was already initialized.
            if (handle.value != nullptr) {
                OutputBuffer reply;
                write(reply, std::errc::connection_already_in_progress);
                return reply;
            }

            OutputBuffer reply = config().task_create.execute(handle, std::move(request));

            if (handle.value == nullptr) {
                // If handle was not properly initialized, remove it
                handles_.erase(identifier);
            } else {
                // Otherwise, create publishing method and append callback port to reply.
                std::string id_str = identifier.to_string();
                handle.publish = [this, id_str](OutputBuffer msg) { publisher_->publish(id_str, std::move(msg)); };
                write(reply, callback_port_);
            }

            return reply;
        }

        if (cmd == config().task_destroy.cmd) {
            OutputBuffer reply = config().task_destroy.execute(handle, std::move(request));
            handles_.erase(identifier);
            return reply;
        }

    } catch (const std::exception& e) {

        // Something went terribly wrong.
        std::cout << "Unhandled exception: " << e.what() << std::endl;

        OutputBuffer reply;
        write(reply, std::errc::state_not_recoverable);
        return reply;
    }

    // Received invalid command.
    OutputBuffer reply;
    write(reply, std::errc::operation_not_supported);
    return reply;
}

/*************************************************************************************************/

template<class Command>
Server<Command>::Handle::Handle()
    : value()
    , publish(nullptr)
{}

/*************************************************************************************************/

}

#endif // JAW_SERVER_H
