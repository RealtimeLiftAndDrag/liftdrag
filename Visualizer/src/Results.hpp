#include <map>

#include "UI/Graph.hpp"

#include "RLD/RLD.hpp"



struct GLFWwindow;



namespace results {

    using rld::Result;

    bool setup(int sliceCount, const vec2 & s_angleGraphRange, const vec2 & s_sliceGraphRange);

    void update();

    // Submits the lift and drag at the given angle (in degrees) for processing and
    // visualization
    void submitAngle(float angle, const Result & result);

    // Clears all angle data
    void clearAngles();

    // Submits the lift and drag for the given slice for processing and
    // visualization
    void submitSlice(int slice, const Result & result);

    // Clears all slice data
    void clearSlices();

    // Returns the linearly interpolated lift and drag values at the given angle (in
    // degrees) and true, or false if the angle is out of interpolation range
    bool valAt(float angle, Result & r_result);

    const std::map<float, Result> & angleRecord();

    const std::map<int, Result> & sliceRecord();

    shr<ui::Graph> angleGraph();
    shr<ui::Graph> sliceGraph();

    void resetGraphs();



}