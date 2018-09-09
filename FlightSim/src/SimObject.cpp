#include "SimObject.hpp"
#include "glm/gtc/matrix_transform.hpp"

SimObject::SimObject() {
    gravityOn = true;
    timeScale = 1.f;
    mass = 1.f;

    pos = vec3(0);
    vel = vec3(0);
    acc = vec3(0);

    a_pos = vec3(0);
    a_vel = vec3(0);
    a_acc = vec3(0);

}

mat4 SimObject::getTransform() {
    mat4 T, R;

    T = glm::translate(mat4(1), pos);

    R = mat4(1);
    R = glm::rotate(R, a_pos.x, vec3(1, 0, 0));
    R = glm::rotate(R, a_pos.y, vec3(0, 1, 0));
    R = glm::rotate(R, a_pos.z, vec3(0, 0, 1));

    return T * R;
}

void SimObject::addTranslationalForce(vec3 force) {
    acc += (force / mass) / timeScale;
}

void SimObject::addAngularForce(vec3 force) {
    a_acc += force / mass;
}

void SimObject::update() {
    vel += acc;
    pos += vel;

    a_vel += a_acc;
    a_pos += a_vel;

    acc = vec3(0);
    a_acc = vec3(0);

    if (gravityOn)
        acc += vec3(0, -9.8, 0);
}

void SimObject::setGravityOn(bool _gravityOn) {
    gravityOn = _gravityOn;
}

void SimObject::setMass(float _mass) {
    mass = _mass;
}