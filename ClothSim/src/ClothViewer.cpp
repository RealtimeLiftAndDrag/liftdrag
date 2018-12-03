#include "ClothViewer.hpp"

#include <iostream>

#include "glad/glad.h"

#include "Common/Shader.hpp"
#include "Common/Camera.hpp"



static constexpr bool k_viewConstraints(false);

static const float k_fov(glm::radians(90.0f));
static const float k_camPanAngle(0.01f);
static const float k_camZoomAmount(0.1f);
static const float k_camMinDist(0.5f);
static const float k_camMaxDist(3.0f);

static bool s_isSetup(false);
static unq<Shader> s_clothShader;
static unq<Shader> s_constraintsShader;
static unq<Shader> s_frameShader;
static u32 s_constraintVAO;
static u32 s_frameVBO, s_frameIBO, s_frameVAO;



bool setup() {
    // Setup shaders
    std::string shadersPath(g_resourcesDir + "/ClothSim/shaders/");
    // Cloth shader
    if (!(s_clothShader = Shader::load(shadersPath + "ClothViewer_cloth.vert", shadersPath + "ClothViewer_cloth.frag"))) {
        std::cerr << "Failed to load cloth shader" << std::endl;
        return false;
    }
    // Constraints shader
    if (!(s_constraintsShader = Shader::load(shadersPath + "ClothViewer_constraints.vert", shadersPath + "ClothViewer_constraints.geom", shadersPath + "ClothViewer_constraints.frag"))) {
        std::cerr << "Failed to load constraints shader" << std::endl;
        return false;
    }
    // Frame shader
    if (!(s_frameShader = Shader::load(shadersPath + "ClothViewer_frame.vert", shadersPath + "ClothViewer_frame.frag"))) {
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

    glGenVertexArrays(1, &s_constraintVAO);

    return true;
}



ClothViewerComponent::ClothViewerComponent(const Model & model, float scale, const ivec2 & minSize) :
    Single(minSize, ivec2()),
    m_model(model),
    m_camera(scale * 0.1f, scale * 10.0f),
    m_isTouch(false),
    m_touchP()
{
    m_camera.zoom(0.5f);
    m_camera.near(scale * 0.01f);
    m_camera.far(scale * 100.0f);

    if (!s_isSetup) {
        setup();
        s_isSetup = true;
    }
}

void ClothViewerComponent::render() const {
    glEnable(GL_DEPTH_TEST);
    setViewport();

    // Render cloth
    if (k_viewConstraints) {
        s_constraintsShader->bind();
        s_constraintsShader->uniform("u_modelMat", mat4());
        s_constraintsShader->uniform("u_normalMat", mat3());
        s_constraintsShader->uniform("u_viewMat", m_camera.viewMat());
        s_constraintsShader->uniform("u_projMat", m_camera.projMat());
        const SoftMesh & mesh(static_cast<const SoftMesh &>(m_model.subModels().front().mesh()));
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mesh.vertexBuffer());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mesh.constraintBuffer());
        glBindVertexArray(s_constraintVAO);
        glDrawArrays(GL_POINTS, 0, mesh.constraintCount());
        glBindVertexArray(0);
    }
    else {
        s_clothShader->bind();
        s_clothShader->uniform("u_modelMat", mat4());
        s_clothShader->uniform("u_normalMat", mat3());
        s_clothShader->uniform("u_viewMat", m_camera.viewMat());
        s_clothShader->uniform("u_projMat", m_camera.projMat());
        s_clothShader->uniform("u_camPos", m_camera.position());
        model().draw();
    }

    // Render frame
    s_frameShader->bind();
    s_frameShader->uniform("u_frameSize", vec3(windframeWidth(), windframeWidth(), windframeDepth()));
    s_frameShader->uniform("u_viewMat", m_camera.viewMat());
    s_frameShader->uniform("u_projMat", m_camera.projMat());
    glBindVertexArray(s_frameVAO);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    Shader::unbind();
    glDisable(GL_DEPTH_TEST);

    glDisable(GL_DEPTH_TEST);
}

void ClothViewerComponent::pack() {
    if (size().y <= size().x) {
        m_camera.fov(vec2(k_fov, k_fov * float(size().y) / float(size().x)));
    }
    else {
        m_camera.fov(vec2(k_fov * float(size().x) / float(size().y), k_fov));
    }
}

void ClothViewerComponent::cursorPositionEvent(const ivec2 & pos, const ivec2 & delta) {
    if (ui::isMouseButtonPressed(0)) {
        m_camera.thetaPhi(
            m_camera.theta() - float(delta.x) * k_camPanAngle,
            m_camera.phi() + float(delta.y) * k_camPanAngle
        );
    }

    if (m_isTouch) {
        detTouchPoint();
    }
}

void ClothViewerComponent::cursorExitEvent() {
    m_isTouch = false;
}

void ClothViewerComponent::mouseButtonEvent(int button, int action, int mods) {
    if (button == 1) {
        if (action) {
            if (!mods) {
                m_isTouch = true;
                detTouchPoint();
            }
        }
        else {
            m_isTouch = false;
        }
    }
}

void ClothViewerComponent::scrollEvent(const ivec2 & delta) {
    m_camera.zoom(m_camera.zoom() - float(delta.y) * k_camZoomAmount);
}

void ClothViewerComponent::detTouchPoint() {
    m_touchP = vec2(ui::cursorPosition() - position());
    m_touchP /= size();
    m_touchP = m_touchP * 2.0f - 1.0f;
    m_touchP *= aspect();
}