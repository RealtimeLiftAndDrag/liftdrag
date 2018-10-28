#include <map>

#include "Interface/Graph.hpp"



struct GLFWwindow;



namespace Results {

    struct Entry { vec3 lift, drag, torq; };

    bool setup(int sliceCount);

    void update();

    // Submits the lift and drag at the given angle (in degrees) for processing and
    // visualization
    void submitAngle(float angle, const Entry & entry);

    // Clears all angle data
    void clearAngles();

    // Submits the lift and drag for the given slice for processing and
    // visualization
    void submitSlice(int slice, const Entry & entry);

    // Clears all slice data
    void clearSlices();

    // Returns the linearly interpolated lift and drag values at the given angle (in
    // degrees) and true, or false if the angle is out of interpolation range
    bool valAt(float angle, Entry & r_entry);

    const std::map<float, Entry> & angleRecord();

    const std::map<int, Entry> & sliceRecord();

    shr<UI::Graph> angleGraph();
    shr<UI::Graph> sliceGraph();

    void resetGraphs();



}