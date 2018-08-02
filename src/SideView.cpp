#include "SideView.hpp"

#include <iostream>

#include "glm/gtc/matrix_transform.hpp"


#include "WindowManager.h"
#include "Program.h"



namespace sideview {
	static constexpr int k_nSlices = 50;
	static constexpr int k_width(720), k_height(480);
	static WindowManager * windowManager = nullptr;
	static unsigned int * nulldata;
	static std::shared_ptr<Program> outlineProg;
	GLuint outlineTopVAO, outlineBotVAO, outlineTopVBO, outlineBotVBO;
	glm::vec3 outlineTopVerts[k_nSlices];
	glm::vec3 outlineBotVerts[k_nSlices];
	glm::vec4 screenSpec; // screen width, screen height, x aspect factor, y aspect factor

	static bool setupShaders(const std::string & resourcesDir) {
		std::string shadersDir(resourcesDir + "/shaders");

		// Foil Shader -------------------------------------------------------------

		outlineProg = std::make_shared<Program>();
		outlineProg->setVerbose(true);
		outlineProg->setShaderNames(shadersDir + "/lineVert.glsl", shadersDir + "/lineFrag.glsl");
		if (!outlineProg->init()) {
			std::cerr << "Failed to initialize foil shader" << std::endl;
			return false;
		}
		outlineProg->addAttribute("topVertPos");
		outlineProg->addAttribute("botVertPos");
		outlineProg->addUniform("P");
		outlineProg->addUniform("V");
		outlineProg->addUniform("M");
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

		for (int i = 0; i < k_nSlices; i++) {
			outlineTopVerts[i] = glm::vec3(0, 0, -1);
			outlineBotVerts[i] = glm::vec3(0, 0, -1);
		}

		screenSpec.x = float(k_width);
		screenSpec.y = float(k_height);
		if (k_width >= k_height) {
			screenSpec.z = float(k_height) / float(k_width);
			screenSpec.w = 1.0f;
		}
		else {
			screenSpec.z = 1.0f;
			screenSpec.w = float(k_width) / float(k_height);
		}

		initGeom();

		return true;
	}

	void initGeom() {
		glGenVertexArrays(1, &outlineTopVAO);
		glBindVertexArray(outlineTopVAO);
		glGenBuffers(1, &outlineTopVBO);
		glBindBuffer(GL_ARRAY_BUFFER, outlineTopVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * k_nSlices, nullptr, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

		glGenVertexArrays(1, &outlineBotVAO);
		glBindVertexArray(outlineBotVAO);
		glGenBuffers(1, &outlineBotVBO);
		glBindBuffer(GL_ARRAY_BUFFER, outlineBotVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * k_nSlices, nullptr, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);;
	}

	void copyData() {
		glBindVertexArray(outlineTopVAO);
		glBindBuffer(GL_ARRAY_BUFFER, outlineTopVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * k_nSlices, &outlineTopVerts, GL_DYNAMIC_DRAW);

		glBindVertexArray(outlineBotVAO);
		glBindBuffer(GL_ARRAY_BUFFER, outlineBotVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * k_nSlices, &outlineBotVerts, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void render() {
		//don't draw anything until we have filled the outline verts at least once
		if (outlineTopVerts[k_nSlices - 1].z != 0)
			return;
		if (outlineBotVerts[k_nSlices - 1].z != 0)
			return;
		int test;
		glfwMakeContextCurrent(windowManager->getHandle());
		glClearColor(0.f, 0.f, 0.f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		copyData();

		glm::mat4 V, M, P;

		P = glm::ortho(-1.f / screenSpec.z, 1.f / screenSpec.z, //left and right
			-1.f / screenSpec.w, 1.f / screenSpec.w, //bottom and top
			-10.f, 10.f); //near and far

		V = glm::mat4(1);
		M = glm::mat4(1);
		M = glm::translate(glm::mat4(1), glm::vec3(-1, 0, 0));

		outlineProg->bind();
		glUniformMatrix4fv(outlineProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(outlineProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(outlineProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);

		glBindVertexArray(outlineBotVAO);
		glDrawArrays(GL_LINE_STRIP, 0, k_nSlices);

		glBindVertexArray(outlineTopVAO);
		glDrawArrays(GL_LINE_STRIP, 0, k_nSlices);

		glBindVertexArray(0);
		glfwSwapBuffers(windowManager->getHandle());

	}

	void submitOutline(int sliceNum, std::pair<glm::vec3, glm::vec3> outlinePoints)
	{
		outlineTopVerts[sliceNum] = glm::vec3(sliceNum*0.05f, outlinePoints.first.y, 0.f);
		outlineBotVerts[sliceNum] = glm::vec3(sliceNum*0.05f, outlinePoints.second.y, 0.f);
	}


}
