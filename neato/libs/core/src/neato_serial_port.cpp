#include "neato_serial_port.hpp"

#include <fstream>
#include <stdexcept>
#include <iostream>
#include "jaw_exception.hpp"

#define SIMULATED 1

namespace Neato {

/*************************************************************************************************/

class SerialPortImpl
    : public SerialPort
{
public:

    // Create new serial connection with specified port.
    SerialPortImpl(const std::string& port)
        : output_()
        , input_()
    {
#if !SIMULATED
        system(("screen -dmS neato " + port).c_str());
        system("screen -S neato -X quit");

        output_.open(port);
        input_.open(port);

        if (!output_ || !input_) {
            throw Jaw::Exception(std::errc::not_connected, "Robot is down");
        }
        input_ >> std::noskipws;

#endif
    }

    // Send command to serial and read reasult.
    virtual std::string execute(const std::string& command)
    {
        //std::cout << "CMD: " << command << std::endl;
        std::string result;
#if !SIMULATED
        static char delimiter = char(26);
        std::string echo;
        output_ << command << std::endl;
        std::getline(input_, echo);
        std::getline(input_, result, delimiter);
#endif
        return result;
    }

private:

    // File streams used in serial connection.
    std::ofstream output_;
    std::ifstream input_;
};

/*************************************************************************************************/

std::shared_ptr<SerialPort> SerialPort::Create(const std::string& port)
{
    return std::make_shared<SerialPortImpl>(port);
}

/*************************************************************************************************/

SerialPort::~SerialPort()
{}

/*************************************************************************************************/

}

