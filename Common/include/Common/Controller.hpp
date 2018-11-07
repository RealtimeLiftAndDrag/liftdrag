#pragma once



#include "Global.hpp"



class Controller {

    public:

    struct State {
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

    // `player` starts at 0
    Controller(int player);

    // Must be called once before calling any of the accessors
    void poll();
    
    int player() const { return m_player; }

    // Whether or not the controller is connected. Must have called poll prior
    bool connected() const { return m_connected; }

    const State & state() const { return m_state; }

    // Set callbacks
    void    lStickCallback(void (*callback)(int,  vec2)) {    m_lStickCallback = callback; }
    void    rStickCallback(void (*callback)(int,  vec2)) {    m_rStickCallback = callback; }
    void  lTriggerCallback(void (*callback)(int, float)) {  m_lTriggerCallback = callback; }
    void  rTriggerCallback(void (*callback)(int, float)) {  m_rTriggerCallback = callback; }
    void      dpadCallback(void (*callback)(int, ivec2)) {      m_dpadCallback = callback; }
    void         aCallback(void (*callback)(int,  bool)) {         m_aCallback = callback; }
    void         bCallback(void (*callback)(int,  bool)) {         m_bCallback = callback; }
    void         xCallback(void (*callback)(int,  bool)) {         m_xCallback = callback; }
    void         yCallback(void (*callback)(int,  bool)) {         m_yCallback = callback; }
    void       dpUCallback(void (*callback)(int,  bool)) {       m_dpUCallback = callback; }
    void       dpDCallback(void (*callback)(int,  bool)) {       m_dpDCallback = callback; }
    void       dpLCallback(void (*callback)(int,  bool)) {       m_dpLCallback = callback; }
    void       dpRCallback(void (*callback)(int,  bool)) {       m_dpRCallback = callback; }
    void lShoulderCallback(void (*callback)(int,  bool)) { m_lShoulderCallback = callback; }
    void rShoulderCallback(void (*callback)(int,  bool)) { m_rShoulderCallback = callback; }
    void    lThumbCallback(void (*callback)(int,  bool)) {    m_lThumbCallback = callback; }
    void    rThumbCallback(void (*callback)(int,  bool)) {    m_rThumbCallback = callback; }
    void     startCallback(void (*callback)(int,  bool)) {     m_startCallback = callback; }
    void      backCallback(void (*callback)(int,  bool)) {      m_backCallback = callback; }

    private:

    int m_player;
    bool m_connected;
    State m_state;

    void (*   m_lStickCallback)(int,  vec2) = nullptr;
    void (*   m_rStickCallback)(int,  vec2) = nullptr;
    void (* m_lTriggerCallback)(int, float) = nullptr;
    void (* m_rTriggerCallback)(int, float) = nullptr;
    void (*     m_dpadCallback)(int, ivec2) = nullptr;
    void (*        m_aCallback)(int,  bool) = nullptr;
    void (*        m_bCallback)(int,  bool) = nullptr;
    void (*        m_xCallback)(int,  bool) = nullptr;
    void (*        m_yCallback)(int,  bool) = nullptr;
    void (*      m_dpUCallback)(int,  bool) = nullptr;
    void (*      m_dpDCallback)(int,  bool) = nullptr;
    void (*      m_dpLCallback)(int,  bool) = nullptr;
    void (*      m_dpRCallback)(int,  bool) = nullptr;
    void (*m_lShoulderCallback)(int,  bool) = nullptr;
    void (*m_rShoulderCallback)(int,  bool) = nullptr;
    void (*   m_lThumbCallback)(int,  bool) = nullptr;
    void (*   m_rThumbCallback)(int,  bool) = nullptr;
    void (*    m_startCallback)(int,  bool) = nullptr;
    void (*     m_backCallback)(int,  bool) = nullptr;

};