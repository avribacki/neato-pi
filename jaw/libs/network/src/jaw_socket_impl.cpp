#include "jaw_socket_impl.hpp"

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>

#include "jaw_guid.hpp"
#include "jaw_protected_call.hpp"

namespace Jaw {

/*************************************************************************************************
 * Socket Monitor *
 *************************************************************************************************/

SocketMonitor::SocketMonitor(zmq::socket_t& socket)
    : zmq::monitor_t()
    , started_(false)
    , connected_(false)
    , listening_(false)
    , condition_()
    , mutex_()
    , thread_(&SocketMonitor::run, this, std::ref(socket))
{
    std::unique_lock<std::mutex> lock(mutex_);

    // Wait for the monitor to be in place. This ensures that we can safely call wait_connection
    // without missing the event. This delay depends on the system scheduler and may timeout if
    // the thread died before the on_monitor_started event was raised.
    if (!condition_.wait_for(lock, std::chrono::seconds(10), [this]() { return started_; })) {
        throw ConnectionException("Could not start socket monitor");
    }
}

/*************************************************************************************************/

SocketMonitor::~SocketMonitor()
{
    std::unique_lock<std::mutex> lock(mutex_);
    zmq::monitor_t::abort();
    thread_.join();
}

/*************************************************************************************************/

bool SocketMonitor::wait_connection(std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait_for(lock, timeout, [this]() { return connected_; });
    return connected_;
}

/*************************************************************************************************/

bool SocketMonitor::wait_listening(std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait_for(lock, timeout, [this]() { return listening_; });
    return listening_;
}

/*************************************************************************************************/

void SocketMonitor::run(zmq::socket_t &socket)
{
    try {
        // Generate random inproc address
        std::string address("inproc://");
        address.append(Guid::generate().to_string());

        monitor(socket, address.c_str(),
                ZMQ_EVENT_CONNECTED | ZMQ_EVENT_LISTENING | ZMQ_EVENT_MONITOR_STOPPED);
    }
    catch (...) {
        // Cannot leak exceptions from another thread.
        // TODO : Whenever a log system is available, report the error.
    }
}

/*************************************************************************************************/

void SocketMonitor::on_monitor_started()
{
    std::unique_lock<std::mutex> lock(mutex_);
    started_ = true;
    condition_.notify_all();
}

/*************************************************************************************************/

void SocketMonitor::on_event_connected(const zmq_event_t &event, const char* addr)
{
    (void) event; (void) addr;
    std::unique_lock<std::mutex> lock(mutex_);
    connected_ = true;
    condition_.notify_all();
}

/*************************************************************************************************/

void SocketMonitor::on_event_listening(const zmq_event_t &event, const char* addr)
{
    (void) event;
    std::unique_lock<std::mutex> lock(mutex_);
    listening_ = true;
    condition_.notify_all();
}

/*************************************************************************************************
 * Helper methods *
 *************************************************************************************************/

// Deleter to be used as free_fn for ZMQ zero-copy mechanism
template<class T>
static void deleter(void*, void* hint)
{
    T* content = static_cast<T*>(hint);
    delete content;
}

// Encapsulates a zmq::message_t as InputBuffer
InputBuffer buffer_from_zmq(std::unique_ptr<zmq::message_t> message)
{
    uint8_t* data = static_cast<uint8_t*>(message->data());
    std::size_t size = message->size();
    void* hint = static_cast<void*>(message.release());
    return InputBuffer(data, size, &deleter<zmq::message_t>, hint);
}

// Encapsulates an OutputBuffer as zmq::message_t
zmq::message_t buffer_to_zmq(OutputBuffer buffer)
{
    std::vector<uint8_t>* data = buffer.release();
    return zmq::message_t(
        &(*data)[0], data->size(),
        deleter<std::vector<uint8_t>>, static_cast<void*>(data));
}

/*************************************************************************************************
 * Base Socket Implementation *
 *************************************************************************************************/

Socket::Impl::Impl(const std::string& address, int zmq_type)
    : context_()
    , socket_()
    , zmq_mutex_()
    , zmq_address_(address)
    , zmq_type_(zmq_type)
    , poll_abort_requester_()
    , poll_abort_listener_()
    , poll_mutex_()
{
    std::size_t found = zmq_address_.find("://");
    std::size_t start = found + 4;
    if (found == std::string::npos) {
        zmq_address_ = "tcp://" + zmq_address_;
        start = 6; // skip protocol declarion
    }
    if (zmq_address_.find(':', start) == std::string::npos) {
        if (zmq_type == ZMQ_REP || zmq_type == ZMQ_PUB) {
            zmq_address_.append(":*");
        } else {
            throw Exception(std::errc::invalid_argument, "Missing port in address");
        }
    }
    std::cout << "Address : " << zmq_address_ << std::endl;
}

/*************************************************************************************************/

Socket::Impl::~Impl()
{
    try {
        close();
    }
    catch (...) {
        // Cannot throw exceptions inside destructor.
        // TODO : Whenever a log system is available, report the error.
    }
}

/*************************************************************************************************/

const std::string& Socket::Impl::address()
{
    return zmq_address_;
}

/*************************************************************************************************/

void Socket::Impl::bind()
{
    // Define an abitrary time to wait for ZMQ to perform underlying bind.
    static auto BIND_TIMEOUT = std::chrono::seconds(5);

    // Release previous socket.
    close();

    // Create new socket infratructure.
    context_ = std::make_unique<zmq::context_t>(1);
    socket_ = std::make_unique<zmq::socket_t>(*context_, zmq_type_);
    SocketMonitor monitor(*socket_);

    // Derived classes may add aditional configurations to the socket.
    configure_socket();

    // Bind ZMQ socket to saved address.
    socket_->bind(zmq_address_.c_str());

    // If could not connect before timeout, throw exception.
    if (!monitor.wait_listening(BIND_TIMEOUT)) {
        close();
        throw ConnectionException("Could not bind to " + zmq_address_);
    }

    update_address();
}

/*************************************************************************************************/

void Socket::Impl::connect()
{
    // Define an abitrary time to wait for ZMQ to perform underlying connection.
    static auto CONNECTION_TIMEOUT = std::chrono::seconds(5);

    // Release previous socket.
    close();

    // Create new socket infratructure.
    context_ = std::make_unique<zmq::context_t>(1);
    socket_ = std::make_unique<zmq::socket_t>(*context_, zmq_type_);
    SocketMonitor monitor(*socket_);

    // Derived classes may add aditional configurations to the socket.
    configure_socket();

    // Connect to end-point.
    socket_->connect(zmq_address_.c_str());

    // If could not connect before timeout, throw exception.
    if (!monitor.wait_connection(CONNECTION_TIMEOUT)) {
        close();
        throw ConnectionException("Could not connect to " + zmq_address_);
    }

    update_address();
}

/*************************************************************************************************/

void Socket::Impl::close()
{
    // Abort any polling operation.
    {
        std::lock_guard<std::mutex> lock(poll_mutex_);
        if (poll_abort_requester_) {
            zmq::message_t request(6);
            ::memcpy(request.data(), "STOP", 5);
            poll_abort_requester_->send(request);
        }
    }

    std::lock_guard<std::recursive_mutex> lock(zmq_mutex_);

    if (poll_abort_requester_) {
        poll_abort_requester_->close();
        poll_abort_requester_.reset();
    }
    if (poll_abort_listener_) {
        poll_abort_listener_->close();
        poll_abort_listener_.reset();
    }
    if (socket_) {
        socket_->close();
        socket_.reset();
    }
    if (context_) {
        context_->close();
        context_.reset();
    }
}

/*************************************************************************************************/

void Socket::Impl::poll(short events)
{
    // Create abort infrastructure if needed
    {
        std::lock_guard<std::mutex> lock(poll_mutex_);

        if (!poll_abort_listener_ || !poll_abort_requester_) {
            poll_abort_requester_ = std::make_unique<zmq::socket_t>(*context_, ZMQ_PAIR);
            poll_abort_listener_ = std::make_unique<zmq::socket_t>(*context_, ZMQ_PAIR);

            // Generate random inproc address
            std::string address("inproc://");
            address.append(Guid::generate().to_string());

            // Bind the listener and connect requester to the address.
            poll_abort_listener_->bind(address.c_str());
            poll_abort_requester_->connect(address.c_str());
        }
    }

    // Wait for message or stop request.
    zmq::pollitem_t items[] = {
        { (void*) *poll_abort_listener_, 0, ZMQ_POLLIN, 0 },
        { (void*) *socket_, 0, events, 0 },
    };

    // Wait indefinitely for events on this socket or abort socket.
    zmq::poll(&items[0], 2, -1);

    // Received an abort request.
    if (items[0].revents & ZMQ_POLLIN) {
        zmq::message_t request;
        poll_abort_listener_->recv(&request);
        throw ConnectionException("Polling aborted");
    }

    //  Received some of the expected events!
    if (items[1].revents & events) {
        return;
    }

    // Something went wrong...
    throw ConnectionException("Unexpected state for polling");
}

/*************************************************************************************************/

void Socket::Impl::update_address()
{
    char buffer[1024]; //make this sufficiently large.
    size_t size = sizeof(buffer);
    socket_->getsockopt(ZMQ_LAST_ENDPOINT, &buffer, &size);
    zmq_address_ = buffer;
}

/*************************************************************************************************
 * Client Socket Implementation *
 *************************************************************************************************/

ClientSocket::Impl::Impl(const std::string& address)
    : Socket::Impl(address, ZMQ_REQ)
{
    connect();
}

/*************************************************************************************************/

void ClientSocket::Impl::configure_socket()
{
    int linger_ms = 0;
    socket_->setsockopt(ZMQ_LINGER, &linger_ms, sizeof(int));
}

/*************************************************************************************************/

InputBuffer ClientSocket::Impl::request(OutputBuffer message, std::chrono::milliseconds timeout)
{
    // Synchronize thread access underlying ZMQ calls.
    // Ensure ZMQ_REQ state machine.
    std::lock_guard<std::recursive_mutex> lock(zmq_mutex_);

    // If we don't have a connection, recreate it
    if (!socket_) {
        connect();
    }

    // Send the data using the ZMQ socket
    socket_->send(buffer_to_zmq(std::move(message)));

    // Receive the reply using the ZMQ socket
    std::unique_ptr<zmq::message_t> reply_msg = std::unique_ptr<zmq::message_t>(new zmq::message_t());
    int timeout_ms = static_cast<int>(timeout.count());
    socket_->setsockopt(ZMQ_RCVTIMEO, &timeout_ms, sizeof(timeout_ms));

    if (socket_->recv(reply_msg.get())) {
        return buffer_from_zmq(std::move(reply_msg));
    } else {
        // Try to recreate connection to provide proper exception.
        connect();
        throw TimeoutException();
    }
}

/*************************************************************************************************
 * Server Socket Implementation *
 *************************************************************************************************/

ServerSocket::Impl::Impl(const std::string& address)
    : Socket::Impl(address, ZMQ_REP)
{
    bind();
}

/*************************************************************************************************/

void ServerSocket::Impl::process(const Work& work)
{
    // Synchronize thread access underlying ZMQ calls.
    // Ensure ZMQ_REP state machine.
    std::lock_guard<std::recursive_mutex> lock(zmq_mutex_);

    // Wait until we have something to receive.
    poll(ZMQ_POLLIN);

    // Receive the request using the ZMQ socket
    zmq::message_t request_msg;
    socket_->recv(&request_msg);
    InputBuffer request(static_cast<uint8_t*>(request_msg.data()), request_msg.size());

    // Use work procedure to get result.
    OutputBuffer result = work(std::move(request));

    // Send result back to client.
    socket_->send(buffer_to_zmq(std::move(result)));
}

/*************************************************************************************************
 * Subscriber Socket Implementation *
 *************************************************************************************************/

SubscriberSocket::Impl::Impl(const std::string& address, const std::string& channel)
    : Socket::Impl(address, ZMQ_SUB)
    , channel_(channel)
{
    connect();
}

/*************************************************************************************************/

void SubscriberSocket::Impl::configure_socket()
{
    // Set the subscriber channel
    socket_->setsockopt(ZMQ_SUBSCRIBE, channel_.data(), channel_.length());
}

/*************************************************************************************************/

InputBuffer SubscriberSocket::Impl::receive()
{
    // Synchronize thread access underlying ZMQ calls.
    std::lock_guard<std::recursive_mutex> lock(zmq_mutex_);

    // Wait until we have something to receive.
    poll(ZMQ_POLLIN);

    // First, read the channel used as envelope.
    zmq::message_t channel_msg;
    socket_->recv(&channel_msg);

    // Second, get the message contents.
    std::unique_ptr<zmq::message_t> contents_msg = std::unique_ptr<zmq::message_t>(new zmq::message_t());
    socket_->recv(contents_msg.get());
    return buffer_from_zmq(std::move(contents_msg));
}

/*************************************************************************************************
 * Publisher Socket Implementation *
 *************************************************************************************************/

PublisherSocket::Impl::Impl(const std::string& address)
    : Socket::Impl(address, ZMQ_PUB)
{
    bind();
}

/*************************************************************************************************/

void PublisherSocket::Impl::configure_socket()
{
    // Reduce HWM to avoid buffering of published messages.
    int send_hwm = 3;
    socket_->setsockopt(ZMQ_SNDHWM, &send_hwm, sizeof(send_hwm));
}

/*************************************************************************************************/

void PublisherSocket::Impl::publish(const std::string& channel, OutputBuffer message)
{
    // Synchronize thread access underlying ZMQ calls.
    std::lock_guard<std::recursive_mutex> lock(zmq_mutex_);

     // Create envelope from channel.
    zmq::message_t envelope(channel.size());
    ::memcpy(envelope.data(), channel.data(), envelope.size());

    // Send message envelope so it can be filtered.
    socket_->send(envelope, ZMQ_SNDMORE);
    // Send the message content.
    socket_->send(buffer_to_zmq(std::move(message)));
}

/*************************************************************************************************/

}
