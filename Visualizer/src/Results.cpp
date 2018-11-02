#include "Results.hpp"

#include <iostream>
#include <memory>
#include <map>
#include <sstream>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "Common/GLSL.h"
#include "Common/Shader.hpp"
#include "UI/Graph.hpp"



namespace results {

    static constexpr float k_angleGranularity(1.0f); // results kept in increments of this many degrees
    static constexpr float k_invAngleGranularity(1.0f / k_angleGranularity);
    static constexpr float k_initAngleGraphDomainMin(-90.0f), k_initAngleGraphDomainMax(90.0f);
    static constexpr float k_angleGraphExcessFactor(1.025f);
    static const ivec2 k_minAngleGraphSize(300, 100);
    static const ivec2 k_maxAngleGraphSize(0, 300);
    static const std::string & k_angleGraphTitleString("Lift and Drag Force (N) by Angle of Attack (\u00B0)");

    static constexpr float k_sliceGraphExcessFactor(k_angleGraphExcessFactor);
    static const ivec2 k_minSliceGraphSize(k_minAngleGraphSize);
    static const ivec2 k_maxSliceGraphSize(k_maxAngleGraphSize);
    static const std::string & k_sliceGraphTitleString("Lift and Drag Force (N) by Slice");


    static int s_sliceCount;
    static vec2 s_angleGraphRange, s_sliceGraphRange;

    static std::map<float, Entry> s_angleRecord; // map of results where key is angle and value is lift/drag pair
    static bool s_isAngleRecordChange;

    static std::map<int, Entry> s_sliceRecord;
    static bool s_isSliceRecordChange;

    static shr<ui::Graph> s_angleGraph, s_sliceGraph;



    bool setup(int sliceCount, const vec2 & angleGraphRange, const vec2 & sliceGraphRange) {
        s_sliceCount = sliceCount;
        s_angleGraphRange = angleGraphRange;
        s_sliceGraphRange = sliceGraphRange;

        // Angle graph

        vec2 min(k_initAngleGraphDomainMin, s_angleGraphRange.x);
        vec2 max(k_initAngleGraphDomainMax, s_angleGraphRange.y);
        vec2 size(max - min);
        vec2 center((min + max) * 0.5f);
        size *= k_angleGraphExcessFactor;
        min = center - size * 0.5f;
        max = center + size * 0.5f;

        s_angleGraph.reset(new ui::Graph(
            k_angleGraphTitleString,
            "Angle",
            min,
            max,
            k_minAngleGraphSize,
            k_maxAngleGraphSize
        ));
        s_angleGraph->addCurve("Lift", vec3(0.0f, 1.0f, 0.0f), int(180.0f * k_invAngleGranularity) + 1);
        s_angleGraph->addCurve("Drag", vec3(1.0f, 0.0f, 0.0f), int(180.0f * k_invAngleGranularity) + 1);
        s_angleGraph->addCurve("Torque", vec3(0.0f, 0.0f, 1.0f), int(180.0f * k_invAngleGranularity) + 1);

        // Slice graph

        min = vec2(0.0f, s_sliceGraphRange.x);
        max = vec2(s_sliceCount, s_sliceGraphRange.y);
        size = vec2(max - min);
        center = vec2((min + max) * 0.5f);
        size *= k_sliceGraphExcessFactor;
        min = center - size * 0.5f;
        max = center + size * 0.5f;

        s_sliceGraph.reset(new ui::Graph(
            k_sliceGraphTitleString,
            "Slice",
            min,
            max,
            k_minSliceGraphSize,
            k_maxSliceGraphSize
        ));
        s_sliceGraph->addCurve("Lift", vec3(0.0f, 1.0f, 0.0f), s_sliceCount);
        s_sliceGraph->addCurve("Drag", vec3(1.0f, 0.0f, 0.0f), s_sliceCount);
        s_sliceGraph->addCurve("Torque", vec3(0.0f, 0.0f, 1.0f), s_sliceCount);

        return true;
    }

