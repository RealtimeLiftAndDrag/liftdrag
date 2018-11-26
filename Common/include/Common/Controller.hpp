#pragma once



#include "Global.hpp"



namespace Controller {

    constexpr int k_maxPlayerCount(4);

    enum class Stick {
        left, right
    };

    enum class Trigger {
        left, right
    };

    enum class Button {
        a, b, x, y,
        dpU, dpD, dpL, dpR,
        lShoulder, rShoulder,
        lThumb, rThumb,
        start, back
    };

    struct State {
         bool connected; // whether or not the controller is connected
         vec2    lStick; // left stick - <-1, -1> is left bottom, <1, 1> is right top
         vec2    rStick; // right stick - <-1, -1> is left bottom, <1, 1> is right top
        float  lTrigger; // left trigger - 0 is unpressed, 1 is fully pressed
        float  rTrigger; // right trigger - 0 is unpressed, 1 is fully pressed
        ivec2      dpad; // dpad - <-1, -1> is left bottom, <1, 1> is right top
         bool         a; // a button - false is unpressed, true is pressed
         bool         b; // b button - false is unpressed, true is pressed
         bool         x; // x button - false is unpressed, true is pressed
         bool         y; // y button - false is unpressed, true is pressed
         bool       dpU; // dpad up button - false is unpressed, true is pressed
         bool       dpD; // dpad down button - false is unpressed, true is pressed
         bool       dpL; // dpad left button - false is unpressed, true is pressed
         bool       dpR; // dpad right button - false is unpressed, true is pressed
         bool lShoulder; // left shoulder button - false is unpressed, true is pressed
         bool rShoulder; // right shoulder button - false is unpressed, true is pressed
         bool    lThumb; // left thumb button - false is unpressed, true is pressed
         bool    rThumb; // right thumb button - false is unpressed, true is pressed
         bool     start; // start button - false is unpressed, true is pressed
         bool      back; // back button - false is unpressed, true is pressed
    };

    // Must be called once before calling any of the accessors
    void poll(int nPlayers);

    // Whether or not the controller is connected. Must have called poll prior
    bool connected(int player);

    const State & state(int player);

    // Set callbacks
    void   stickCallback(void (*callback)(int,   Stick,  vec2));
    void triggerCallback(void (*callback)(int, Trigger, float));
    void    dpadCallback(void (*callback)(int,          ivec2));
    void  buttonCallback(void (*callback)(int,  Button,  bool));

};