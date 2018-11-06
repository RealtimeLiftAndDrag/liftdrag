#pragma once

#include "UI.hpp"



namespace ui {

    class HorizontalGroup : public Group {

        public:

        virtual void pack() override;

        protected:

        virtual duo<ivec2> detSizeExtrema() const override;

    };

    class VerticalGroup : public Group {

        public:

        virtual void pack() override;

        protected:

        virtual duo<ivec2> detSizeExtrema() const override;

    };

}