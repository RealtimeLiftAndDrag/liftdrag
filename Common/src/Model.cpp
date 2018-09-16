#include "Model.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

#include "glad/glad.h"
#include "tinyobjloader/tiny_obj_loader.h"
#include "GRLLoader.hpp"

#include "Util.hpp"



Mesh::Mesh(std::vector<vec3> && locations, std::vector<vec3> && normals, std::vector<u32> && indices) :
    m_locations(std::move(locations)),
    m_normals(std::move(normals)),
    m_indices(std::move(indices)),
    m_vbo(0),
    m_ibo(0),
    m_vao(0),
    m_spanMin(),
    m_spanMax()
{
    detSpan();
}

Mesh::Mesh(std::vector<vec3> && locations, std::vector<vec3> && normals) :
    Mesh(std::move(locations), std::move(normals), std::vector<u32>())
{}

Mesh::Mesh(Mesh && other) :
    m_locations(std::move(other.m_locations)),
    m_normals(std::move(other.m_normals)),
    m_indices(std::move(other.m_indices)),
    m_vbo(other.m_vbo),
    m_ibo(other.m_ibo),
    m_vao(other.m_vao),
    m_spanMin(other.m_spanMin),
    m_spanMax(other.m_spanMax)
{
    other.m_vbo = 0;
    other.m_ibo = 0;
    other.m_vao = 0;
}

bool Mesh::load() {
    size_t vertexCount(m_locations.size());
    size_t locationsSize(vertexCount * sizeof(vec3));
    size_t normalsSize(vertexCount * sizeof(vec3));
    size_t verticesSize(locationsSize + normalsSize);
    size_t locationsOffset(0);
    size_t normalsOffset(locationsSize);

    // VBO
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, verticesSize, nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, locationsOffset, locationsSize, m_locations.data());
    glBufferSubData(GL_ARRAY_BUFFER, normalsOffset, normalsSize, m_normals.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // IBO
    if (m_indices.size()) {
        glGenBuffers(1, &m_ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(u32), m_indices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    // VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, reinterpret_cast<const void *>(locationsOffset));
    glVertexAttribPointer(1, 3, GL_FLOAT, false, 0, reinterpret_cast<const void *>(normalsOffset));
    if (m_indices.size()) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    if (glGetError()) {
        std::cerr << "OpenGL error" << std::endl;
        return false;
    }

    return true;
}

void Mesh::draw() const {
    glBindVertexArray(m_vao);
    if (m_indices.size()) {
        glDrawElements(GL_TRIANGLES, u32(m_indices.size()), GL_UNSIGNED_INT, reinterpret_cast<const void *>(0));
    }
    else {
        glDrawArrays(GL_TRIANGLES, 0, u32(m_locations.size()));
    }
    glBindVertexArray(0);
}

void Mesh::detSpan() {
    m_spanMin = vec3( std::numeric_limits<float>::infinity());
    m_spanMax = vec3(-std::numeric_limits<float>::infinity());
    for (const vec3 & location : m_locations) {
        m_spanMin = glm::min(m_spanMin, location);
        m_spanMax = glm::max(m_spanMax, location);
    }
}



SubModel::SubModel(std::string && name, Mesh && mesh) :
    SubModel(std::move(name), std::move(mesh), mat4())
{}

SubModel::SubModel(std::string && name, Mesh && mesh, const mat4 & originMat) :
    m_name(std::move(name)),
    m_mesh(std::move(mesh)),
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

        vec3 * start(reinterpret_cast<vec3 *>(shape.mesh.positions.data()));
        std::vector<vec3> locations(start, start + vertexCount);

        start = reinterpret_cast<vec3 *>(shape.mesh.normals.data());
        std::vector<vec3> normals(start, start + vertexCount);

        Mesh mesh(std::move(locations), std::move(normals), std::move(shape.mesh.indices));
        subModels.emplace_back(std::move(shape.name), std::move(mesh));
    }

    return unq<Model>(new Model(std::move(subModels)));
}

static unq<Model> loadGRL(const std::string & filename) {
    std::vector<grl::Object> objects(grl::load(filename));
    if (objects.empty()) {
        std::cerr << "Failed to load GRL" << std::endl;
        return nullptr;
    }    

    std::vector<SubModel> subModels;
    for (grl::Object & object : objects) {
        Mesh mesh(std::move(object.posData), std::move(object.norData));
        subModels.emplace_back(std::move(object.name), std::move(mesh), object.originMat);
    }

    return unq<Model>(new Model(std::move(subModels)));
}

unq<Model> Model::load(const std::string & filename) {
    std::string ext(Util::getExtension(filename));

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
        if (!subModel.m_mesh.load()) {
            std::cerr << "Failed to load sub model: " << subModel.m_name << std::endl;
            return {};
        }
    }

    return std::move(model);
}

Model::Model(std::vector<SubModel> && subModels) :
    m_subModels(std::move(subModels)),
    m_nameMap(),
    m_spanMin(),
    m_spanMax()
{
    detNameMap();
    detSpan();
}

Model::Model(Model && other) :
    m_subModels(std::move(other.m_subModels)),
    m_nameMap(std::move(other.m_nameMap))
{
    m_spanMin = other.m_spanMin;
    m_spanMax = other.m_spanMax;
}

void Model::draw() const {
    for (const SubModel & subModel : m_subModels) {
        subModel.m_mesh.draw();
    }
}

void Model::draw(const mat4 & modelMat, const mat3 & normalMat, uint modelMatUniformBinding, uint normalMatUniformBinding) const {
    for (const SubModel & subModel : m_subModels) {
        glm::mat4 combModelMat(modelMat * subModel.m_modelMat);
        glm::mat3 combNormalMat(normalMat * subModel.m_normalMat);
        glUniformMatrix4fv(modelMatUniformBinding, 1, GL_FALSE, reinterpret_cast<const float *>(&combModelMat));
        glUniformMatrix3fv(normalMatUniformBinding, 1, GL_FALSE, reinterpret_cast<const float *>(&combNormalMat));
        subModel.m_mesh.draw();
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

void Model::detSpan() {
    m_spanMin = vec3( std::numeric_limits<float>::infinity());
    m_spanMax = vec3(-std::numeric_limits<float>::infinity());
    for (const SubModel & subModel : m_subModels) {
        m_spanMin = glm::min(m_spanMin, subModel.m_mesh.spanMin());
        m_spanMax = glm::max(m_spanMax, subModel.m_mesh.spanMax());
    }    
}