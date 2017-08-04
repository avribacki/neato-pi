#include "jaw_protected_call.hpp"
#include "jaw_exception.hpp"

#include <iostream>

namespace Jaw {

/*************************************************************************************************/

// Wrap function call to capture all exceptions.
int protected_call(std::function<int()> method)
{
    try {
        return method();
    }
    catch (const std::system_error& e) {
        std::cout << e.what() << std::endl;
        return e.code().value();
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return static_cast<int>(std::errc::state_not_recoverable);
    }
    catch (...) {
        return static_cast<int>(std::errc::state_not_recoverable);
    }
}

/*************************************************************************************************/

}
