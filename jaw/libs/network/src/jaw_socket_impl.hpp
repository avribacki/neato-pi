#ifndef JAW_SOCKET_IMPL_H
#define JAW_SOCKET_IMPL_H

#include "jaw_socket.hpp"

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

#include <zmq.hpp>

namespace Jaw {

/*************************************************************************************************/

// Monitor a ZMQ socket to assert connection was effectively done.
// Connections in ZMQ can occur asynchronously. Creating this monitor before connecting and
// calling wait_connection can be used to assert that the server side is up and running.
class SocketMonitor
    : public zmq::monitor_t
{
public:
    // Creates monitor for the specified socket.
    SocketMonitor(zmq::socket_t& socket);

    // Stops and destroys monitor.
    virtual ~SocketMonitor();

    // Wait timeout milliseconds for the connected event to be triggered for the monitored socket.
    // Returns connected address if event is received before timeout, empty string otherwise.
    bool wait_connection(std::chrono::milliseconds timeout);

    // Wait timeout milliseconds for the listening event to be triggered for the monitored socket.
    // Returns binded address if event is received before timeout, empty string otherwise.
    bool wait_listening(std::chrono::milliseconds timeout);

private:
    // Code executed by thread
    void run(zmq::socket_t &socket);

    // Callbacks for zmq::monitor_t
    virtual void on_monitor_started();
    virtual void on_event_connected(const zmq_event_t &event, const char* addr);
    virtual void on_event_listening(const zmq_event_t &event, const char *addr);

    // Flags used to detect monitor start and connection event
    bool started_;
    bool connected_;
    bool listening_;

    // Variables used to wait for special conditions
    std::condition_variable condition_;
    std::mutex mutex_;

    // Holds the thread that will run the monitor
    std::thread thread_;
};

/*************************************************************************************************/

// The actual socket implementation.
// It hides ZMQ avoiding unecessary include files to the final user.
class Socket::Impl
{
public:
    // Create a Socket::Impl with supplied address and type (e.g. ZMQ_REQ)
    Impl(const std::string& address, int zmq_type);

    // Destroys this implementation.
    virtual ~Impl();

    // Gets the address to which this socket is bounded (connected or listening).
   const std::string& address();

    // Bind socket
    void bind();

    // Establishes the connection.
    void connect();

    // Closes opened connection aborting blocking operations.
    void close();

protected:

    // Poll for events on this sockets.
    // This method will block indefinitely until the events are received or close is called.
    // When aborted, throws ConnectionException.
    void poll(short events);

    // Configure the socket after creation but before connection.
    // To be overloaded by derived classes.
    virtual void configure_socket() {}

    // The underlying infratructure used to connect
    std::unique_ptr<zmq::context_t> context_;
    std::unique_ptr<zmq::socket_t> socket_;

    // Mutex used to control access to ZMQ functionality is it is not thread safe.
    // Also used to ensure state machine for ZMQ_REP and ZMQ_REQ.
    std::recursive_mutex zmq_mutex_;

private:

    // Use last endpoint as new zmq_address_.
    void update_address();

    // Connection configuration
    std::string zmq_address_;       // The ZMQ Address of the connection
    int zmq_type_;                  // The ZMQ Type of the connection (ZMQ_REQ, ZMQ_SUB, ...)

    // Sockets used to abort blocking poll procedure.
    std::unique_ptr<zmq::socket_t> poll_abort_requester_;
    std::unique_ptr<zmq::socket_t> poll_abort_listener_;
    std::mutex poll_mutex_;
};

/*************************************************************************************************/

// Client socket implmentation using ZMQ_REQ
class ClientSocket::Impl
    : public Socket::Impl
{
public:
    // Create new client socket implementation.
    Impl(const std::string& address);

    // Send data and receive reply as input buffer.
    // An exception is thrown if no reply is received before timeout.
    InputBuffer request(OutputBuffer message, std::chrono::milliseconds timeout);

protected:
    // Overload configure method to set LINGER time to zero.
    void configure_socket() override;
};

/*************************************************************************************************/

// Server socket implmentation using ZMQ_REP
class ServerSocket::Impl
    : public Socket::Impl
{
public:
    // Create new server socket implementation.
    Impl(const std::string& address);

    // Wait indefinitely for a request and process it.
    // Throw TimeoutException when socket is destructed.
    void process(const Work& work);
};

/*************************************************************************************************/

// Subscriber socket implmentation using ZMQ_SUB
class SubscriberSocket::Impl
    : public Socket::Impl
{
public:
    // Create new subscriber socket implementation.
    Impl(const std::string& address, const std::string& channel);

    // Receive raw data from connection waiting indefinitely.
    // Throw TimeoutException when socket is destructed.
    InputBuffer receive();

protected:
    // Overload configure method to set up subscriber channel.
    void configure_socket() override;

private:
     // The channel used to filter the subscriber socket.
    std::string channel_;
};

/*************************************************************************************************/

// Publisher socket implmentation using ZMQ_PUB
class PublisherSocket::Impl
    : public Socket::Impl
{
public:
    // Create new publisher socket implementation.
    Impl(const std::string& address);

    // Publish message to specified channel.
    void publish(const std::string& channel, OutputBuffer message);

protected:
    // Overload configure method to set HWM to low value.
    // TODO: Maybe add as an option.
    void configure_socket() override;
};

/*************************************************************************************************/

}

#endif // JAW_SOCKET_IMPL_H
