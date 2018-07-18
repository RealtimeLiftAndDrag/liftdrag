#include <string>



namespace results {



bool setup(const std::string & resourcesDir);

void cleanup();

void render();

// Submits the lift and drag at the given angle (in degrees) for processing and
// visualization
void submit(float angle, float lift, float drag);



}