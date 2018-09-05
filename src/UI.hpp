#pragma once



#include <vector>

#include "Global.hpp"



namespace UI {

    class Component {

        protected:

        ivec2 m_position;
        ivec2 m_size;
    
        public:

        virtual void update() {};

        virtual void render() const = 0;

        virtual void pack() = 0;

        virtual const ivec2 & position() const final { return m_position; }
        virtual void position(const ivec2 & position) final;

        virtual const ivec2 & size() const final { return m_size; }
        virtual void size(const ivec2 & size) final;

        virtual const ivec2 & minSize() const = 0;
        virtual const ivec2 & maxSize() const = 0;

        virtual bool contains(const ivec2 & point) const final;

        virtual void keyEvent(int key, int action, int mods) {}

        virtual void cursorPositionEvent(const ivec2 & pos, const ivec2 & delta) {}

        virtual void cursorEnterEvent() {}

        virtual void cursorExitEvent() {}

        virtual void mouseButtonEvent(int button, int action, int mods) {}

        virtual void scrollEvent(const ivec2 & delta) {}

    };

    class Single : public Component {

        protected:

        ivec2 m_minSize;
        ivec2 m_maxSize;

        Single() :
            m_minSize(),
            m_maxSize()
        {}

        Single(const ivec2 & minSize, const ivec2 & maxSize) :
            m_minSize(minSize),
            m_maxSize(maxSize)
        {}

        public:

        virtual void pack() override {}

        virtual const ivec2 & minSize() const override final { return m_minSize; }
        virtual const ivec2 & maxSize() const override final { return m_maxSize; }

    };

    class Group : public Component {

        protected:
        
        std::vector<shr<Component>> m_components;

        mutable Component * m_cursorOverComp;

        mutable ivec2 m_minSize;
        mutable ivec2 m_maxSize;
        mutable bool m_areSizeExtremaValid;

        public:

        virtual void update() override;

        virtual void render() const override;

        virtual const ivec2 & minSize() const override;
        virtual const ivec2 & maxSize() const override;

        virtual void keyEvent(int key, int action, int mods) override;

        virtual void cursorPositionEvent(const ivec2 & pos, const ivec2 & delta) override;

        virtual void cursorExitEvent() override;

        virtual void mouseButtonEvent(int button, int action, int mods) override;

        virtual void scrollEvent(const ivec2 & delta) override;

        virtual void add(shr<Component> component);

        protected:

        virtual void detSizeExtrema() const = 0;

        virtual void packComponents();

        virtual void detCursorOverComp(const ivec2 & cursorPos) const;

    };

    class HorizontalGroup : public Group {

        public:

        virtual void pack() override;

        private:

        virtual void detSizeExtrema() const override;

    };

    class VerticalGroup : public Group {

        public:

        virtual void pack() override;

        private:

        virtual void detSizeExtrema() const override;

    };

    bool setup(const ivec2 & windowSize, const std::string & windowTitle, int majorGLVersion, int minorGLVersion, bool vSync, const std::string & resourceDir);

    void setRootComponent(shr<Component> root);

    void setTooltip(shr<Component> tooltip);

    void poll();

    void update();

    void render();

    bool shouldClose();

    const ivec2 & cursorPosition();

    bool isMouseButtonPressed(int button);

    bool isKeyPressed(int key);

}