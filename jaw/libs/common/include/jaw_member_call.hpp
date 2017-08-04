#ifndef JAW_MEMBER_CALL_H
#define JAW_MEMBER_CALL_H

#include <system_error>
#include "jaw_protected_call.hpp"

namespace Jaw {

/*************************************************************************************************/

// Call specified method using supplied handle instance and arguments.
// The ret variable will be updated with the value returned from the method.
template <class Object, class MethodRet, class Ret, class... Params, class... Args>
static typename std::enable_if<!std::is_void<MethodRet>::value, int>::type
member_call(void* handle, MethodRet (Object::*method)(Params...), Ret* ret, Args&&... args)
{
    Object* object = static_cast<Object*>(handle);
    if (!object || !ret) {
        return static_cast<int>(std::errc::invalid_argument);
    }

    return protected_call([&object, &method, &ret, &args...]() {
        *ret = std::bind(method, object, std::forward<Args>(args)...)();
        return 0;
    });
}

/*************************************************************************************************/

// Special case when return is void.
template <class Object, class... Params, class... Args>
static int member_call(void* handle, void (Object::*method)(Params...), Args&&... args)
{
    Object* object = static_cast<Object*>(handle);
    if (!object) {
        return static_cast<int>(std::errc::invalid_argument);
    }

    return protected_call([&object, &method, &args...]() {
        std::bind(method, object, std::forward<Args>(args)...)();
        return 0;
    });
}

/*************************************************************************************************/

}

#endif // JAW_MEMBER_CALL_H
