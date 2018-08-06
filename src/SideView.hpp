#pragma once
#include <string>
#include "glm/glm.hpp"


struct GLFWwindow;



namespace sideview {
	bool setup(const std::string & resourcesDir, int sideTexID, GLFWwindow * mainWindow);
	void render();
}