    void update() {
        if (s_isAngleRecordChange) {
            auto & liftPoints(s_angleGraph->mutatePoints(0));
            auto & dragPoints(s_angleGraph->mutatePoints(1));
            auto & torqPoints(s_angleGraph->mutatePoints(2));
            liftPoints.clear();
            dragPoints.clear();
            torqPoints.clear();
            for (const auto & r : s_angleRecord) {
                liftPoints.emplace_back(r.first, r.second.lift.y); // TODO: figure out a better representation
                dragPoints.emplace_back(r.first, glm::length(r.second.drag)); // TODO: figure out a better representation
                torqPoints.emplace_back(r.first, r.second.torq.x); // TODO: figure out a better representation
            }

            s_isAngleRecordChange = false;
        }
        if (s_isSliceRecordChange) {
            auto & liftPoints(s_sliceGraph->mutatePoints(0));
            auto & dragPoints(s_sliceGraph->mutatePoints(1));
            auto & torqPoints(s_sliceGraph->mutatePoints(2));
            liftPoints.clear();
            dragPoints.clear();
            torqPoints.clear();
            for (const auto & r : s_sliceRecord) {
                liftPoints.emplace_back(r.first, r.second.lift.y); // TODO: figure out a better representation
                dragPoints.emplace_back(r.first, glm::length(r.second.drag)); // TODO: figure out a better representation
                torqPoints.emplace_back(r.first, r.second.torq.x); // TODO: figure out a better representation
            }

            s_isSliceRecordChange = false;
        }
    }

    void submitAngle(float angle, const Entry & entry) {
        angle = std::round(angle * k_invAngleGranularity) * k_angleGranularity;
        s_angleRecord[angle] = entry;
        s_isAngleRecordChange = true;
    }

    void clearAngles() {
        s_angleRecord.clear();
        s_isAngleRecordChange = true;
    }

    void submitSlice(int slice, const Entry & entry) {
        s_sliceRecord[slice] = entry;
        s_isSliceRecordChange = true;
    }

    void clearSlices() {
        s_sliceRecord.clear();
        s_isSliceRecordChange = true;
    }

    bool valAt(float angle, Entry & r_entry) {
        auto it(s_angleRecord.find(angle));
        if (it != s_angleRecord.end()) {
            r_entry = it->second;
            return true;
        }

        auto preIt(s_angleRecord.lower_bound(angle)), postIt(s_angleRecord.upper_bound(angle));
        if (preIt == s_angleRecord.begin() || postIt == s_angleRecord.end()) {
            return false;
        }
        --preIt;

        float preAngle(preIt->first);
        const Entry & preEntry(preIt->second);
        float postAngle(postIt->first);
        const Entry & postEntry(postIt->second);

        float interpP((angle - preAngle) / (postAngle - preAngle));
        r_entry.lift = glm::mix(preEntry.lift, postEntry.lift, interpP);
        r_entry.drag = glm::mix(preEntry.drag, postEntry.drag, interpP);
        r_entry.torq = glm::mix(preEntry.torq, postEntry.torq, interpP);
        return true;
    }

    const std::map<float, Entry> & angleRecord() {
        return s_angleRecord;
    }

    const std::map<int, Entry> & sliceRecord() {
        return s_sliceRecord;
    }

    shr<ui::Graph> angleGraph() {
        return s_angleGraph;
    }

    shr<ui::Graph> sliceGraph() {
        return s_sliceGraph;
    }

    void resetGraphs() {
        s_angleGraph->setView(
            vec2(k_initAngleGraphDomainMin, s_angleGraphRange.x),
            vec2(k_initAngleGraphDomainMax, s_angleGraphRange.y)
        );
        s_sliceGraph->setView(
            vec2(0, s_sliceGraphRange.x),
            vec2(s_sliceCount, s_sliceGraphRange.y)
        );
    }

}
