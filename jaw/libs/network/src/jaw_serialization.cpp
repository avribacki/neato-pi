#include "jaw_serialization.hpp"

#include <stdexcept>

namespace Jaw {

/*************************************************************************************************/

OutputBuffer::OutputBuffer()
    : data_(std::unique_ptr<std::vector<uint8_t>>(new std::vector<uint8_t>()))
{}

OutputBuffer::OutputBuffer(OutputBuffer&& other)
    : data_(std::move(other.data_))
{}

/*************************************************************************************************/

OutputBuffer::~OutputBuffer()
{}

/*************************************************************************************************/

void OutputBuffer::write(const uint8_t* source, std::size_t size)
{
    if (!data_) {
        throw Exception(std::errc::operation_not_permitted, "Can't write to buffer after conversion to zmq::message_t");
    }
    data_->insert(data_->end(), source, source + size);
}

/*************************************************************************************************/

std::vector<uint8_t>* OutputBuffer::release()
{
    return data_.release();
}

/*************************************************************************************************/

void write(OutputBuffer& buffer) {}

/*************************************************************************************************/

InputBuffer::InputBuffer(uint8_t* data, std::size_t size)
    : data_(data)
    , read_bytes_(0)
    , max_bytes_(size)
    , deleter_(nullptr)
    , hint_(nullptr)
{}

/*************************************************************************************************/

InputBuffer::InputBuffer(uint8_t* data, std::size_t size, Deleter* deleter, void* hint)
    : data_(data)
    , read_bytes_(0)
    , max_bytes_(size)
    , deleter_(deleter)
    , hint_(hint)
{}

/*************************************************************************************************/

InputBuffer::InputBuffer(InputBuffer&& other)
    : data_(other.data_)
    , read_bytes_(other.read_bytes_)
    , max_bytes_(other.max_bytes_)
    , deleter_(other.deleter_)
    , hint_(other.hint_)
{
    // We took ownership of the buffer. Don't let other release it.
    other.deleter_ = nullptr;
}

/*************************************************************************************************/

InputBuffer::~InputBuffer()
{
    // If this input buffer was create with an associated deleter, call it now.
    if (deleter_ != nullptr) {
        deleter_(data_, hint_);
    }
}

/*************************************************************************************************/

void* InputBuffer::read(std::size_t size)
{
    if (read_bytes_ + size > max_bytes_) {
        throw Exception(std::errc::result_out_of_range, "Trying to access outside limits of buffer");
    }
    void* current = static_cast<void*>(data_ + read_bytes_);
    // Move reading position
    read_bytes_ += size;
    return current;
}

/*************************************************************************************************/

void read(InputBuffer& buffer) {}

/*************************************************************************************************/

}
