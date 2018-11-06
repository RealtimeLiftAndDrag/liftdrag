#pragma once



#include <vector>

#include "Common/Global.hpp"

namespace ui {

    class Component {

        public:

        virtual void update() {};

        virtual void render() const;

        virtual void pack() = 0;

        virtual const ivec2 & position() const final { return m_position; }
        virtual void position(const ivec2 & position) final;

        virtual const ivec2 & size() const final { return m_size; }
        virtual void size(const ivec2 & size) final;

        virtual const ivec2 & minSize() const = 0;
        virtual const ivec2 & maxSize() const = 0;

        virtual bool contains(const ivec2 & point) const final;

        virtual void focus() final;

        virtual void unfocus() final;

        virtual bool focused() const final;

        virtual const vec4 & backColor() const final { return m_backColor; }
        virtual void backColor(const vec4 & color) final;

        virtual const vec4 & borderColor() const final { return m_backColor; }
        virtual void borderColor(const vec4 & color) final;

        virtual void keyEvent(int key, int action, int mods) {}

        virtual void charEvent(char c) {}

        virtual void cursorPositionEvent(const ivec2 & pos, const ivec2 & delta) {}

        virtual void cursorEnterEvent() {}

        virtual void cursorExitEvent() {}

        virtual void mouseButtonEvent(int button, int action, int mods) {}

        virtual void scrollEvent(const ivec2 & delta) {}

        virtual void focusEvent() {}

        virtual void unfocusEvent() {}

        protected:

        virtual void setViewport() const final;

        private:

        ivec2 m_position;
        ivec2 m_size;
        vec4 m_backColor;
        vec4 m_borderColor;

    };

    class Single : public Component {

        public:

        virtual void pack() override {}

        virtual const ivec2 & minSize() const override final { return m_minSize; }
        virtual const ivec2 & maxSize() const override final { return m_maxSize; }

        protected:

        Single() :
            m_minSize(),
            m_maxSize()
        {}

        Single(const ivec2 & minSize, const ivec2 & maxSize) :
            m_minSize(minSize),
            m_maxSize(maxSize)
        {}

        private:

        ivec2 m_minSize;
        ivec2 m_maxSize;

    };

    class Group : public Component {

        public:

        virtual void update() override;

        virtual void render() const override;

        virtual const ivec2 & minSize() const override final;
        virtual const ivec2 & maxSize() const override final;

        virtual void keyEvent(int key, int action, int mods) override;

        virtual void charEvent(char c) override;

        virtual void cursorPositionEvent(const ivec2 & pos, const ivec2 & delta) override;

        virtual void cursorExitEvent() override;

        virtual void mouseButtonEvent(int button, int action, int mods) override;

        virtual void scrollEvent(const ivec2 & delta) override;

        virtual void add(shr<Component> component);

        protected:

        virtual duo<ivec2> detSizeExtrema() const = 0;

        virtual void packComponents();

        virtual void detCursorOverComp(const ivec2 & cursorPos) const;

        std::vector<shr<Component>> m_components;

        private:

        mutable Component * m_cursorOverComp;
        mutable ivec2 m_minSize, m_maxSize;
        mutable bool m_areSizeExtremaValid;

    };



    bool setup(const ivec2 & windowSize, const std::string & windowTitle, int majorGLVersion, int minorGLVersion, bool vSync);

    void poll();

    const ivec2 & cursorPosition();

    bool isMouseButtonPressed(int button);

    bool isKeyPressed(int key);

    bool shouldExit();

    /*
    // Called when a key is pressed, held, or released
    // Args: key code, action, mods
    void keyCallback(fptr<void(int, int, int)> callback);

    // Called when the mouse is moved in the client area
    // Args: cursor position relative to client area
    void cursorPositionCallback(fptr<void(dvec2)> callback);

    // Called when the cursor enters or exits the client area
    // Args: true if entered, false if exited
    void cursorEnterCallback(fptr<void(bool)> callback);

    // Called when a mouse button is pressed or released
    // Args: button, action, mods
    void mouseButtonCallback(fptr<void(int, int, int)> callback);

    // Called when the scroll wheel is turned
    // Args: x and y offset
    void scrollCallback(fptr<void(dvec2)> callback);

    // Called when a request has been made for the program to exit
    void exitCallback(fptr<void()> callback);
    */

    // Requires OpenGL state:
    // - `GL_DEPTH_TEST` disabled
    // - `GL_BLEND` enabled
    void update();

    void render();

    void setRootComponent(shr<Component> root);

    void setTooltip(shr<Component> tooltip);

    void focus(Component * component);

}