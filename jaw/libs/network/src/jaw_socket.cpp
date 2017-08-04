#include "jaw_socket.hpp"
#include "jaw_socket_impl.hpp"

namespace Jaw {

/*************************************************************************************************
 * Base Socket *
 *************************************************************************************************/

Socket::Socket()
    : pimpl_()
{}

/*************************************************************************************************/

Socket::~Socket()
{
    pimpl_.reset();
}

/*************************************************************************************************/

const std::string& Socket::address()
{
    if (!pimpl_) throw ConnectionException("Closed");
    return pimpl_->address();
}

/*************************************************************************************************/

void Socket::close()
{
    pimpl_.reset();
}

/*************************************************************************************************/

ClientSocket::ClientSocket(const std::string& address)
{
    pimpl_ = std::make_unique<ClientSocket::Impl>(address);
}

/*************************************************************************************************/

InputBuffer ClientSocket::request(OutputBuffer message, std::chrono::milliseconds timeout)
{
    ClientSocket::Impl* pimpl = dynamic_cast<ClientSocket::Impl*>(pimpl_.get());
    if (!pimpl) throw ConnectionException("Closed");
    return pimpl->request(std::move(message), timeout);
}

/*************************************************************************************************/

ServerSocket::ServerSocket(const std::string& address)
{
    pimpl_ = std::make_unique<ServerSocket::Impl>(address);
}

/*************************************************************************************************/

void ServerSocket::process(const Work& work)
{
    ServerSocket::Impl* pimpl = dynamic_cast<ServerSocket::Impl*>(pimpl_.get());
    if (!pimpl) throw ConnectionException("Closed");
    return pimpl->process(work);
}

/*************************************************************************************************/

SubscriberSocket::SubscriberSocket(const std::string& address, const std::string& channel)
{
    pimpl_ = std::make_unique<SubscriberSocket::Impl>(address, channel);
}

/*************************************************************************************************/

InputBuffer SubscriberSocket::receive()
{
    SubscriberSocket::Impl* pimpl = dynamic_cast<SubscriberSocket::Impl*>(pimpl_.get());
    if (!pimpl) throw ConnectionException("Closed");
    return pimpl->receive();
}

/*************************************************************************************************/

PublisherSocket::PublisherSocket(const std::string& address)
{
    pimpl_ = std::make_unique<PublisherSocket::Impl>(address);
}

/*************************************************************************************************/

void PublisherSocket::publish(const std::string& channel, OutputBuffer message)
{
    PublisherSocket::Impl* pimpl = dynamic_cast<PublisherSocket::Impl*>(pimpl_.get());
    if (!pimpl) throw ConnectionException("Closed");
    pimpl->publish(channel, std::move(message));
}

/*************************************************************************************************/

}
