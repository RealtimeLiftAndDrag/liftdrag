#pragma once



#include <string>

#include "Global.hpp"



struct GLFWwindow;



namespace SideView {

bool setupShaders(const std::string & resourcesDir);

bool setup(const std::string & resourcesDir, int sideTexID, GLFWwindow * mainWindow);

bool initGeom();

void copyOutlineData();

void render();

void submitOutline(int sliceNum, std::pair<vec3, vec3> outlinePoints);

GLFWwindow * getWindow();



}