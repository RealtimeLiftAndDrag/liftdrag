#include "SoftModel.hpp"

#include <iostream>

#include "glad/glad.h"



SoftMesh::SoftMesh(std::vector<SoftVertex> && vertices, std::vector<u32> && indices, std::vector<Constraint> && constraints) :
    m_vertices(move(vertices)),
    m_indices(move(indices)),
    m_constraints(move(constraints)),
    m_vertexBuffer(0),
    m_indexBuffer(0),
    m_constraintBuffer(0),
    m_vao(0)
{}

SoftMesh::SoftMesh(SoftMesh && other) :
    m_vertices(move(other.m_vertices)),
    m_indices(move(other.m_indices)),
    m_constraints(move(other.m_constraints)),
    m_vertexBuffer(other.m_vertexBuffer),
    m_indexBuffer(other.m_indexBuffer),
    m_constraintBuffer(other.m_constraintBuffer),
    m_vao(other.m_vao)
{
    other.m_vertexBuffer = 0;
    other.m_indexBuffer = 0;
    other.m_constraintBuffer = 0;
    other.m_vao = 0;
}


bool SoftMesh::load() {
    glGenBuffers(1, &m_vertexBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_vertexBuffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, m_vertices.size() * sizeof(SoftVertex), m_vertices.data(), GL_DYNAMIC_STORAGE_BIT); // TODO: can remove dynamic if not using cpu physics
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenBuffers(1, &m_indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(u32), m_indices.data(), 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glGenBuffers(1, &m_constraintBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_constraintBuffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, m_constraints.size() * sizeof(Constraint), m_constraints.data(), 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer); // this works, thankfully
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer); // and so does this
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(SoftVertex), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(SoftVertex), reinterpret_cast<const void *>(sizeof(vec4)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    if (glGetError() != GL_NO_ERROR) {
        std::cerr << "OpenGL error" << std::endl;
        return false;
    }

    return true;
}

void SoftMesh::upload() {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_vertexBuffer);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, m_vertices.size() * sizeof(SoftVertex), m_vertices.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void SoftMesh::draw() const {
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, u32(m_indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}



SoftModel::SoftModel(SoftMesh && mesh) :
    m_mesh(move(mesh))
{}

bool SoftModel::load() {
    return m_mesh.load();
}

void SoftModel::draw() const {
    m_mesh.draw();
}