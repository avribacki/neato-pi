/*************************************************************************************************
* Serialization - Serialization framework for fast network messaging.
*
* This file implements the serialization functions for reading/writing buffers message buffers.
*
* It uses C++11 variadic templates to define read/write methods that receive a variable number
* of arguments with variable types. Using function overloads, different read/write functions are
* implemented for the possible argument types.
*
* To write data in order to be sent, first create a new OutputBuffer (it can be stack allocated)
* and pass it to write functions with the parameters that should be serialized:
* 
* OutputBuffer buffer;
* write(buffer, 10, 20.0, std::string("something"));
*
* The correct function is selected using templates automatic type deduction. In this case, the
* called method would be write<int, float, std::string>(...). As most variadic templates, this
* solution uses recursion (at compilation time). So, write<int, float, std::string>() will write
* the integer into the buffer then call write<float, std::string>() to write the float, which  will
* call write<std::string>() to write the string, and lastly, call an empty termination function write().
*
* When the buffer contains all desired data, it can be retrieved using "release" method.
* This won't copy any data from the buffer and the pointer can be used in a zero-copy mechanism like
* provided by ZMQ. Once released, the buffer becomes invalid and cannot be extended.
*
* To read received data, the process is similar. We create an InputBuffer from raw bytes and
* pass it to the read functions. The supplied buffer is freed only if a deleter function is supplied.
* 
* For example, we can deserialize the previous example with the following code:
*
* InputBuffer buffer(zmqMessage);
* int aInteger;
* double aDouble;
* std::string aString;
* read(buffer, aInteger, aDouble, aString);
*
* As for writing, the correct function is selected using template type deduction and the read method
* is created using compile time recursion.
*
* This document contains the overloads for read/write functions of basic types, containers and strings.
* Overload for user-defined types are defined by the "protocol" module of each library.
*
**************************************************************************************************/

#ifndef JAW_SERIALIZATION_H
#define JAW_SERIALIZATION_H

#include <vector>
#include <memory>
#include <iterator>
#include <tuple>
#include <stdexcept>

#include <cstdint>
#include <cstring>

#include "jaw_guid.hpp"
#include "jaw_exception.hpp"

namespace Jaw {


/**************************************************************************************************
 * Write-only buffer *
 *************************************************************************************************/

// Class used as output buffer for serialization (write) function.
class OutputBuffer
{
public:
    // Create new output buffer.
    OutputBuffer();

    // Move constructor
    OutputBuffer(OutputBuffer&& other);

    // Disable copy
    OutputBuffer(const OutputBuffer&) = delete;
    OutputBuffer& operator= (const OutputBuffer&) = delete;

    // Destroy buffer.
    // Internal memory is freed if "release" method was not called.
    ~OutputBuffer();

    // Copy size bytes from source pointer into internal memory.
    void write(const uint8_t* source, std::size_t size);

    // Get the internal data associated releasing its ownership.
    std::vector<uint8_t>* release();

private:
    // Create internal memory as std::vector of bytes (uint8_t).
    std::unique_ptr<std::vector<uint8_t>> data_;
};

/**************************************************************************************************
* Read-only buffer  *
**************************************************************************************************/

// Class used as input buffer for deserialization (read) function.
class InputBuffer
{
public:
    typedef void (Deleter) (void *data, void *hint);

    // Create an InputBuffer from a raw data.
    // As we only keeps a reference to the data, it must remain valid for the lifetime of
    // the input buffer. If it is destroyed earlier, the read method will access invalid memory.
    InputBuffer(uint8_t* data, std::size_t size);

    // Create an InputBuffer from a raw data and get ownership of it.
    // The deleter will be called passing the data and the hint as argument.
    InputBuffer(uint8_t* data, std::size_t size, Deleter* deleter, void* hint = nullptr);

    // Move constructor
    InputBuffer(InputBuffer&& other);

