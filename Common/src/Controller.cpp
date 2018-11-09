#include "Controller.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <XInput.h>

#pragma comment(lib, "XInput.lib")



namespace Controller {

    static State s_states[k_maxPlayerCount];

    static void (*s_stickCallback)(int, Stick, vec2);
    static void (*s_triggerCallback)(int, Trigger, float);
    static void (*s_dpadCallback)(int, ivec2);
    static void (*s_buttonCallback)(int, Button, bool);

    static float stickVal(s16 v) {
        constexpr float k_posNormFactor(1.0f / 32767.0f), k_negNormFactor(1.0f / 32768.0f);
        constexpr float k_threshold(0.2f);
        constexpr float k_invFactor(1.0f / (1.0f - k_threshold));

        float fv(v >= 0 ? v * k_posNormFactor : v * k_negNormFactor);
        if (fv >= +k_threshold) return (fv - k_threshold) * k_invFactor;
        if (fv <= -k_threshold) return (fv + k_threshold) * k_invFactor;
        return 0.0f;
    }

    static float triggerVal(u08 v) {
        constexpr float k_normFactor(1.0f / 255.0f);
        constexpr float k_threshold(0.1f);
        constexpr float k_invFactor(1.0f / (1.0f - k_threshold));

        float fv(v * k_normFactor);
        return fv >= k_threshold ? (fv - k_threshold) * k_invFactor : 0.0f;
    }

    static ivec2 dpadVal(WORD buttons) {
        ivec2 dpad;
        if (buttons & XINPUT_GAMEPAD_DPAD_LEFT ) ++dpad.x;
        if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) --dpad.x;
        if (buttons & XINPUT_GAMEPAD_DPAD_DOWN ) ++dpad.y;
        if (buttons & XINPUT_GAMEPAD_DPAD_UP   ) --dpad.y;
        return dpad;
    }

    static void pollPlayer(int player) {
        State & state(s_states[player]);        

        XINPUT_STATE xboxState;
        if (!(state.connected = !XInputGetState(player, &xboxState))) {
            return;
        }
        XINPUT_GAMEPAD & gamepad(xboxState.Gamepad);
        WORD buttons(gamepad.wButtons);

        // Save previous state
        State prevState(state);

        // Update state
        state. lStick.x = stickVal(gamepad.sThumbLX);
        state. lStick.y = stickVal(gamepad.sThumbLY);
        state. rStick.x = stickVal(gamepad.sThumbRX);
        state. rStick.y = stickVal(gamepad.sThumbRY);
        state. lTrigger = triggerVal(gamepad.bLeftTrigger);
        state. rTrigger = triggerVal(gamepad.bRightTrigger);
        state.     dpad = dpadVal(buttons);
        state.        a = buttons & XINPUT_GAMEPAD_A;
        state.        b = buttons & XINPUT_GAMEPAD_B;
        state.        x = buttons & XINPUT_GAMEPAD_X;
        state.        y = buttons & XINPUT_GAMEPAD_Y;
        state.      dpU = buttons & XINPUT_GAMEPAD_DPAD_UP;
        state.      dpD = buttons & XINPUT_GAMEPAD_DPAD_DOWN;
        state.      dpL = buttons & XINPUT_GAMEPAD_DPAD_LEFT;
        state.      dpR = buttons & XINPUT_GAMEPAD_DPAD_RIGHT;
        state.lShoulder = buttons & XINPUT_GAMEPAD_LEFT_SHOULDER;
        state.rShoulder = buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
        state.   lThumb = buttons & XINPUT_GAMEPAD_LEFT_THUMB;
        state.   rThumb = buttons & XINPUT_GAMEPAD_RIGHT_THUMB;
        state.    start = buttons & XINPUT_GAMEPAD_START;
        state.     back = buttons & XINPUT_GAMEPAD_BACK;
    
        // Call callbacks if there was a change of state
        if (s_stickCallback) {
            if (state.lStick != prevState.lStick) s_stickCallback(player, Stick:: left, state.lStick);
            if (state.rStick != prevState.rStick) s_stickCallback(player, Stick::right, state.rStick);
        }
        if (s_triggerCallback) {
            if (state. lTrigger != prevState. lTrigger) s_triggerCallback(player, Trigger:: left, state.lTrigger);
            if (state. rTrigger != prevState. rTrigger) s_triggerCallback(player, Trigger::right, state.rTrigger);
        }
        if (s_dpadCallback) {
            if (state.dpad != prevState.dpad) s_dpadCallback(player, state.dpad);
        }
        if (s_buttonCallback) {
            if (state.        a != prevState.        a) s_buttonCallback(player, Button::        a, state.        a);
            if (state.        b != prevState.        b) s_buttonCallback(player, Button::        b, state.        b);
            if (state.        x != prevState.        x) s_buttonCallback(player, Button::        x, state.        x);
            if (state.        y != prevState.        y) s_buttonCallback(player, Button::        y, state.        y);
            if (state.      dpU != prevState.      dpU) s_buttonCallback(player, Button::      dpU, state.      dpU);
            if (state.      dpD != prevState.      dpD) s_buttonCallback(player, Button::      dpD, state.      dpD);
            if (state.      dpL != prevState.      dpL) s_buttonCallback(player, Button::      dpL, state.      dpL);
            if (state.      dpR != prevState.      dpR) s_buttonCallback(player, Button::      dpR, state.      dpR);
            if (state.lShoulder != prevState.lShoulder) s_buttonCallback(player, Button::lShoulder, state.lShoulder);
            if (state.rShoulder != prevState.rShoulder) s_buttonCallback(player, Button::rShoulder, state.rShoulder);
            if (state.   lThumb != prevState.   lThumb) s_buttonCallback(player, Button::   lThumb, state.   lThumb);
            if (state.   rThumb != prevState.   rThumb) s_buttonCallback(player, Button::   rThumb, state.   rThumb);
            if (state.    start != prevState.    start) s_buttonCallback(player, Button::    start, state.    start);
            if (state.     back != prevState.     back) s_buttonCallback(player, Button::     back, state.     back);
        }
    }



    void poll(int nPlayers) {
        nPlayers = glm::clamp(nPlayers, 0, k_maxPlayerCount);
        for (int player(0); player < nPlayers; ++player) pollPlayer(player);
    }

    bool connected(int player) {
        return s_states[player].connected;
    }

    const State & state(int player) {
        return s_states[player];
    }

    void stickCallback(void (*callback)(int, Stick, vec2)) {
        s_stickCallback = callback;
    }

    void triggerCallback(void (*callback)(int, Trigger, float)) {
        s_triggerCallback = callback;
    }

    void dpadCallback(void (*callback)(int, ivec2)) {
        s_dpadCallback = callback;
    }

    void buttonCallback(void (*callback)(int, Button, bool)) {
        s_buttonCallback = callback;
    }


}