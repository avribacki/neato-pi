#ifndef NEATO_SERIAL_PORT_H
#define NEATO_SERIAL_PORT_H

/*************************************************************************************************/

#include <memory>
#include <string>

namespace Neato {

/*************************************************************************************************/

// Helper class used to communicate with Neato serial interface.
class SerialPort
{
public:
    // Create new serial connection with specified port
    static std::shared_ptr<SerialPort> Create(const std::string& port);

    // Destroy and close serial connection.
    virtual ~SerialPort();

    // Send command to serial and read reasult.
    virtual std::string execute(const std::string& command) = 0;
};

/*************************************************************************************************/

}

#endif // NEATO_SERIAL_PORT_H
