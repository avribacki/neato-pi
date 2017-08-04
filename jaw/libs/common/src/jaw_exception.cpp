#include "jaw_exception.hpp"

namespace Jaw {

/*************************************************************************************************/

Exception::Exception(std::errc code, const std::string& detail)
    : system_error(std::make_error_code(code), detail)
{}

/*************************************************************************************************/

}
