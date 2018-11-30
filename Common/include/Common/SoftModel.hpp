#pragma once



#include <vector>

#include "Global.hpp"



struct SoftVertex {
    vec3 position;
    float mass;
    vec3 normal;
    float _0;
    vec3 force;
    float constraintFactor; // inverse of the number of constraints for this vertex
    vec3 prevPosition;
    s32 _1;
};

struct Constraint {
    u32 i; // index of first vertex
    u32 j; // index of second vertex
    float d; // rest distance between vertices
    float _0;
};

class SoftMesh {

    public:

    SoftMesh(std::vector<SoftVertex> && vertices, std::vector<u32> && indices, std::vector<Constraint> && constraints);
    SoftMesh(SoftMesh && other);

    bool load();

    void upload();

    void draw() const;

    int vertexCount() const { return int(m_vertices.size()); }

    int indexCount() const { return int(m_indices.size()); }

    int constraintCount() const { return int(m_constraints.size()); }

    std::vector<SoftVertex> & vertices() { return m_vertices; }
    const std::vector<SoftVertex> & vertices() const { return m_vertices; }

    std::vector<u32> & indices() { return m_indices; }
    const std::vector<u32> & indices() const { return m_indices; }

    std::vector<Constraint> & constraints() { return m_constraints; }
    const std::vector<Constraint> & constraints() const { return m_constraints; }

    u32 vertexBuffer() const { return m_vertexBuffer; }

    u32 indexBuffer() const { return m_indexBuffer; }

    u32 constraintBuffer() const { return m_constraintBuffer; }

    private:

    std::vector<SoftVertex> m_vertices;
    std::vector<u32> m_indices;
    std::vector<Constraint> m_constraints;
    u32 m_vertexBuffer;
    u32 m_indexBuffer;
    u32 m_constraintBuffer;
    u32 m_vao;

};

class SoftModel {

    private:

    SoftMesh m_mesh;

    public:

    SoftModel(SoftMesh && mesh);

    bool load();

    void draw() const;

    SoftMesh & mesh() { return m_mesh; }
    const SoftMesh & mesh() const { return m_mesh; }

};