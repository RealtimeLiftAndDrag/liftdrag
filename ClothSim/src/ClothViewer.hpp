#pragma once



#include "UI/UI.hpp"
#include "Common/Camera.hpp"
#include "Common/Model.hpp"

#include "ClothSim.hpp"



class ClothViewerComponent : public ui::Single {

    public:

    ClothViewerComponent(const SoftModel & model, float scale, const ivec2 & minSize);

    virtual void render() const override;

    virtual void pack() override;

    virtual void cursorPositionEvent(const ivec2 & pos, const ivec2 & delta) override;

    virtual void cursorExitEvent() override;

    virtual void mouseButtonEvent(int button, int action, int mods) override;

    virtual void scrollEvent(const ivec2 & delta) override;

    const ThirdPersonCamera & camera() const { return m_camera; }

    bool isTouch() const { return m_isTouch; }

    const vec2 & touchPoint() const { return m_touchP; }

    private:

    void detTouchPoint();
    
    const SoftModel & m_model;
    ThirdPersonCamera m_camera;
    bool m_isTouch;
    vec2 m_touchP;

};
