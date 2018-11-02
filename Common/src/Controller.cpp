#include "Controller.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <XInput.h>

#pragma comment(lib, "XInput.lib")



static constexpr int k_maxNumPlayers(8);

static XINPUT_STATE s_states[k_maxNumPlayers];

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



Controller::Controller(int player) :
    m_player(player),
    m_connected(false),
    m_lStick(),
    m_rStick(),
    m_lTrigger(),
    m_rTrigger()
{}

void Controller::poll() {
    XINPUT_STATE & state(s_states[m_player]);
    ZeroMemory(&state, sizeof(XINPUT_STATE));
    m_connected = !XInputGetState(m_player, &state);
    if (!m_connected) {
        return;
    }

    m_lStick.x = stickVal(state.Gamepad.sThumbLX);
    m_lStick.y = stickVal(state.Gamepad.sThumbLY);
    m_rStick.x = stickVal(state.Gamepad.sThumbRX);
    m_rStick.y = stickVal(state.Gamepad.sThumbRY);
    m_lTrigger = triggerVal(state.Gamepad.bLeftTrigger);
    m_rTrigger = triggerVal(state.Gamepad.bRightTrigger);
}