#include "Camera.hpp"



#include "glm/gtx/transform.hpp"

#include "Util.hpp"



static const vec2 k_defFOV(glm::radians(90.0f));
static constexpr float k_defNear(0.1f);
static constexpr float k_defFar(100.0f);



Camera::Camera() :
    m_position(),
    m_orientation(),
    m_orientMatrix(),
    m_fov(k_defFOV),
    m_near(k_defNear),
    m_far(k_defFar),
    m_viewMat(),
    m_isViewMatValid(false),
    m_projMat(),
    m_isProjMatValid(false)
{}

void Camera::position(const vec3 & position) {
    m_position = position;
    m_isViewMatValid = false;
}

void Camera::orientation(const quat & orientation) {
    m_orientation = orientation;
    m_orientMatrix = glm::toMat3(m_orientation);
    m_isViewMatValid = false;
}

void Camera::orientMatrix(const mat3 & orientMatrix) {
    m_orientMatrix = orientMatrix;
    m_orientation = glm::toQuat(m_orientMatrix);
    m_isViewMatValid = false;
}

void Camera::uvw(const vec3 & u, const vec3 & v, const vec3 & w) {
    Camera::orientMatrix(mat3(u, v, w));
}

void Camera::fov(const vec2 & fov) {
    m_fov = fov;
    m_isProjMatValid = false;
}

void Camera::near(float near) {
    m_near = near;
    m_isProjMatValid = false;
}

void Camera::far(float far) {
    m_far = far;
    m_isProjMatValid = false;
}

const mat4 & Camera::viewMat() const {
    if (!m_isViewMatValid) {
        vec3 t(-m_position);
        m_viewMat = mat4(
            m_u.x, m_v.x, m_w.x, 0.0f,
            m_u.y, m_v.y, m_w.y, 0.0f,
            m_u.z, m_v.z, m_w.z, 0.0f,
            glm::dot(m_u, t), glm::dot(m_v, t), glm::dot(m_w, t), 1.0f
        );
        m_isViewMatValid = true;
    }
    return m_viewMat;
}

const mat4 & Camera::projMat() const {
    if (!m_isProjMatValid) {
        m_projMat = glm::perspective(m_fov.y, m_fov.x / m_fov.y, m_near, m_far);
        m_isProjMatValid = true;
    }
    return m_projMat;
}



ThirdPersonCamera::ThirdPersonCamera(float minDistance, float maxDistance) :
    m_distance(minDistance),
    m_theta(0.0f),
    m_phi(glm::pi<float>() * 0.5f),
    m_minDistance(minDistance),
    m_maxDistance(maxDistance),
    m_zoom(0.0f)
{}

void ThirdPersonCamera::position(const vec3 & position) {
    Camera::position(position);
    updateDownPosition();
}

void ThirdPersonCamera::orientation(const quat & orientation) {
    Camera::orientation(orientation);
    updateDownOrientation();
}

void ThirdPersonCamera::orientMatrix(const mat3 & orientMatrix) {
    Camera::orientMatrix(orientMatrix);
    updateDownOrientation();
}

void ThirdPersonCamera::distance(float distance) {
    m_distance = glm::clamp(distance, m_minDistance, m_maxDistance);
    m_zoom = std::sqrt((m_distance - m_minDistance) / (m_maxDistance - m_minDistance));
    updateUp();
}

void ThirdPersonCamera::thetaPhi(float theta, float phi) {
    if      (theta >  glm::pi<float>()) theta = std::fmod(theta, glm::pi<float>()) - glm::pi<float>();
    else if (theta < -glm::pi<float>()) theta = glm::pi<float>() - std::fmod(-theta, glm::pi<float>());
    m_theta = theta;
    m_phi = glm::clamp(phi, 0.0f, glm::pi<float>());
    updateUp();
}

void ThirdPersonCamera::zoom(float zoom) {
    m_zoom = glm::clamp(zoom, 0.0f, 1.0f);
    m_distance = m_zoom * m_zoom * (m_maxDistance - m_minDistance) + m_minDistance;
    updateUp();
}

void ThirdPersonCamera::updateUp() {
    vec3 xzP(std::sin(m_theta), 0.0f, std::cos(m_theta));
    float sinPhi(std::sin(m_phi));
    vec3 w(xzP.x * sinPhi, std::cos(m_phi), xzP.z * sinPhi);
    vec3 u(xzP.z, 0.0f, -xzP.x);
    uvw(u, glm::cross(w, u), w);
    Camera::position(w * m_distance);
}

void ThirdPersonCamera::updateDownPosition() {
    if (util::isZero(position())) {
        m_distance = m_minDistance;
        m_theta = 0.0f;
        m_phi = glm::pi<float>() * 0.5f;
        m_zoom = 0.0f;
    }
    else {
        m_distance = glm::clamp(glm::length(position()), m_minDistance, m_maxDistance);
        m_phi = std::acos(position().y / m_distance);
        vec2 xzP(position().z, position().x);
        if (util::isZero(xzP)) {
            m_theta = 0.0f;
        }
        else {
            xzP = glm::normalize(xzP);
            m_theta = std::atan2(xzP.y, xzP.x);
        }
        m_zoom = std::sqrt((m_distance - m_minDistance) / (m_maxDistance - m_minDistance));
    }
    updateUp();
}

void ThirdPersonCamera::updateDownOrientation() {
    position(w() * m_distance);
}