#include "Controller.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <XInput.h>

#pragma comment(lib, "XInput.lib")



static XINPUT_STATE s_xboxState;

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



Controller::Controller(int player) :
    m_player(player),
    m_connected(false),
    m_state{}
{}

void Controller::poll() {
    m_connected = !XInputGetState(m_player, &s_xboxState);
    if (!m_connected) {
        return;
    }

    // Save previous state
    State prevState(m_state);

    // Update state
    m_state. lStick.x = stickVal(s_xboxState.Gamepad.sThumbLX);
    m_state. lStick.y = stickVal(s_xboxState.Gamepad.sThumbLY);
    m_state. rStick.x = stickVal(s_xboxState.Gamepad.sThumbRX);
    m_state. rStick.y = stickVal(s_xboxState.Gamepad.sThumbRY);
    m_state. lTrigger = triggerVal(s_xboxState.Gamepad.bLeftTrigger);
    m_state. rTrigger = triggerVal(s_xboxState.Gamepad.bRightTrigger);
    m_state.     dpad = dpadVal(s_xboxState.Gamepad.wButtons);
    m_state.        a = s_xboxState.Gamepad.wButtons & XINPUT_GAMEPAD_A;
    m_state.        b = s_xboxState.Gamepad.wButtons & XINPUT_GAMEPAD_B;
    m_state.        x = s_xboxState.Gamepad.wButtons & XINPUT_GAMEPAD_X;
    m_state.        y = s_xboxState.Gamepad.wButtons & XINPUT_GAMEPAD_Y;
    m_state.      dpU = s_xboxState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP;
    m_state.      dpD = s_xboxState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
    m_state.      dpL = s_xboxState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
    m_state.      dpR = s_xboxState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
    m_state.lShoulder = s_xboxState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
    m_state.rShoulder = s_xboxState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
    m_state.   lThumb = s_xboxState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
    m_state.   rThumb = s_xboxState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
    m_state.    start = s_xboxState.Gamepad.wButtons & XINPUT_GAMEPAD_START;
    m_state.     back = s_xboxState.Gamepad.wButtons & XINPUT_GAMEPAD_BACK;
    
    // Call callbacks if there was a change of state
    if (   m_lStickCallback && m_state.   lStick != prevState.   lStick)    m_lStickCallback(m_player, m_state.   lStick);
    if (   m_rStickCallback && m_state.   rStick != prevState.   rStick)    m_rStickCallback(m_player, m_state.   rStick);
    if ( m_lTriggerCallback && m_state. lTrigger != prevState. lTrigger)  m_lTriggerCallback(m_player, m_state. lTrigger);
    if ( m_rTriggerCallback && m_state. rTrigger != prevState. rTrigger)  m_rTriggerCallback(m_player, m_state. rTrigger);
    if (     m_dpadCallback && m_state.     dpad != prevState.     dpad)      m_dpadCallback(m_player, m_state.     dpad);
    if (        m_aCallback && m_state.        a != prevState.        a)         m_aCallback(m_player, m_state.        a);
    if (        m_bCallback && m_state.        b != prevState.        b)         m_bCallback(m_player, m_state.        b);
    if (        m_xCallback && m_state.        x != prevState.        x)         m_xCallback(m_player, m_state.        x);
    if (        m_yCallback && m_state.        y != prevState.        y)         m_yCallback(m_player, m_state.        y);
    if (      m_dpUCallback && m_state.      dpU != prevState.      dpU)       m_dpUCallback(m_player, m_state.      dpU);
    if (      m_dpDCallback && m_state.      dpD != prevState.      dpD)       m_dpDCallback(m_player, m_state.      dpD);
    if (      m_dpLCallback && m_state.      dpL != prevState.      dpL)       m_dpLCallback(m_player, m_state.      dpL);
    if (      m_dpRCallback && m_state.      dpR != prevState.      dpR)       m_dpRCallback(m_player, m_state.      dpR);
    if (m_lShoulderCallback && m_state.lShoulder != prevState.lShoulder) m_lShoulderCallback(m_player, m_state.lShoulder);
    if (m_rShoulderCallback && m_state.rShoulder != prevState.rShoulder) m_rShoulderCallback(m_player, m_state.rShoulder);
    if (   m_lThumbCallback && m_state.   lThumb != prevState.   lThumb)    m_lThumbCallback(m_player, m_state.   lThumb);
    if (   m_rThumbCallback && m_state.   rThumb != prevState.   rThumb)    m_rThumbCallback(m_player, m_state.   rThumb);
    if (    m_startCallback && m_state.    start != prevState.    start)     m_startCallback(m_player, m_state.    start);
    if (     m_backCallback && m_state.     back != prevState.     back)      m_backCallback(m_player, m_state.     back);

}
