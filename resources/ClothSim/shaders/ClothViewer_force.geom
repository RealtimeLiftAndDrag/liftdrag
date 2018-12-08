#version 450 core

// Inputs ----------------------------------------------------------------------

layout(points) in;

// Outputs ---------------------------------------------------------------------

layout(line_strip, max_vertices = 2) out;
layout (location = 0) out float out_mag;

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

// Uniforms --------------------------------------------------------------------

uniform mat4 u_modelMat;
uniform mat3 u_normalMat;
uniform mat4 u_viewMat;
uniform mat4 u_projMat;

layout (binding = 0, std430) restrict buffer Vertices {
    Vertex u_vertices[];
};

// Functions -------------------------------------------------------------------

void main() {
    Vertex vertex = u_vertices[gl_PrimitiveIDIn];
    if (vertex.mass <= 0.0f) {
        return;
    }

    if (vertex.force0 != vec3(0.0f)) {
        vec3 acc = vertex.force0 / vertex.mass;
        float mag = length(acc);
        vec3 p2 = vertex.position + acc * 0.1f;

        out_mag = mag;
        gl_Position = u_projMat * u_viewMat * u_modelMat * vec4(vertex.position, 1.0f);
        EmitVertex();
    
        out_mag = mag;
        gl_Position = u_projMat * u_viewMat * u_modelMat * vec4(p2, 1.0f);
        EmitVertex();

        EndPrimitive();
    }
    
    if (vertex.force1 != vec3(0.0f)) {
        vec3 acc = vertex.force1 / vertex.mass;
        float mag = length(acc);
        vec3 p2 = vertex.position + acc * 0.1f;

        out_mag = -mag;
        gl_Position = u_projMat * u_viewMat * u_modelMat * vec4(vertex.position, 1.0f);
        EmitVertex();
    
        out_mag = -mag;
        gl_Position = u_projMat * u_viewMat * u_modelMat * vec4(p2, 1.0f);
        EmitVertex();

        EndPrimitive();
    }
}