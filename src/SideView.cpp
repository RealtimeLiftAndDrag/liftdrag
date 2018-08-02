#include "SideView.hpp"

#include <iostream>

#include "WindowManager.h"
#include "Program.h"



namespace sideview {
	static constexpr int k_width(720), k_height(480);
	static WindowManager * windowManager = nullptr;
	static unsigned int * nulldata;
	static std::shared_ptr<Program> sideProg;
	GLuint outlineVAO, outlineVBO;
	glm::vec3 outlineVerts [100];

	static bool setupShaders(const std::string & resourcesDir) {
		std::string shadersDir(resourcesDir + "/shaders");

		// Foil Shader -------------------------------------------------------------

		sideProg = std::make_shared<Program>();
		sideProg->setVerbose(true);
		sideProg->setShaderNames(shadersDir + "/vs.glsl", shadersDir + "/fs.glsl");
		if (!sideProg->init()) {
			std::cerr << "Failed to initialize foil shader" << std::endl;
			return false;
		}
		sideProg->addAttribute("vertPos");
		sideProg->addUniform("P");
		sideProg->addUniform("V");
		sideProg->addUniform("M");
	}

	bool setup(const std::string & resourcesDir, GLFWwindow * mainWindow) {
		windowManager = new WindowManager();
		if (!windowManager->init(k_width, k_height)) {
			std::cerr << "Failed to initialize window manager" << std::endl;
			return false;
		}

		// Initialize nulldata
		nulldata = new unsigned int[k_width * k_height * 2]{}; // zero initialization

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glEnable(GL_DEPTH_TEST);

		// Setup shaders
		if (!setupShaders(resourcesDir)) {
			std::cerr << "Failed to setup shaders" << std::endl;
			return false;
		}

		for (int i = 0; i < 100; i++) {
			outlineVerts[i] = glm::vec3(0, 0, -1);
		}

		return true;
	}




	void submitOutline(int sliceNum, std::pair<glm::vec3, glm::vec3> outlinePoints)
	{
		outlineVerts[sliceNum] = glm::vec3((float)sliceNum, outlinePoints.first.y, 0.f);
		outlineVerts[sliceNum + 50] = glm::vec3(sliceNum, outlinePoints.second.y, 0);
	}

	void initGeom() {
		glGenVertexArrays(1, &outlineVAO);
		glBindVertexArray(outlineVAO);
		glGenBuffers(1, &outlineVBO);


	}
}
