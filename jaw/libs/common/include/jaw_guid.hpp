#ifndef JAW_GUID_H
#define JAW_GUID_H

#include <string>

namespace Jaw {

/*************************************************************************************************/

// Represents a globally unique identifier (GUID).
class Guid
{
public:
    // Creates a new Empty Guid (all bytes zero).
    Guid();

    // Creates a new Guid using the supplied 16 bytes (128 bits).
    Guid(unsigned char (&bytes)[16]);

    // Checks whether the PcGuid is zeroed (if all bytes are zero).
    bool empty() const;

    // Converts a PcGuid to a string format.
    std::string to_string() const;

    // Generates a new random PcGuid.
    static Guid generate();

    // Generates a new PcGuid from the supplied string.
    // The source string must be in the format "{00000000-0000-0000-0000-000000000000}"
    static Guid from_string(const std::string& str);

private:

    // Byte sequence representing this Guid.
    unsigned char bytes_[16];
};

/*************************************************************************************************/

// Tests whether two Guids are equal.
bool operator == (const Guid& left, const Guid& right);

// Tests whether two Guids are different.
bool operator != (const Guid& left, const Guid& right);

// Test whether the first Guid is less than the second Guid.
bool operator < (const Guid& left, const Guid& right);

/*************************************************************************************************/
}

#endif // JAW_GUID_H