    // Disable copy
    InputBuffer(const InputBuffer&) = delete;
    InputBuffer& operator= (const InputBuffer&) = delete;

    // Destructor.
    // Data is released only if deleter was provided.
    ~InputBuffer();

    // Return current memoy location in the buffer as void* and advance reading position by
    // size bytes. Note that noy memory is deallocated or modified.
    void* read(std::size_t size);

private:
    uint8_t* data_;
    std::size_t read_bytes_;
    std::size_t max_bytes_;
    Deleter* deleter_;
    void* hint_;
};

/*************************************************************************************************/
// Termination functions for template recursion

void write(OutputBuffer& buffer);
void read(InputBuffer& buffer);

/*************************************************************************************************/
// Fundamental types (int, double, float, ...)

template<class T, class... Args>
typename std::enable_if<std::is_fundamental<T>::value, void>::type
write(OutputBuffer& buffer, const T& value, const Args&... args)
{
    buffer.write(reinterpret_cast<const uint8_t*>(&value), sizeof(T));
    write(buffer, args...);
}

template<class T, class... Args>
typename std::enable_if<std::is_fundamental<T>::value, void>::type
read(InputBuffer& buffer, T& value, Args&... args)
{
    value = *(reinterpret_cast<T*>(buffer.read(sizeof(T))));
    read(buffer, args...);
}

/*************************************************************************************************/
// Handle enums as 32bit integers

template<class T, class... Args>
typename std::enable_if<std::is_enum<T>::value, void>::type
write(OutputBuffer& buffer, const T& value, const Args&... args)
{
    write(buffer, static_cast<std::int32_t>(value));
    write(buffer, args...);
}

template<class T, class... Args>
typename std::enable_if<std::is_enum<T>::value, void>::type
read(InputBuffer& buffer, T& value, Args&... args)
{
    std::int32_t as_integer;
    read(buffer, as_integer);
    value = static_cast<T>(as_integer);
    read(buffer, args...);
}

/*************************************************************************************************/
// Handle bools as 32bit integers containing either 1 or 0

template<class... Args>
void write(OutputBuffer& buffer, const bool& value, const Args&... args)
{
    write(buffer, std::int32_t(value ? 1 : 0));
    write(buffer, args...);
}

template<class... Args>
void read(InputBuffer& buffer, bool& value, Args&... args)
{
    std::int32_t as_integer;
    read(buffer, as_integer);
    value = (as_integer != 0);
    read(buffer, args...);
}

/*************************************************************************************************/
// Continuous arrays

template<class T, std::int32_t N, class... Args>
void write(OutputBuffer& buffer, const T(&value)[N], const Args&... args)
{
    // First write the array length as 32bit integer
    write(buffer, N);

    // Write continuous raw data
    if (N > 0) {
        buffer.write(reinterpret_cast<const uint8_t*>(value), static_cast<std::size_t>(sizeof(T) * N));
    }

    write(buffer, args...);
}

template<class T, size_t N, class... Args>
void read(InputBuffer& buffer, T(&value)[N], const Args&... args)
{
    std::int32_t length;

    // First read the array length as 32bit integer
    read(buffer, length);

    // Read continuous raw data
    if (length == N) {
        // Move reader without copying any data.
        T* tmp = reinterpret_cast<T*>(buffer.read(static_cast<std::size_t>(sizeof(T) * N)));
        // Copy the data to the array.
        ::memcpy(value, tmp, static_cast<std::size_t>(sizeof(T) * N));
    } else {
        throw Exception(std::errc::bad_message, "Received array with unexpected size");
    }

    read(buffer, args...);
}

/*************************************************************************************************/
// Iterable types

template<class T, class... Args>
void write_iterable(OutputBuffer& buffer, const T& iterable, const Args&... args)
{
    // First write the iterable length as 32bit integer
    write(buffer, static_cast<std::int32_t>(iterable.size()));

    // Write each element
    for (const typename T::value_type& value : iterable) {
        write(buffer, value);
    }

    write(buffer, args...);
}

template<class T, class... Args>
void read_iterable(InputBuffer& buffer, T& iterable, Args&... args)
{
    std::int32_t length;
    read(buffer, length);
    iterable.reserve(static_cast<std::size_t>(length));

    for (std::int32_t i = 0; i < length; i++) {
        typename T::value_type value;
        read(buffer, value);
        iterable.push_back(value);
    }

    read(buffer, args...);
}

/*************************************************************************************************/
// STL std::vector, std::list (and others) using [write|read]_iterable

template<template<class, class> class V, class T, class... Args>
void write(OutputBuffer& buffer, const V<T, std::allocator<T>>& container, const Args&... args)
{
    write_iterable(buffer, container, args...);
}

template<template<class, class> class V, class T, class... Args>
void read(InputBuffer& buffer, V<T, std::allocator<T>>& container, Args&... args)
{
    read_iterable(buffer, container, args...);
}

/*************************************************************************************************/
// STL std::basic_string<T> using [write|read]_iterable

template<class T, class... Args>
void write(OutputBuffer& buffer, const std::basic_string<T, std::char_traits<T>, std::allocator<T> >& str, const Args&... args)
{
    write_iterable(buffer, str, args...);
}

template<class T, class... Args>
void read(InputBuffer& buffer, std::basic_string<T, std::char_traits<T>, std::allocator<T> >& str, Args&... args)
{
    read_iterable(buffer, str, args...);
}

/*************************************************************************************************/
// Jaw::Guid class

template<class... Args>
void write(OutputBuffer& buffer, const Guid& value, const Args&... args)
{
    buffer.write((uint8_t* const)& value, sizeof(value));
    write(buffer, args...);
}

template<class... Args>
void read(InputBuffer& buffer, Guid& value, Args&... args)
{
    // Get pointer to InputBuffer and forward reading position. No copy is made.
    unsigned char (&bytes)[16] = *reinterpret_cast<unsigned char(*)[16]>(buffer.read(sizeof(Guid)));
    value = Guid(bytes);
    read(buffer, args...);
}

/*************************************************************************************************
 * NOTE: The following code implements something like the std::index_sequence_for available
 * only in C++14 <utility> header. Use the std version once available.
 *************************************************************************************************/

/// A type that represents a parameter pack of zero or more integers.
template<std::size_t... Indexes>
struct index_sequence
{
    /// Generate an index_tuple with an additional element.
    template<unsigned N>
    using next = index_sequence<Indexes..., N>;
};

/// Unary metafunction that generates an index_tuple containing [0, Size)
template<std::size_t Size>
struct make_index_sequence
{
    typedef typename make_index_sequence<Size - 1>::type::template next<Size - 1>  type;
};

// Terminal case of the recursive metafunction.
template<>
struct make_index_sequence<0u>
{
    typedef index_sequence<> type;
};

/*************************************************************************************************/
// STL std::tuple

// Entry point
template<class... Args>
void write(OutputBuffer& buffer, const std::tuple<Args...>& value)
{
    write_tuple(buffer, value, typename make_index_sequence<sizeof...(Args)>::type());
}

// Write expanded tuple
template<class Tuple, std::size_t... I>
void write_tuple(OutputBuffer& buffer, const Tuple& value, index_sequence<I...>)
{
    write(buffer, std::get<I>(value)...);
}

// Entry point
template<class... Args>
void read(InputBuffer& buffer, std::tuple<Args...>& value)
{
    read_tuple(buffer, value, typename make_index_sequence<sizeof...(Args)>::type());
}

// Read expanded tuple
template<class Tuple, std::size_t... I>
void read_tuple(InputBuffer& buffer, Tuple& value, index_sequence<I...>)
{
    read(buffer, std::get<I>(value)...);
}

/**************************************************************************************************/

}

#endif // JAW_SERIALIZATION_H
