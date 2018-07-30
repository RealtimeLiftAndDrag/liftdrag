#include <string>



struct GLFWwindow;



namespace results {



bool setup(const std::string & resourcesDir, GLFWwindow * mainWindow);

void cleanup();

void render();

// Submits the lift and drag at the given angle (in degrees) for processing and
// visualization
void submit(float angle, float lift, float drag);

// Returns the linearly interpolated lift and drag values at the given angle (in
// degrees) and true, or false if the angle is out of interpolation range
bool valAt(float angle, float & r_lift, float & r_drag);



}