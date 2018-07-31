#pragma once



#include <string>

#include "glm/glm.hpp"



struct GLFWwindow;



namespace simulation {



bool setup(const std::string & resourceDir);

// Does one slice and returns if it was the last one
bool step();

// Renders current situation to screen
void render();

void cleanup();

GLFWwindow * getWindow();

// Returns the index of the slice that would be next
int getSlice();

// Returns the total number of slices
int getNSlices();

// Angle is in degrees
float getAngleOfAttack();

// Angle is in degrees
void setAngleOfAttack(float angle);

// Returns the lift of the previous sweep
glm::vec3 getLift();

// Returns the drag of the previous sweep
glm::vec3 getDrag();



}