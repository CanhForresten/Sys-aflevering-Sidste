#include <thread>
#include <iostream>
#include "display.hpp"
#include "bitmaps.hpp"
#include "message.hpp"
#include "broker.hpp"
#include "game_control.hpp"
#include "game_state.hpp"


static const int ballCenterPosX = 64;
static const int ballCenterPosY = 16;

GameControl::GameControl(Display& display) : display(&display) {
    gameState.ball_x = ballCenterPosX;
    gameState.ball_y = ballCenterPosY;
    gameState.score = 0;
    display.drawDisplay(gameState);
}

void GameControl::handleAccelerometer(const std::string& data) {
    const int speed = 2;
    double x, y, z;

    /*
     * TODO:
     * Decode the message data
     * Update gameState ball_x, ball_y according to accl values
     * Update gameState.score according to accl values
     * Draw display
     * Activity 5: publish 'boundary' messsage if ball out of screen
     */

}

void GameControl::handleButton(const std::string& data) {
    int gpio, value;

    Message::decodeButtonData(data, &gpio, &value);
    if (value == 0) 
    {
        std::cout << "RESET" << std::endl;
        /*
         * TODO:
         * Center ball
         * Reset score
         */
    }
}

/*
* TODO:
* Create handler for 'boundary' messages
* Subtract score by 1000 if the data value is "1"
*/


void GameControl::onMessage(const Message& msg){

    if (msg.topic.compare("btn") == 0)
        handleButton(msg.data);
}