#ifndef JAW_EXCEPTION_H
#define JAW_EXCEPTION_H

#include <exception>
#include <system_error>
#include <string>

namespace Jaw {

/*************************************************************************************************/

// Exception type thrown by Jaw library
class Exception : public std::system_error
{
public:
    // Create new Exception with supplied code and detailed information.
    Exception(std::errc code, const std::string& detail = "");
};

/*************************************************************************************************/

}

#endif // JAW_EXCEPTION_H
