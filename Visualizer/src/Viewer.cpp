#include "Viewer.hpp"

#include <iostream>

#include "glad/glad.h"

#include "Common/Shader.hpp"
#include "Common/Camera.hpp"



namespace Viewer {

    static const ivec2 k_minCompSize(300);
    static const float k_fov(glm::radians(90.0f));
    static const float k_camPanAngle(0.01f);
    static const float k_camZoomAmount(0.1f);
    static const float k_camMinDist(1.0f);
    static const float k_camMaxDist(10.0f);

    static unq<Shader> s_objectShader, s_frameShader;
    static ThirdPersonCamera s_camera;
    static shr<ViewerComponent> s_component;
    static uint s_frameVBO, s_frameIBO, s_frameVAO;



    ViewerComponent::ViewerComponent() :
        Single(k_minCompSize, ivec2())
    {}

    void ViewerComponent::render() const {
        glEnable(GL_DEPTH_TEST);
        setViewport();

        s_objectShader->bind();
        s_objectShader->uniform("u_viewMat", s_camera.viewMat());
        s_objectShader->uniform("u_projMat", s_camera.projMat());
        model().draw(modelMat(), normalMat(), s_objectShader->uniformLocation("u_modelMat"), s_objectShader->uniformLocation("u_normalMat"));

        s_frameShader->bind();
        s_frameShader->uniform("u_frameSize", vec3(windframeWidth(), windframeWidth(), windframeDepth()));
        s_frameShader->uniform("u_viewMat", s_camera.viewMat());
        s_frameShader->uniform("u_projMat", s_camera.projMat());
        glBindVertexArray(s_frameVAO);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

        Shader::unbind();
        glDisable(GL_DEPTH_TEST);
    }

    void ViewerComponent::pack() {
        if (size().y <= size().x) {
            s_camera.fov(vec2(k_fov * float(size().x) / float(size().y), k_fov));
        }
        else {
            s_camera.fov(vec2(k_fov, k_fov * float(size().y) / float(size().x)));
        }
    }

    void ViewerComponent::cursorPositionEvent(const ivec2 & pos, const ivec2 & delta) {
        if (ui::isMouseButtonPressed(0)) {
            s_camera.thetaPhi(
                s_camera.theta() - float(delta.x) * k_camPanAngle,
                s_camera.phi() + float(delta.y) * k_camPanAngle
            );
        }
    }

    void ViewerComponent::scrollEvent(const ivec2 & delta) {
        float distP((s_camera.distance() - k_camMinDist) / (k_camMaxDist - k_camMinDist));
        float zoom(std::sqrt(distP));
        zoom = glm::clamp(zoom - float(delta.y) * k_camZoomAmount, 0.0f, 1.0f);
        distP = zoom * zoom;
        s_camera.distance(k_camMinDist + distP * (k_camMaxDist - k_camMinDist));
    }

    bool setup() {
        // Setup shaders
        std::string shaderPath(g_resourcesDir + "/Visualizer/shaders/");
        if (!(s_objectShader = Shader::load(shaderPath + "viewer_object.vert", shaderPath + "viewer_object.frag"))) {
            std::cerr << "Failed to load object shader" << std::endl;
            return false;
        }
        if (!(s_frameShader = Shader::load(shaderPath + "viewer_frame.vert", shaderPath + "viewer_frame.frag"))) {
            std::cerr << "Failed to load frame shader" << std::endl;
            return false;
        }

        // Setup frame
        vec3 positions[8]{
            { -1.0f, -1.0f, -1.0f },
            {  1.0f, -1.0f, -1.0f },
            { -1.0f,  1.0f, -1.0f },
            {  1.0f,  1.0f, -1.0f },
            { -1.0f, -1.0f,  1.0f },
            {  1.0f, -1.0f,  1.0f },
            { -1.0f,  1.0f,  1.0f },
            {  1.0f,  1.0f,  1.0f }
        };
        u32 indices[24]{
            0, 1,   2, 3,   4, 5,   6, 7,
            0, 2,   4, 6,   1, 3,   5, 7,
            0, 4,   1, 5,   2, 6,   3, 7
        };
        glGenVertexArrays(1, &s_frameVAO);
        glGenBuffers(1, &s_frameVBO);
        glGenBuffers(1, &s_frameIBO);
        glBindVertexArray(s_frameVAO);
        glBindBuffer(GL_ARRAY_BUFFER, s_frameVBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_frameIBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, nullptr);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        if (glGetError() != GL_NO_ERROR) {
            std::cerr << "OpenGL error setting up frame" << std::endl;
            return false;
        }

        // Setup camera
        s_camera.distance(k_camMinDist);
        s_camera.near(0.01f);
        s_camera.far(100.0f);
        s_camera.fov(vec2(glm::pi<float>() * 0.5f));

        s_component.reset(new ViewerComponent());

        return true;
    }

    shr<ViewerComponent> component() {
        return s_component;
    }

}