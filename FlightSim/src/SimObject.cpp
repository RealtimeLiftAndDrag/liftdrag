#include "SimObject.hpp"

#include "glm/gtc/matrix_transform.hpp"

SimObject::SimObject(float mass, const vec3 & momentsOfInertia, float dryThrust, const vec3 & position, const vec3 & direction, float speed) :
    k_mass(mass),
    k_invMass(1.0f / k_mass),
    k_momentsOfInertia(momentsOfInertia),
    k_dryThrust(dryThrust),
    m_position(position),
    m_velocity(direction * speed),
    m_acceleration(),
    m_orientation(),
    m_orientMatrix(),
    m_angularVel(),
    m_angularAcc(),
    m_thrust(0.0f)
{
    m_w = -direction;
    m_u = glm::normalize(glm::cross(vec3(0.0f, 1.0f, 0.0f), m_w)); // TODO: this will break if `direction` is parallel to y axis
    m_v = glm::cross(m_w, m_u);
    m_orientation = glm::toQuat(m_orientMatrix);
}

void SimObject::addTranslationalForce(const vec3 & force) {
    m_acceleration += (force * k_invMass);
}

void SimObject::addAngularForce(const vec3 & force) {
    m_angularAcc += m_orientMatrix * ((glm::transpose(m_orientMatrix) * force) / k_momentsOfInertia); // TODO: transposeMult function
}

void SimObject::update(float dt) {
    vec3 thrustForce = m_w * -(m_thrust * k_dryThrust);
    addTranslationalForce(thrustForce);

    m_velocity += m_acceleration * dt;
    m_position += m_velocity * dt;
    m_angularVel += m_angularAcc * dt;
    float angularSpeed(glm::length(m_angularVel));
    if (angularSpeed > 0.0f) {
        m_orientation = glm::angleAxis(angularSpeed * dt, m_angularVel / angularSpeed) * m_orientation;
        m_orientMatrix = glm::toMat3(m_orientation);
    }

    m_acceleration = vec3();
    m_angularAcc = vec3();
}

void SimObject::thrust(float thrust) {
    m_thrust = thrust;
}
