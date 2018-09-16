#pragma once

#include "Global.hpp"

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>



class Model;

class Mesh {

    private:

    std::vector<vec3> m_locations;
    std::vector<vec3> m_normals;
    std::vector<u32> m_indices;
    uint m_vbo;
    uint m_ibo;
    uint m_vao;
    vec3 m_spanMin, m_spanMax;

    public:

    Mesh(std::vector<vec3> && locations, std::vector<vec3> && normals, std::vector<u32> && indices);
    Mesh(std::vector<vec3> && locations, std::vector<vec3> && normals);
    Mesh(Mesh && other);

    bool load();

    void draw() const;

    u32 vertexCount() const { return u32(m_locations.size()); }

    const std::vector<vec3> & locations() const { return m_locations; }

    const std::vector<vec3> & normals() const { return m_normals; }

    u32 indexCount() const { return u32(m_indices.size()); }

    const std::vector<u32> & indices() const { return m_indices; }

    const vec3 & spanMin() const { return m_spanMin; }
    const vec3 & spanMax() const { return m_spanMax; }

    private:

    void detSpan();

};

class SubModel {

    friend Model;

    private:

    std::string m_name;
    Mesh m_mesh;
    mat4 m_modelMat; // final model matrix including local and origin transforms
    mat3 m_normalMat; // final normal matrix including local and origin transforms
    bool m_isOrigin; // true if `originMat` is not identity matrix
    mat4 m_originModelMat; // from local to world
    mat4 m_invOriginModelMat; // from world to local
    mat3 m_originNormalMat; // from local to world directional
    mat3 m_invOriginNormalMat; // from world to local directional

    public:

    SubModel(std::string && name, Mesh && mesh);
    SubModel(std::string && name, Mesh && mesh, const mat4 & originMat);

    void localTransform(const mat4 & modelMat, const mat3 & normalMat);

    const std::string & name() const { return m_name; }

    const Mesh & mesh() const { return m_mesh; }

    const mat4 & modelMat() const { return m_modelMat; }

    const mat3 & normalMat() const { return m_normalMat; }

};

class Model {

    public:

    static unq<Model> load(const std::string & filename);

    private:

    std::vector<SubModel> m_subModels;
    std::unordered_map<std::string, size_t> m_nameMap;
    vec3 m_spanMin, m_spanMax;

    public:

    Model(std::vector<SubModel> && subModels);
    Model(Model && other);

    void draw(const mat4 & modelMat, const mat3 & normalMat, uint modelMatUniformBinding, uint normalMatUniformBinding) const;
    void draw() const;

    size_t subModelCount() const { return m_subModels.size(); }

    const std::vector<SubModel> & subModels() const { return m_subModels; }

    SubModel * subModel(const std::string & name);
    const SubModel * subModel(const std::string & name) const;

    const vec3 & spanMin() const { return m_spanMin; }
    const vec3 & spanMax() const { return m_spanMax; }

    private:

    void detNameMap();

    void detSpan();

};