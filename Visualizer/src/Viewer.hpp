#pragma once



#include "Visualizer.hpp"
#include "UI/UI.hpp"



namespace Viewer {

    class ViewerComponent : public ui::Single {

        public:

        ViewerComponent();

        virtual void render() const override;

        virtual void pack() override;

        virtual void cursorPositionEvent(const ivec2 & pos, const ivec2 & delta) override;

        virtual void scrollEvent(const ivec2 & delta) override;

    };

    bool setup();

    shr<ViewerComponent> component();

}