#include <string>
#include "glm/glm.hpp"



struct GLFWwindow;



namespace sideview {

	bool setup(const std::string & resourcesDir, GLFWwindow * mainWindow);

	void cleanup();

	void render();
	void submitOutline(int sliceNum, std::pair<glm::vec3, glm::vec3> outlinePoints);
}