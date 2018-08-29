#include "Results.hpp"

#include <iostream>
#include <memory>
#include <map>
#include <sstream>

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "GLSL.h"
#include "Program.h"
#include "Graph.hpp"
#include "Text.hpp"



namespace Results {

    static constexpr float k_angleGranularity(1.0f); // results kept in increments of this many degrees
    static constexpr float k_invAngleGranularity(1.0f / k_angleGranularity);
    static constexpr float k_initAngleGraphDomainMin(-90.0f), k_initAngleGraphDomainMax(90.0f);
    static constexpr float k_initAngleGraphRangeMin(-1000.0f), k_initAngleGraphRangeMax(1000.0f);
    static const ivec2 k_minAngleGraphSize(200, 100);
    static const std::string k_angleGraphTitleString("Lift and Drag Force (N) by Angle of Attack (\u00B0)");
    
    static constexpr int k_sliceCount(100); // should be changed in accordance with the simulation setting
    static constexpr float k_initSliceGraphRangeMin(-1000.0f), k_initSliceGraphRangeMax(1000.0f);
    static const ivec2 k_minSliceGraphSize(200, 100);
    static const std::string k_sliceGraphTitleString("Lift and Drag Force (N) by Slice");


    static constexpr float k_zoomFactor(1.1f);
    static constexpr int k_valTextPrecision(3);
    static const std::string k_graphLegendString("Green: Lift\n  Red: Drag");



    static std::map<float, std::pair<float, float>> s_record; // map of results where key is angle and value is lift/drag pair
    static bool s_isRecordChange;

    static shr<UI::HorizGroup> s_parentGroup;
    static shr<Graph> s_angleGraph, s_sliceGraph;

    //static GLFWwindow * s_window;
    static ivec2 s_windowSize;
    static bool s_mouseButtonDown;
    static ivec2 s_mousePos;

    static ivec2 s_graphPos, s_graphSize;

    static unq<Text> s_domainMinText, s_domainMaxText;
    static unq<Text> s_rangeMinText, s_rangeMaxText;
    static bool s_isGridTextUpdateNeeded;

    static unq<Text> s_cursorText;
    static bool s_isCursorTextUpdateNeeded;

    static unq<Text> s_titleText;
    static unq<Text> s_legendText;
    static bool s_isGraphTextUpdateNeeded;

    static bool s_isRedrawNeeded(true);



