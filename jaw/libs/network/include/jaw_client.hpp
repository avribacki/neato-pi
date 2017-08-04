#ifndef JAW_CLIENT_H
#define JAW_CLIENT_H

#include <memory>
#include <mutex>
#include <chrono>
#include <thread>
#include <functional>
#include <system_error>
#include <map>
#include <tuple>
#include <regex>
#include <iostream>

#include "jaw_protected_call.hpp"
#include "jaw_serialization.hpp"
#include "jaw_socket.hpp"

namespace Jaw {

/*************************************************************************************************
 * RPC Client *
 *************************************************************************************************/

// Client for RPC (Remote Procedure Call).
template<class Command>
class Client
{
public:
    // Define generic function signature that can be registered with callback monitor.
    // When a message containing the Command associated with the callback is received, this
    // function will be invoked with message buffer. It's up to the called method to parse the
    // message and call original callback.
    using Callback = std::function<void(InputBuffer)>;

    // Define timeout type.
    using Timeout = std::chrono::milliseconds;

    // Creates new Client instance storing it as opaque handler in handle parameter.
    // Returned handle is valid, and therefore destroyable, if create returns zero.
    template <class... Input>
    static int create(void** handle, Command cmd, Timeout timeout, const char* address, const std::tuple<Input...>& input);

    // Destroys instance returned by create method.
    static int destroy(void* handle, Command cmd, Timeout timeout);

    // Send request and wait process reply.
    template <class... Input, class... Output>
    static int request(void* handle, Command cmd, Timeout timeout, const std::tuple<Input...>& input, Output&... output);

    // Overloaded version that doesn't receive any input.
    template <class... Output>
    static int request(void* handle, Command cmd, Timeout timeout, Output&... output);

    // Set callback to handle supplied ID on this client.
    // Pass a non-callable callback to disable handling.
    static int set_callback(void* handle, Command cmd, Timeout timeout, const Callback& callback);

private:

    // Create new RPC Client instance connecting to specified address.
    Client(const std::string& address);

    // Forward declaration of CallbackMonitor.
    class CallbackMonitor;

    // Create callback monitor using port saved during construction of client.
    // This will be called only on the first call to set_callback to avoid unecessary sockets.
    int create_callback_monitor();

    // Unique identifier of the connection.
    Guid identifier_;

    // Socket used to communicate with server
    ClientSocket socket_;

    // Store callback port received during creationg.
    unsigned short callback_port_;

