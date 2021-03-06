#pragma once



#include <vector>

#include "Text.hpp"



namespace ui {

    class Graph : public ui::VerticalGroup {

        private:

        struct Curve {

            std::vector<vec2> points;
            std::string label;
            int maxNPoints;
            vec3 color;

            mutable unsigned int vao, vbo;
            mutable bool isSetup, isChange;

            Curve(const std::string & label, const vec3 & color, int maxNPoints);

            bool setup() const;

            void upload() const;

            void cleanup();

            float valAt(float x) const;

        };

        class PlotComp : public ui::Single {

            friend Graph;

            private:

            Graph & m_graph;
            shr<ui::Text> m_cursorText;
            bool m_isFocusUpdateNeeded;

            public:

            PlotComp(Graph & graph, const ivec2 & minSize, const ivec2 & maxSize);

            virtual void update() override;

            virtual void render() const override;

            virtual void cursorPositionEvent(const ivec2 & pos, const ivec2 & delta) override;

            virtual void cursorEnterEvent() override;

            virtual void cursorExitEvent() override;

            virtual void scrollEvent(const ivec2 & delta) override;

            private:

            void updateFocus();

        };

        class InnerComp : public ui::Group {

            friend Graph;

            private:

            Graph & m_graph;
            shr<PlotComp> m_plotComp;
            shr<ui::Number> m_domainMinText, m_domainMaxText;
            shr<ui::Number> m_rangeMinText, m_rangeMaxText;
            ivec2 m_minMargin, m_maxMargin;
            bool m_isGridTextUpdateNeeded;

            public:

            InnerComp(Graph & graph, const ivec2 & minPlotSize, const ivec2 & maxPlotSize);

            virtual void update();

            virtual void pack() override;

            virtual duo<ivec2> detSizeExtrema() const;

            private:

            void updateGridText();

        };

        public:

        static bool setup();

        private:

        std::vector<Curve> m_curves;
        std::string m_xLabel;
        vec2 m_viewMin, m_viewMax;
        vec2 m_gridSize;
        bool m_isFocusX;
        float m_focusX;
        bool m_isMarkX;
        float m_markX;
        shr<InnerComp> m_innerComp;

        public:

        Graph(const std::string & title, const std::string & xLabel, const vec2 & viewMin, const vec2 & viewMax, const ivec2 & minPlotSize, const ivec2 & maxPlotSize);

        void addCurve(const std::string & label, const vec3 & color, int maxNPoints);

        const std::vector<vec2> & accessPoints(int curveI) const;

        std::vector<vec2> & mutatePoints(int curveI);

        const vec2 & viewMin() const { return m_viewMin; }
        const vec2 & viewMax() const { return m_viewMax; }

        void setView(const vec2 & min, const vec2 & max);

        void zoomView(const vec2 & factor);

        void moveView(const vec2 & delta);

        void focusX(float x);
        float focusX() const { return m_focusX; }

        void unfocusX();

        void markX(float x);
        float markX() const { return m_markX; }

        void unmarkX();

        const vec2 & gridSize() const { return m_gridSize; }

        void cleanup();

    };

}