    /*static void detGraphBounds() {
        s_graphPos.x = k_leftMargin;
        s_graphPos.y = k_bottomMargin;
        s_graphSize.x = s_windowSize.x - k_leftMargin - k_rightMargin;
        s_graphSize.y = s_windowSize.y - k_bottomMargin - k_topMargin;
    }

    static std::string valToString(float v) {
        std::stringstream ss;
        ss.precision(k_valTextPrecision);
        ss << v;
        return ss.str();
    }

    static std::string pointToString(float angle, float lift, float drag) {
        std::stringstream ss;
        ss.precision(k_valTextPrecision);
        ss << "Angle:" << angle << "�\nLift:" << lift << "N\nDrag:" << drag << "N";
        return ss.str();
    }

    static bool isCursorInGraph() {    
        ivec2 relPos(s_mousePos - s_graphPos);
        return relPos.x >= 0 && relPos.y >= 0 && relPos.x < s_graphSize.x && relPos.y < s_graphSize.y;
    }

    static void updateGridText() {
        vec2 gridMin(glm::ceil(s_graph->viewMin() / s_graph->gridSize()) * s_graph->gridSize());
        vec2 gridMax(glm::floor(s_graph->viewMax() / s_graph->gridSize()) * s_graph->gridSize());
        vec2 invViewSize(1.0f / (s_graph->viewMax() - s_graph->viewMin()));
        vec2 graphSize(s_graphSize);
        ivec2 gridMinPos(glm::round((gridMin - s_graph->viewMin()) * invViewSize * graphSize));
        ivec2 gridMaxPos(glm::round((gridMax - s_graph->viewMin()) * invViewSize * graphSize));
    
        s_domainMinText->position(ivec2(s_graphPos.x + gridMinPos.x, s_graphPos.y));
        s_domainMinText->string(valToString(gridMin.x));
        s_domainMaxText->position(ivec2(s_graphPos.x + gridMaxPos.x, s_graphPos.y));
        s_domainMaxText->string(valToString(gridMax.x));

        s_rangeMinText->position(ivec2(s_graphPos.x, s_graphPos.y + gridMinPos.y));
        s_rangeMinText->string(valToString(gridMin.y));
        s_rangeMaxText->position(ivec2(s_graphPos.x, s_graphPos.y + gridMaxPos.y));
        s_rangeMaxText->string(valToString(gridMax.y));
    }

    static void updateCursorText() {
        s_cursorText->string("");
        s_graph->removeFocusX();

        if (!isCursorInGraph()) {
            return;
        }

        float angle(float(s_mousePos.x - s_graphPos.x));
        angle /= s_graphSize.x;
        angle *= s_graph->viewMax().x - s_graph->viewMin().x;
        angle += s_graph->viewMin().x;

        float lift, drag;
        if (!valAt(angle, lift, drag)) {
            return;
        }

        s_cursorText->position(s_mousePos + ivec2(10, -10));
        s_cursorText->string(pointToString(angle, lift, drag));
        s_graph->focusX(angle);
    }

    static void updateGraphText() {
        s_titleText->position(ivec2(s_windowSize.x / 2, s_graphPos.y + s_graphSize.y + 10));
        s_legendText->position(s_graphPos + s_graphSize + ivec2(10, -10));
    }

    static void framebufferSizeCallback(GLFWwindow * window, int width, int height) {
        s_windowSize.x = width; s_windowSize.y = height;
        detGraphBounds();
        s_isGridTextUpdateNeeded = true;
        s_isCursorTextUpdateNeeded = true;
        s_isGraphTextUpdateNeeded = true;
        s_isRedrawNeeded = true;
    }

    static void scrollCallback(GLFWwindow * window, double x, double y) {
        if (y != 0.0f) {
            if (y < 0.0f) s_graph->zoomView(k_zoomFactor);
            else if (y > 0.0f) s_graph->zoomView(1.0f / k_zoomFactor);
            s_isGridTextUpdateNeeded = true;
            s_isCursorTextUpdateNeeded = true;
            s_isRedrawNeeded = true;
        }
    }

    static void cursorPosCallback(GLFWwindow * window, double x, double y) {
        ivec2 newPos{int(x), s_windowSize.y - 1 - int(y)};
        ivec2 delta(newPos - s_mousePos);

        if (s_mouseButtonDown) {
            vec2 factor((s_graph->viewMax() - s_graph->viewMin()) / vec2(s_graphSize));
            s_graph->moveView(-factor * vec2(delta));
            s_isGridTextUpdateNeeded = true;
            s_isRedrawNeeded = true;
        }

        s_mousePos += delta;
        s_isCursorTextUpdateNeeded = true;
    }

    static void mouseButtonCallback(GLFWwindow * window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS && mods == 0) {
            s_mouseButtonDown = true;
        }
        else {
            s_mouseButtonDown = false;
        }
    }*/