    // Monitor for callback events if one was registered.
    std::unique_ptr<CallbackMonitor> callback_monitor_;
};

/*************************************************************************************************/

template<class Command>
template <class... Input>
int Client<Command>::create(void** handle, Command cmd, Timeout timeout, const char* address, const std::tuple<Input...>& input)
{
    if (!handle || !address) {
        return static_cast<int>(std::errc::invalid_argument);
    }

    // Create connection with server
    std::unique_ptr<Client> client;

    int error = protected_call([&handle, &address, &client]() {
        client = std::unique_ptr<Client>(new Client(address));
        *handle = static_cast<void*>(client.get());
        return 0;
    });

    if (error) {
        return error;
    }

    // Create robot in remote server obtaining the port for callbacks.
    error = request(*handle, cmd, timeout, input, client->callback_port_);

    // If successful, release client as it shold be destroyed by neato_destroy now.
    if (!error) {
        client.release();
    }

    return error;
}

/*************************************************************************************************/

template<class Command>
int Client<Command>::destroy(void* handle, Command cmd, Timeout timeout)
{
    Client* client = static_cast<Client*>(handle);
    if (client == nullptr) {
        return static_cast<int>(std::errc::invalid_argument);
    }

    int error = request(handle, cmd, timeout);

    // Even if we fail to perform the request to destroy, delete client to free resources.
    int error_del = protected_call([&client]() {
        delete client;
        return 0;
    });

    // The remote error has higher importance.
    if (error) {
        return error;
    }
    return error_del;
}

/*************************************************************************************************/

template<class Command>
template <class... Input, class... Output>
int Client<Command>::request(void* handle, Command cmd, Timeout timeout, const std::tuple<Input...>& input, Output&... output)
{
    Client* client = static_cast<Client*>(handle);
    if (!client) {
        return static_cast<int>(std::errc::invalid_argument);
    }

    return protected_call([&client, &cmd, &timeout, &input, &output...]()
    {
        // Write request message
        OutputBuffer request;
        write(request, client->identifier_, cmd, input);

        // Perform request and parse reply
        InputBuffer reply = client->socket_.request(std::move(request), timeout);
        std::int32_t error;
        read(reply, error);

        // Only read output if request was successful.
        if (!error) {
            read(reply, output...);
        }
        return error;
    });
}

/*************************************************************************************************/

template<class Command>
template <class... Output>
int Client<Command>::request(void* handle, Command cmd, Timeout timeout, Output&... output)
{
    return request(handle, cmd, timeout, std::make_tuple<>(), output...);
}

/*************************************************************************************************/

template<class Command>
int Client<Command>::set_callback(void* handle, Command cmd, Timeout timeout, const Callback& callback)
{
    Client* client = static_cast<Client*>(handle);
    if (!client) {
        return static_cast<int>(std::errc::invalid_argument);
    }

    // Make sure Callback Monitor is created.
    int error = client->create_callback_monitor();
    if (error) {
        return error;
    }

    // Request server to enable or disable this callback ID notification.
    error = request(handle, cmd, timeout, std::forward_as_tuple((bool) callback));

    // Update monitor if callback was succesfully registered with server or if
    // we are trying to disable the callback.
    if (!error || !callback) {
        return client->callback_monitor_->set_callback(cmd, callback);
    }

    return error;
}

/*************************************************************************************************/

template<class Command>
Client<Command>::Client(const std::string& address)
    : identifier_(Guid::generate())
    , socket_(address)
    , callback_monitor_()
{}

/*************************************************************************************************/

template<class Command>
int Client<Command>::create_callback_monitor()
{
    // Do nothing if already created.
    if (callback_monitor_) {
        return 0;
    }

    return protected_call([this] {

        // Convert received port to string.
        std::string port_str = ":" + std::to_string(callback_port_);

        // Callback address is the same as socket but with the new port.
        std::string addr = std::regex_replace(socket_.address(), std::regex(":\\d+"), port_str);

        // Add new monitor to client using identifier as channel.
        callback_monitor_ = std::make_unique<CallbackMonitor>(addr, identifier_.to_string());

        // Succeeded.
        return 0;
    });
}

/*************************************************************************************************
 * Callback Monitor *
 *************************************************************************************************/

// Launch new thread to monitor callbacks events.
template<class Command>
class Client<Command>::CallbackMonitor
{
public:
    // Create new subscriber socket and launch thread to monitor incoming messages.
    CallbackMonitor(const std::string& address, const std::string& channel);

    // Stop monitoring thread.
    ~CallbackMonitor();

    // Set callback to handle specified ID replacing any previous callback that was set.
    // Pass a non-callable callback to disable handling of speficied ID.
    int set_callback(Command id, const Callback& callback);

private:

    // Method executed by main thread
    void main_loop();

    // Save installed callbacks
    std::map<Command, Callback> callbacks_;

    // Mutex used to lock access to callback mapping.
    std::mutex mutex_;

    // Socket that will be receive callback notifications.
    SubscriberSocket socket_;

    // Thread responsible for asynchronous execution.
    std::thread main_thread_;
};

/*************************************************************************************************/

template<class Command>
Client<Command>::CallbackMonitor::CallbackMonitor(const std::string& address, const std::string& channel)
    : callbacks_()
    , mutex_()
    , socket_(address, channel)
    , main_thread_(&CallbackMonitor::main_loop, this)
{}

/*************************************************************************************************/

template<class Command>
Client<Command>::CallbackMonitor::~CallbackMonitor()
{
    socket_.close();
    main_thread_.join();
}

/*************************************************************************************************/

template<class Command>
int Client<Command>::CallbackMonitor::set_callback(Command id, const Callback& callback)
{
    return protected_call([this, &id, &callback]{
        std::lock_guard<std::mutex> lock(mutex_);

        if (callback) {
            callbacks_[id] = callback;
        } else {
            callbacks_.erase(id);
        }
        return 0;
    });
}

/*************************************************************************************************/

template<class Command>
void Client<Command>::CallbackMonitor::main_loop()
{
    // Exit when receive is aborted
    while (true) {
        try {
            // Block until new message is available
            InputBuffer message = socket_.receive();

            // The first parameter is the callback identifier;
            Command id;
            read(message, id);

            // Find if we have a callback registered to handle this id.
            std::lock_guard<std::mutex> lock(mutex_);
            auto found = callbacks_.find(id);

            // Call it passing remaining of the received message.
            if (found != callbacks_.end()) {
                found->second(std::move(message));
            }
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

}

#endif // JAW_CLIENT_H
