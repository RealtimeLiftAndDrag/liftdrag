#include "Results.hpp"

#include <iostream>
#include <memory>
#include <map>
#include <sstream>

#include "Common/glad/glad.h"
#include "Common/GLFW/glfw3.h"
#include "Common/GLSL.h"
#include "Common/Program.h"
#include "Common/Graph.hpp"
#include "Common/Text.hpp"



namespace Results {

    static constexpr float k_angleGranularity(1.0f); // results kept in increments of this many degrees
    static constexpr float k_invAngleGranularity(1.0f / k_angleGranularity);
    static constexpr float k_initAngleGraphDomainMin(-90.0f), k_initAngleGraphDomainMax(90.0f);
    static constexpr float k_initAngleGraphRangeMin(-50.0f), k_initAngleGraphRangeMax(50.0f);
    static constexpr float k_angleGraphExcessFactor(1.025f);
    static const ivec2 k_minAngleGraphSize(300, 100);
    static const ivec2 k_maxAngleGraphSize(0, 300);
    static const std::string k_angleGraphTitleString("Lift and Drag Force (N) by Angle of Attack (\u00B0)");
    
    static constexpr int k_sliceCount(100); // should be changed in accordance with the simulation setting
    static constexpr float k_initSliceGraphRangeMin(-1.5f), k_initSliceGraphRangeMax(1.5f);
    static constexpr float k_sliceGraphExcessFactor(k_angleGraphExcessFactor);
    static const ivec2 k_minSliceGraphSize(k_minAngleGraphSize);
    static const ivec2 k_maxSliceGraphSize(k_maxAngleGraphSize);
    static const std::string k_sliceGraphTitleString("Lift and Drag Force (N) by Slice");



    static std::map<float, std::pair<float, float>> s_angleRecord; // map of results where key is angle and value is lift/drag pair
    static bool s_isAngleRecordChange;

    static std::map<int, std::pair<float, float>> s_sliceRecord;
    static bool s_isSliceRecordChange;

    static shr<Graph> s_angleGraph, s_sliceGraph;



    bool setup(const std::string & resourcesDir) {
        // Angle graph

        vec2 min(k_initAngleGraphDomainMin, k_initAngleGraphRangeMin);
        vec2 max(k_initAngleGraphDomainMax, k_initAngleGraphRangeMax);
        vec2 size(max - min);
        vec2 center((min + max) * 0.5f);
        size *= k_angleGraphExcessFactor;
        min = center - size * 0.5f;
        max = center + size * 0.5f;

        s_angleGraph.reset(new Graph(
            k_angleGraphTitleString,
            "Angle",
            min,
            max,
            bvec2(false, true),
            k_minAngleGraphSize,
            k_maxAngleGraphSize
        ));
        s_angleGraph->addCurve("Lift", vec3(0.0f, 1.0f, 0.0f), int(180.0f * k_invAngleGranularity) + 1);
        s_angleGraph->addCurve("Drag", vec3(1.0f, 0.0f, 0.0f), int(180.0f * k_invAngleGranularity) + 1);

        // Slice graph

        min = vec2(0.0f, k_initSliceGraphRangeMin);
        max = vec2(k_sliceCount, k_initSliceGraphRangeMax);
        size = vec2(max - min);
        center = vec2((min + max) * 0.5f);
        size *= k_sliceGraphExcessFactor;
        min = center - size * 0.5f;
        max = center + size * 0.5f;

        s_sliceGraph.reset(new Graph(
            k_sliceGraphTitleString,
            "Slice",
            min,
            max,
            bvec2(false, true),
            k_minSliceGraphSize,
            k_maxSliceGraphSize
        ));
        s_sliceGraph->addCurve("Lift", vec3(0.0f, 1.0f, 0.0f), k_sliceCount);
        s_sliceGraph->addCurve("Drag", vec3(1.0f, 0.0f, 0.0f), k_sliceCount);

        return true;
    }

    void update() {
        if (s_isAngleRecordChange) {
            auto & liftPoints(s_angleGraph->mutatePoints(0));
            auto & dragPoints(s_angleGraph->mutatePoints(1));
            liftPoints.clear();
            dragPoints.clear();
            for (const auto & r : s_angleRecord) {
                liftPoints.emplace_back(r.first, r.second.first);
                dragPoints.emplace_back(r.first, r.second.second);
            }

            s_isAngleRecordChange = false;
        }
        if (s_isSliceRecordChange) {
            auto & liftPoints(s_sliceGraph->mutatePoints(0));
            auto & dragPoints(s_sliceGraph->mutatePoints(1));
            liftPoints.clear();
            dragPoints.clear();
            for (const auto & r : s_sliceRecord) {
                liftPoints.emplace_back(r.first, r.second.first);
                dragPoints.emplace_back(r.first, r.second.second);
            }

            s_isSliceRecordChange = false;
        }
    }

    void submitAngle(float angle, float lift, float drag) {
        angle = std::round(angle * k_invAngleGranularity) * k_angleGranularity;
        s_angleRecord[angle] = { lift, drag };
        s_isAngleRecordChange = true;
    }

    void clearAngles() {
        s_angleRecord.clear();
        s_isAngleRecordChange = true;
    }

    void submitSlice(int slice, float lift, float drag) {
        s_sliceRecord[slice] = { lift, drag };
        s_isSliceRecordChange = true;
    }

    void clearSlices() {
        s_sliceRecord.clear();
        s_isSliceRecordChange = true;
    }

    bool valAt(float angle, float & r_lift, float & r_drag) {
        auto it(s_angleRecord.find(angle));
        if (it != s_angleRecord.end()) {
            r_lift = it->second.first;
            r_drag = it->second.second;
            return true;
        }

        auto preIt(s_angleRecord.lower_bound(angle)), postIt(s_angleRecord.upper_bound(angle));
        if (preIt == s_angleRecord.begin() || postIt == s_angleRecord.end()) {
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

    shr<Graph> angleGraph() {
        return s_angleGraph;
    }

    shr<Graph> sliceGraph() {
        return s_sliceGraph;
    }

    void resetGraphs() {
        s_angleGraph->setView(
            vec2(k_initAngleGraphDomainMin, k_initAngleGraphRangeMin),
            vec2(k_initAngleGraphDomainMax, k_initAngleGraphRangeMax)
        );
        s_sliceGraph->setView(
            vec2(0, k_initSliceGraphRangeMin),
            vec2(k_sliceCount, k_initSliceGraphRangeMax)
        );
    }

}
