#ifndef JAW_PROTECTED_CALL_H
#define JAW_PROTECTED_CALL_H

#include <functional>

namespace Jaw {

/*************************************************************************************************/

// Wrap function call to capture all exceptions.
// Method should return zero if succeeded or an errno value otherwise.
int protected_call(std::function<int()> method);

/*************************************************************************************************/

}

#endif // JAW_PROTECTED_CALL_H
