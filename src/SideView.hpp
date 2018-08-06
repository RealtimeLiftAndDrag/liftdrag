#pragma once



#include <string>
#include "glm/glm.hpp"



struct GLFWwindow;



namespace sideview {

bool setupShaders(const std::string & resourcesDir);

bool setup(const std::string & resourcesDir, int sideTexID, GLFWwindow * mainWindow);

bool initGeom();

void copyOutlineData();

void render();

void submitOutline(int sliceNum, std::pair<glm::vec3, glm::vec3> outlinePoints);

GLFWwindow * getWindow();



}