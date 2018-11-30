#include "ClothCPU.hpp"

#include "glm/gtc/constants.hpp"



using Vertex = SoftVertex;

const vec3 k_gravity(0.0f, -9.8f, 0.0f);
const float k_damping(0.01f);
const int k_constraintPasses(16);
const bool k_doSphere(true);
const bool k_doSin(false);
const float k_sphereRadius(0.4f);
const float k_sphereRadius2(k_sphereRadius * k_sphereRadius);
const vec3 k_sphereStartP(0.0f, -0.25f, 1.0f);
const vec3 k_sphereEndP(0.0f, -0.25f, -1.0f);



static void doSphere(std::vector<Vertex> & vertices, float t) {
    vec3 sphereOrigin(glm::mix(k_sphereStartP, k_sphereEndP, t));
    for (Vertex & v : vertices) {
        vec3 d(v.position - sphereOrigin);
        float dist2(glm::dot(d, d));
        if (dist2 < k_sphereRadius2) {
            v.position = sphereOrigin + d / sqrt(dist2) * k_sphereRadius;
        }
    }
}

static void doSin(std::vector<Vertex> & vertices, float t) {
    float sval(t * 2.0f * glm::pi<float>());
    for (Vertex & v : vertices) {
        v.position.z = sin(sval + v.position.x * v.position.y * 100.0f) * 0.025f;
    }
}

static void update(std::vector<Vertex> & vertices, float dt) {
    float dt2(dt * dt);
    for (Vertex & v : vertices) {
        vec3 pos(v.position);
        if (v.mass > 0.0f) {
            vec3 acceleration(v.force / v.mass + k_gravity);
            v.position += (pos - v.prevPosition) * (1.0f - k_damping) + acceleration * dt2;
        }
        v.force = vec3(0.0f);
        v.prevPosition = pos;
    }
}

void constrain(std::vector<Vertex> & vertices, const std::vector<Constraint> & constraints) {
    for (const Constraint & c : constraints) {
        Vertex & v1(vertices[c.i]);
        Vertex & v2(vertices[c.j]);
        vec3 delta(v2.position - v1.position);
        vec3 correction(delta * (1.0f - c.d / glm::length(delta)));
        if (v1.mass > 0.0f || v2.mass > 0.0f) {
            float p(v1.mass / (v1.mass + v2.mass));
            vertices[c.i].force += correction * p;
            vertices[c.j].force += correction * (p - 1.0f);
        }
    }
    
    for (Vertex & v : vertices) {
        v.position += v.force * v.constraintFactor;
        v.force = vec3(0.0f);
    }
}



void doCPU(SoftModel & model, float t, float dt) {
    SoftMesh & mesh(model.mesh());
    std::vector<SoftVertex> & vertices(mesh.vertices());
    const std::vector<Constraint> & constraints(mesh.constraints());

    if (k_doSphere) {
        doSphere(vertices, t);
    }
    else if (k_doSin) {
        doSin(vertices, t);
    }

    // Update vertices
    update(vertices, dt);

    // Constrain
    for (int pass(0); pass < k_constraintPasses; ++pass) {
        constrain(vertices, constraints);
    }

    mesh.upload();
}