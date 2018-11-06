#pragma once



#include "Global.hpp"



class Controller {

    public:

    // `player` starts at 0
    Controller(int player);

    // Must be called once before calling any of the accessors
    void poll();
    
    bool connected() const { return m_connected; }
    
    const vec2 & lStick() const { return m_lStick; }
    const vec2 & rStick() const { return m_rStick; }

    float lTrigger() const { return m_lTrigger; }
    float rTrigger() const { return m_rTrigger; }

    bool aButton() const { return m_aButton; }

    private:

    int m_player;
    bool m_connected;
    vec2 m_lStick;
    vec2 m_rStick;
    float m_lTrigger;
    float m_rTrigger;
    bool m_aButton;

};