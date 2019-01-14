#version 450 core

// Inputs ----------------------------------------------------------------------

layout(points) in;

// Outputs ---------------------------------------------------------------------

layout(line_strip, max_vertices = 2) out;
layout (location = 0) out float out_stress;

// Types -----------------------------------------------------------------------

struct Vertex {
    vec3 position;
    float mass;
    vec3 normal;
    int group;
    vec3 prevPosition;
    float _0;
    vec3 force0; // lift
    float _1;
    vec3 force1; // drag
    float _2;
};

struct Constraint {
    int i;
    int j;
    float d;
    float padding0;
};

// Uniforms --------------------------------------------------------------------

uniform mat4 u_modelMat;
uniform mat3 u_normalMat;
uniform mat4 u_viewMat;
uniform mat4 u_projMat;

layout (binding = 0, std430) restrict buffer Vertices {
    Vertex u_vertices[];
};

layout (binding = 2, std430) restrict buffer Constraints {
    Constraint u_constraints[];
};

// Functions -------------------------------------------------------------------

void main() {
    Constraint c = u_constraints[gl_PrimitiveIDIn];
    if (c.i == c.j) {
        return;
    }

    vec3 p1 = u_vertices[c.i].position;
    vec3 p2 = u_vertices[c.j].position;
    float dist = distance(p1, p2);
    float stress = dist > c.d ? dist / c.d - 1.0f : 1.0f - c.d / dist;

    out_stress = stress;
    gl_Position = u_projMat * u_viewMat * u_modelMat * vec4(p1, 1.0f);
    EmitVertex();

    out_stress = stress;
    gl_Position = u_projMat * u_viewMat * u_modelMat * vec4(p2, 1.0f);
    EmitVertex();

    EndPrimitive();
}