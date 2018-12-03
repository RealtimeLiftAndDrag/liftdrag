#include "Model.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

#include "glad/glad.h"
#include "tinyobjloader/tiny_obj_loader.h"
#include "GRLLoader.hpp"

#include "Util.hpp"



HardMesh::HardMesh(std::vector<Vertex> && vertices, std::vector<u32> && indices) :
    m_vertices(move(vertices)),
    m_indices(move(indices)),
    m_vertexBuffer(0),
    m_indexBuffer(0),
    m_vao(0)
{}

HardMesh::HardMesh(std::vector<Vertex> && vertices) :
    HardMesh(move(vertices), std::vector<u32>())
{}

HardMesh::HardMesh(HardMesh && other) :
    m_vertices(move(other.m_vertices)),
    m_indices(move(other.m_indices)),
    m_vertexBuffer(other.m_vertexBuffer),
    m_indexBuffer(other.m_indexBuffer),
    m_vao(other.m_vao)
{
    other.m_vertexBuffer = 0;
    other.m_indexBuffer = 0;
    other.m_vao = 0;
}

bool HardMesh::load() {
    // Vertex buffer
    glGenBuffers(1, &m_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferStorage(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Index buffer
    if (m_indices.size()) {
        glGenBuffers(1, &m_indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
        glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(u32), m_indices.data(), 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    // VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    if (m_indices.size()) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void *>(           0));
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void *>(sizeof(vec4)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    if (glGetError()) {
        std::cerr << "OpenGL error" << std::endl;
        return false;
    }

    return true;
}

void HardMesh::draw() const {
    glBindVertexArray(m_vao);
    if (m_indices.size()) {
        glDrawElements(GL_TRIANGLES, u32(m_indices.size()), GL_UNSIGNED_INT, reinterpret_cast<const void *>(0));
    }
    else {
        glDrawArrays(GL_TRIANGLES, 0, u32(m_vertices.size()));
    }
    glBindVertexArray(0);
}



SoftMesh::SoftMesh(std::vector<Vertex> && vertices, std::vector<u32> && indices, std::vector<Constraint> && constraints) :
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
    // Vertex buffer
    glGenBuffers(1, &m_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferStorage(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Index buffer
    glGenBuffers(1, &m_indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(u32), m_indices.data(), 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Constraint buffer
    glGenBuffers(1, &m_constraintBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_constraintBuffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, m_constraints.size() * sizeof(Constraint), m_constraints.data(), 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void *>(           0));
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void *>(sizeof(vec4)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    if (glGetError() != GL_NO_ERROR) {
        std::cerr << "OpenGL error" << std::endl;
        return false;
    }

    return true;
}

void SoftMesh::draw() const {
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, u32(m_indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}



SubModel::SubModel(std::string && name, unq<Mesh> && mesh) :
    SubModel(move(name), move(mesh), mat4())
{}

SubModel::SubModel(std::string && name, unq<Mesh> && mesh, const mat4 & originMat) :
    m_name(move(name)),
    m_mesh(move(mesh)),
    m_modelMat(),
    m_normalMat(),
    m_isOrigin(originMat != mat4()),
    m_originModelMat(m_isOrigin ? originMat : mat4()),
    m_invOriginModelMat(m_isOrigin ? glm::inverse(m_originModelMat) : mat4()),
    m_originNormalMat(m_isOrigin ? glm::transpose(glm::mat3(m_invOriginModelMat)) : mat3()),
    m_invOriginNormalMat(m_isOrigin ? glm::transpose(glm::mat3(m_originModelMat)) : mat3())
{}

void SubModel::localTransform(const mat4 & modelMat, const mat3 & normalMat) {
    if (m_isOrigin) {
        m_modelMat = m_originModelMat * modelMat * m_invOriginModelMat;
        m_normalMat = m_originNormalMat * normalMat * m_invOriginNormalMat;
    }
    else {
        m_modelMat = modelMat;
        m_normalMat = normalMat;
    }
}



static unq<Model> loadOBJ(const std::string & filename) {
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string errStr;
    if (!tinyobj::LoadObj(shapes, materials, errStr, filename.c_str())) {
        std::cerr << "tinyobj error: " << errStr << std::endl;
        return nullptr;
    }

    std::vector<SubModel> subModels;
    for (tinyobj::shape_t & shape : shapes) {
        size_t vertexCount(shape.mesh.positions.size() / 3);
        std::vector<HardMesh::Vertex> vertices(vertexCount);
        const vec3 * positions(reinterpret_cast<const vec3 *>(shape.mesh.positions.data()));
        const vec3 *   normals(reinterpret_cast<const vec3 *>(shape.mesh.  normals.data()));
        for (int i(0); i < vertexCount; ++i) {
            vertices[i].position = positions[i];
            vertices[i].  normal =   normals[i];
        }
        unq<Mesh> mesh(new HardMesh(move(vertices), move(shape.mesh.indices)));
        subModels.emplace_back(move(shape.name), move(mesh));
    }

    return unq<Model>(new Model(move(subModels)));
}

static unq<Model> loadGRL(const std::string & filename) {
    std::vector<grl::Object> objects(grl::load(filename));
    if (objects.empty()) {
        std::cerr << "Failed to load GRL" << std::endl;
        return nullptr;
    }

    std::vector<SubModel> subModels;
    for (grl::Object & object : objects) {
        unq<Mesh> mesh(new HardMesh(move(object.vertices)));
        subModels.emplace_back(move(object.name), move(mesh), object.originMat);
    }

    return unq<Model>(new Model(move(subModels)));
}

unq<Model> Model::load(const std::string & filename) {
    std::string ext(util::getExtension(filename));

    unq<Model> model;
    if (ext == "obj") {
        model = loadOBJ(filename);
    }
    else if (ext == "grl") {
        model = loadGRL(filename);
    }
    else {
        std::cerr << "Unrecognized file extension: " << ext << std::endl;
        return {};
    }

    if (!model) {
        return {};
    }

    for (SubModel & subModel : model->m_subModels) {
        if (!subModel.m_mesh->load()) {
            std::cerr << "Failed to load sub model: " << subModel.m_name << std::endl;
            return {};
        }
    }

    return move(model);
}

Model::Model(std::vector<SubModel> && subModels) :
    m_subModels(move(subModels)),
    m_nameMap()
{
    detNameMap();
}

Model::Model(SubModel && subModel) :
    Model(util::singleToVector(move(subModel)))
{}

Model::Model(Model && other) :
    m_subModels(move(other.m_subModels)),
    m_nameMap(move(other.m_nameMap))
{}

void Model::draw() const {
    for (const SubModel & subModel : m_subModels) {
        subModel.m_mesh->draw();
    }
}

void Model::draw(const mat4 & modelMat, const mat3 & normalMat, u32 modelMatUniformBinding, u32 normalMatUniformBinding) const {
    for (const SubModel & subModel : m_subModels) {
        glm::mat4 combModelMat(modelMat * subModel.m_modelMat);
        glm::mat3 combNormalMat(normalMat * subModel.m_normalMat);
        glUniformMatrix4fv(modelMatUniformBinding, 1, GL_FALSE, reinterpret_cast<const float *>(&combModelMat));
        glUniformMatrix3fv(normalMatUniformBinding, 1, GL_FALSE, reinterpret_cast<const float *>(&combNormalMat));
        subModel.m_mesh->draw();
    }
}


SubModel * Model::subModel(const std::string & name) {
    auto it(m_nameMap.find(name));
    return it == m_nameMap.end() ? nullptr : &m_subModels[it->second];
}

const SubModel * Model::subModel(const std::string & name) const {
    return const_cast<Model *>(this)->subModel(name);
}

void Model::detNameMap() {
    m_nameMap.clear();
    for (size_t i(0); i < m_subModels.size(); ++i) {
        m_nameMap[m_subModels[i].m_name] = i;
    }
}