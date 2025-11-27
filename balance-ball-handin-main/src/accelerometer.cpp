#include <cstdint>
#include <cstring>
#include <thread>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "broker.hpp"
#include "accelerometer.hpp"

// Initialize SPI bus
void Accelerometer::initSPI(std::string path_name) {
    /*
     * TODO: See exercise text
     */
}

// Read from register
int Accelerometer::readReg(uint8_t reg, uint8_t nbytes) {
    /*
     * TODO: See exercise text
     */
  return 0; // Update this line also!!
}

// Write to register
int Accelerometer::writeReg(uint8_t reg, uint8_t data) {
    /*
     * TODO: See exercise text
     */
  return 0; // Update this line also!!
}

//Turn on accelerometer
void Accelerometer::startAccel(void) {
    /*
     * TODO: See exercise text
     */
}

// Check if accelerometer data is available
bool Accelerometer::isAccelDataAvailable(void) {
    /*
     * TODO: See exercise text
     */
    return false; // Update this line also!!
}

// Read accelerometer data (6 bytes)
void Accelerometer::readAccel(void) {
    /*
     * TODO: See exercise text
     */
}

Accelerometer::Accelerometer(std::string path_name)
{
    /* 
     * TODO:
     * `initSPI` and `startAccel`
     */

    isActive = true;
}

Accelerometer::~Accelerometer()
{
    isActive = false;
    /*
     * TODO:
     * Close File descriptors
     */
}

void Accelerometer::accelerometerThread()
{
    char buf[16];
    while (isActive)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Check if Accelerometer data is available
        // in while loop. Sleep for 100 
        // microseconds, if not

        // Read acceleration

        // Convert to doubles (x,y,z)

        // Encode data

        // Create message & publish
    }
}