    bool setup(const std::string & resourcesDir) {

        s_angleGraph.reset(new Graph(
            vec2(k_initAngleGraphDomainMin, k_initAngleGraphRangeMin),
            vec2(k_initAngleGraphDomainMax, k_initAngleGraphRangeMax),
            k_minAngleGraphSize
        ));
        s_angleGraph->addCurve(vec3(0.0f, 1.0f, 0.0f), int(180.0f * k_invAngleGranularity) + 1);
        s_angleGraph->addCurve(vec3(1.0f, 0.0f, 0.0f), int(180.0f * k_invAngleGranularity) + 1);

        shr<Text> angleTitleText(new Text(k_angleGraphTitleString, ivec2(), vec3(1.0f), int(k_angleGraphTitleString.length()), 0, 1));

        shr<UI::VertGroup> angleGroup(new UI::VertGroup());
        angleGroup->add(s_angleGraph);
        angleGroup->add(angleTitleText);

        s_sliceGraph.reset(new Graph(
            vec2(0, k_initSliceGraphRangeMin),
            vec2(k_sliceCount, k_initSliceGraphRangeMax),
            k_minSliceGraphSize
        ));
        s_sliceGraph->addCurve(vec3(0.0f, 1.0f, 0.0f), k_sliceCount);
        s_sliceGraph->addCurve(vec3(1.0f, 0.0f, 0.0f), k_sliceCount);

        shr<Text> sliceTitleText(new Text(k_sliceGraphTitleString, ivec2(), vec3(1.0f), int(k_sliceGraphTitleString.length()), 0, 1));

        shr<UI::VertGroup> sliceGroup(new UI::VertGroup());
        sliceGroup->add(s_sliceGraph);
        sliceGroup->add(sliceTitleText);

        s_parentGroup.reset(new UI::HorizGroup());
        s_parentGroup->add(angleGroup);
        s_parentGroup->add(sliceGroup);


        //s_domainMinText.reset(new Text("", ivec2(), ivec2(0, 1), vec3(1.0f)));
        //s_domainMaxText.reset(new Text("", ivec2(), ivec2(0, 1), vec3(1.0f)));
        //s_rangeMinText.reset(new Text("", ivec2(), ivec2(-1, 0), vec3(1.0f)));
        //s_rangeMaxText.reset(new Text("", ivec2(), ivec2(-1, 0), vec3(1.0f)));
        //s_isGridTextUpdateNeeded = true;

        //s_cursorText.reset(new Text("", ivec2(), ivec2(1, 1), vec3(1.0f)));

        //s_titleText.reset(new Text(k_graphTitleString, ivec2(), ivec2(0, -1), vec3(1.0f)));
        //s_legendText.reset(new Text(k_graphLegendString, ivec2(), ivec2(1, 1), vec3(0.5f)));
        s_isGraphTextUpdateNeeded = true;

        return true;
    }

    void update() {
        if (s_isRecordChange) {
            auto & liftPoints(s_angleGraph->mutatePoints(0));
            auto & dragPoints(s_angleGraph->mutatePoints(1));
            liftPoints.clear();
            dragPoints.clear();
            for (const auto & r : s_record) {
                liftPoints.emplace_back(r.first, r.second.first);
                dragPoints.emplace_back(r.first, r.second.second);
            }

            s_isRecordChange = false;
            s_isCursorTextUpdateNeeded = true;
        }
    }

    void submit(float angle, float lift, float drag) {
        angle = std::round(angle * k_invAngleGranularity) * k_angleGranularity;
        s_record[angle] = { lift, drag };
        s_isRecordChange = true;
    }

    bool valAt(float angle, float & r_lift, float & r_drag) {
        auto it(s_record.find(angle));
        if (it != s_record.end()) {
            r_lift = it->second.first;
            r_drag = it->second.second;
            return true;
        }

        auto preIt(s_record.lower_bound(angle)), postIt(s_record.upper_bound(angle));
        if (preIt == s_record.begin() || postIt == s_record.end()) {
            return false;
        }
        --preIt;

        float preAngle(preIt->first), preLift(preIt->second.first), preDrag(preIt->second.second);
        float postAngle(postIt->first), postLift(postIt->second.first), postDrag(postIt->second.second);

        float interpP((angle - preAngle) / (postAngle - preAngle));
        r_lift = glm::mix(preLift, postLift, interpP);
        r_drag = glm::mix(preDrag, postDrag, interpP);
        return true;
    }

    shr<UI::Component> getUI() {
        return s_parentGroup;
    }

}
