#include <string>

#include "Common/Graph.hpp"



struct GLFWwindow;



namespace Results {



bool setup(const std::string & resourcesDir);

void update();

// Submits the lift and drag at the given angle (in degrees) for processing and
// visualization
void submitAngle(float angle, float lift, float drag);

// Clears all angle data
void clearAngles();

// Submits the lift and drag for the given slice for processing and
// visualization
void submitSlice(int slice, float lift, float drag);

// Clears all slice data
void clearSlices();

// Returns the linearly interpolated lift and drag values at the given angle (in
// degrees) and true, or false if the angle is out of interpolation range
bool valAt(float angle, float & r_lift, float & r_drag);

shr<Graph> angleGraph();
shr<Graph> sliceGraph();

void resetGraphs();



}