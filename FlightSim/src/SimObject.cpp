#include "SimObject.hpp"
#include "glm/gtc/matrix_transform.hpp"

SimObject::SimObject() {
    gravityOn = true;
    timeScale = 1.f;
    mass = 1.f;

    pos = vec3(0);
    vel = vec3(0);
    acc = vec3(0);

    thrust_vel = vec3(0);

    a_pos = vec3(0);
    a_vel = vec3(0);
    a_acc = vec3(0);

}

mat4 SimObject::getTransform() {
    return getTranslate() * getRotate();
}

void SimObject::addTranslationalForce(vec3 force) {
    acc += (force / mass) * timeScale;
}

void SimObject::addAngularForce(vec3 force) {
    a_acc += force / 200000.f;
}

void SimObject::update() {
    vel += acc;
    pos += vel;

    a_vel += a_acc;
    a_pos += a_vel;

    if (thrust) {
        thrust_vel = vec3(vec4(0, 0, thrust, 1) * getRotate());
        pos += thrust_vel;
    }

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

void SimObject::setTimeScale(float _timeScale)
{
    timeScale = _timeScale;
}

void SimObject::setThrust(float _thrust)
{
    thrust = _thrust;
}

mat4 SimObject::getTranslate()
{
    return glm::translate(mat4(1), pos);
}

mat4 SimObject::getRotate()
{
    mat4 R = mat4(1);
    R = glm::rotate(R, a_pos.x, vec3(1, 0, 0));
    R = glm::rotate(R, a_pos.y, vec3(0, 1, 0));
    R = glm::rotate(R, a_pos.z, vec3(0, 0, 1));
    return R;
}

mat4 SimObject::getTransformRelativeTo(vec3 dir)
{
    
    // 2. Calculate cameraDirection
    glm::vec3 zaxis = glm::normalize(vec3(0, 0, 0) - dir);
    // 3. Get positive right axis vector
    glm::vec3 xaxis = vec3(1, 0, 0);
    // 4. Calculate camera up vector
    glm::vec3 yaxis = glm::cross(zaxis, xaxis);
    return mat4();
}
