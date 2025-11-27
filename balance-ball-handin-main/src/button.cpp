#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <iostream>
#include "broker.hpp"
#include "button.hpp"

Button::Button(std::string path_name, int gpio) : gpio(gpio)
{
    /*
     * TODO: 
     * Request GPIO Inputs
     * - Open gpio device
     */

    isActive = true;
}

Button::~Button()
{
    isActive = false;
    /*
     * Close File descriptors
     */
}

void Button::ButtonThread()
{
    while (isActive)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        /*
         * TODO:
         * Read value from button device node (Posix)
         * Encode value to message data string
         * Create a message
         * Publish the message
         */

    }
}
