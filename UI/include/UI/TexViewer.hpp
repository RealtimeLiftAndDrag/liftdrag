#pragma once



#include "UI.hpp"



namespace ui {

    class TexViewer : public ui::Single {

        public:

        static bool setup();

        private:

        uint m_tex;
        ivec2 m_texSize;
        vec2 m_initViewSize; // in texture space
        vec2 m_viewSize; // in texture space
        vec2 m_center; // in texture space
        int m_zoom;

        public:

        TexViewer(uint tex, const ivec2 & texSize, const ivec2 & minSize);

        virtual void render() const override;

        virtual void pack() override;

        virtual void cursorPositionEvent(const ivec2 & pos, const ivec2 & delta) override;

        virtual void scrollEvent(const ivec2 & delta) override;

        const vec2 & center() const { return m_center; }
        void center(const vec2 & center);

        void moveCenter(const vec2 & delta);

        int zoom() const { return m_zoom; }
        void zoom(int zoom);

        void zoomIn();

        void zoomOut();

        void zoomReset();

    };

}