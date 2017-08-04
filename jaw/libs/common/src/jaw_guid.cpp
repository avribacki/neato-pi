#include "jaw_guid.hpp"

#include <vector>
#include <iomanip>
#include <sstream>
#include <random>
#include <chrono>
#include <memory>
#include <cstring>

namespace Jaw {

/*************************************************************************************************/

enum class Endianess { LITTLE, BIG };
struct block_info { int size; Endianess order; };

static const std::vector<block_info>& guid_specification() {
    static const std::vector<block_info> spec = {
        { 4, Endianess::LITTLE },
        { 2, Endianess::LITTLE },
        { 2, Endianess::LITTLE },
        { 2, Endianess::BIG },
        { 6, Endianess::BIG }
    };
    return spec;
}

/*************************************************************************************************/

Guid::Guid()
{
    std::fill(bytes_, bytes_ + sizeof(bytes_), 0);
}

/*************************************************************************************************/

Guid::Guid(unsigned char (&bytes)[16])
{
    std::copy(bytes, bytes + sizeof(bytes_), bytes_);
}

/*************************************************************************************************/

bool Guid::empty() const
{
    static const unsigned char empty[16] = { 0 };
    return std::memcmp(this, empty, sizeof(Guid)) == 0;
}

/*************************************************************************************************/

std::string Guid::to_string() const
{
    std::stringstream guid_str;
    unsigned char* bytes = const_cast<unsigned char*>(bytes_);

    guid_str << "{";
    guid_str << std::hex << std::uppercase << std::setfill<char>('0');

    for (auto& block : guid_specification()) {
        unsigned char* memory = (block.order == Endianess::LITTLE ? (bytes + block.size - 1) : bytes);
        int step = (block.order == Endianess::LITTLE ? -1 : +1);
        for (int i = 0; i < block.size; i++) {
            guid_str << std::setw(2) << static_cast<int>(*memory);
            memory += step;
        }
        guid_str << "-";
        bytes += block.size;
    }
    std::string result = guid_str.str();
    result.back() = '}'; // Replace last '-' by '}'

    return result;
}

/*************************************************************************************************/

Guid Guid::generate()
{
    // Create static generator for uniform random distribution
    static unsigned int timestamp = static_cast<unsigned int>(
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    static std::default_random_engine generator(timestamp);
    static std::uniform_int_distribution<int> distribution(0, 255);

    Guid result;

    // Fill all 128 bits randomly
    for (int i = 0; i < sizeof(result.bytes_); i++) {
        result.bytes_[i] = distribution(generator);
    }

    // Specify variant (rfc4122): two most significant bits of octet 8 of the UUID as '10'
    result.bytes_[8] &= 0x3F;
    result.bytes_[8] |= 0x80;

    // Specify version 4 (random): first nibble of octet 6
    result.bytes_[7] &= 0x0F;
    result.bytes_[7] |= 0x40;

    return result;
}

/*************************************************************************************************/

Guid Guid::from_string(const std::string& str)
{
    Guid result;
    const char* guid_str = str.c_str();
    unsigned char* bytes = result.bytes_;

    wchar_t byte_str[3];
    byte_str[2] = '\0';

    // Can optionally begin with {
    if (guid_str[0] == '{') guid_str++;

    for (auto& block : guid_specification()) {
        unsigned char* memory = (block.order == Endianess::LITTLE ? (bytes + block.size - 1) : bytes);
        int step = (block.order == Endianess::LITTLE ? -1 : +1);
        for (int i = 0; i < block.size; i++) {
            std::copy(guid_str, guid_str + 2, byte_str);
            *memory = static_cast<unsigned char>(wcstol(byte_str, nullptr, 16));
            memory += step;
            guid_str += 2;
        }
        // Verify block finished as expected
        if (guid_str[0] != '-' && guid_str[0] != '}' && guid_str[0] != '\0') {
            throw std::runtime_error("Invalid argument");
        }
        bytes += block.size;
        // Skip block separator
        guid_str += 1;
    }

    return result;
}

/*************************************************************************************************/

bool operator == (const Guid& left, const Guid& right)
{
    return std::memcmp(&left, &right, sizeof(Guid)) == 0;
}

bool operator != (const Guid& left, const Guid& right)
{
    return !(left == right);
}

bool operator < (const Guid& left, const Guid& right)
{
    return std::memcmp(&left, &right, sizeof(Guid)) < 0;
}

/*************************************************************************************************/

}
