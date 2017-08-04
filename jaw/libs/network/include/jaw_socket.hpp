#ifndef JAW_SOCKET_H
#define JAW_SOCKET_H

#include <memory>
#include <chrono>
#include <functional>
#include <stdexcept>
#include <cerrno>

#include "jaw_exception.hpp"
#include "jaw_serialization.hpp"

namespace Jaw {

/*************************************************************************************************
 * Exceptions thrown by the Socket *
 *************************************************************************************************/

class TimeoutException
    : public Exception
{
public:
    TimeoutException()
        : Exception(std::errc::timed_out)
    {}
};

class ConnectionException
    : public Exception
{
public:
    ConnectionException(const std::string& detail)
        : Exception(std::errc::not_connected, detail)
    {}
};

/*************************************************************************************************
 * Base Socket *
 *************************************************************************************************/

// Generic socket representation.
// Each derived socket will create and configure the Impl differently.
class Socket
{
public:
    // Destroys socket.
    virtual ~Socket();

    // Gets the address to which this socket is bounded (connected or listening).
    // Useful to identify listening port when it was automatically defined (passed '*' as port).
    // It will contain the chosen transport and port if they were not supplied.
   const std::string& address();

   // Close the socket aborting any blocking operations which will throw ConnectionException.
   void close();

protected:

    // Disable generic socket instantiation.
    Socket();

    // Bridge Design Pattern
    // It is responsibility of derived classes to initialize with the correct implementation.
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/*************************************************************************************************
 * Client Socket *
 *************************************************************************************************/

class ClientSocket
    : public Socket
{
public:
    // Create new client socket that will connect to specified address.
    // Address should be in the form "[transport://]address:port". TCP is the default transport.
    ClientSocket(const std::string& address);

    // Send data and receive reply as input buffer.
    // An exception is thrown if no reply is received before timeout.
    InputBuffer request(OutputBuffer message, std::chrono::milliseconds timeout);

protected:
    // Client socket implementation
    class Impl;
};

/*************************************************************************************************
 * Server Socket *
 *************************************************************************************************/

class ServerSocket
    : public Socket
{
public:
    // Define work method signature
    using Work = std::function<OutputBuffer(InputBuffer)>;

    // Create new server socket that will bind to specified address.
    // If address is "*", listen to all IP address using TCP on random free port.
    ServerSocket(const std::string& address);

    // Wait indefinitely for incomming request and call work method passing received data.
    // The method should return a valid OutputBuffer to be sent as reply.
    // Throw ConnectionException when socket is destructed or closed.
    void process(const Work& work);

protected:
    // Server socket implementation
    class Impl;
};

/*************************************************************************************************
 * Subscriber Socket *
 *************************************************************************************************/

class SubscriberSocket
    : public Socket
{
public:
    // Create new subscriber socket that will connect to specified address
    // and receive messages from specified channel.
    SubscriberSocket(const std::string& address, const std::string& channel);

    // Receive raw data from connection waiting indefinitely.
    // Throw ConnectionException when socket is destructed or closed.
    InputBuffer receive();

protected:
    // Subscriber socket implementation
    class Impl;
};

/*************************************************************************************************
 * Publisher Socket *
 *************************************************************************************************/

class PublisherSocket
    : public Socket
{
public:
    // Create new publisher socket that will bind to specified address.
    // If address is "*", listen to all IP address using TCP on random free port.
    PublisherSocket(const std::string& address);

    // Publish message to specified channel.
    void publish(const std::string& channel, OutputBuffer message);

protected:
    // Publisher socket implementation
    class Impl;
};

/*************************************************************************************************/

}

#endif // JAW_SOCKET_H
