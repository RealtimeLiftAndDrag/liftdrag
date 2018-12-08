#pragma once

#include "Global.hpp"

#include <vector>
#include <memory>
#include <unordered_map>



class Model;

class Mesh {

    public:

    virtual bool load() = 0;

    virtual void draw() const = 0;

    virtual int vertexCount() const = 0;

    virtual int indexCount() const = 0;

};

class HardMesh : public Mesh {

    public:

    struct Vertex {
        vec3 position;
        float _0;
        vec3 normal;
        float _1;
    };

    HardMesh(std::vector<Vertex> && vertices, std::vector<u32> && indices);
    HardMesh(std::vector<Vertex> && vertices);
    HardMesh(HardMesh && other);

    virtual bool load() override;

    virtual void draw() const override;

    virtual int vertexCount() const override { return int(m_vertices.size()); }

    virtual int indexCount() const override { return int(m_indices.size()); }

    const std::vector<Vertex> & vertices() const { return m_vertices; }

    const std::vector<u32> & indices() const { return m_indices; }

    private:

    std::vector<Vertex> m_vertices;
    std::vector<u32> m_indices;
    u32 m_vertexBuffer;
    u32 m_indexBuffer;
    u32 m_vao;

};

class SoftMesh : public Mesh {

    public:

    struct Vertex {
        vec3 position;
        float mass;
        vec3 normal;
        s32 group;
        vec3 prevPosition;
        float _0;
        vec3 force0; // lift
        float _1;
        vec3 force1; // drag
        float _2;
    };

    struct Constraint {
        u32 i;
        u32 j;
        float restDist;
        float _0;
    };

    SoftMesh(std::vector<Vertex> && vertices, std::vector<u32> && indices, std::vector<Constraint> && constraints);
    SoftMesh(SoftMesh && other);

    virtual bool load() override;

    virtual void draw() const override;

    virtual int vertexCount() const override { return int(m_vertices.size()); }

    virtual int indexCount() const override { return int(m_indices.size()); }

    int constraintCount() const { return int(m_constraints.size()); }

    const std::vector<Vertex> & vertices() const { return m_vertices; }
    std::vector<Vertex> & vertices() { return m_vertices; }

    const std::vector<u32> & indices() const { return m_indices; }
    std::vector<u32> & indices() { return m_indices; }

    const std::vector<Constraint> & constraints() const { return m_constraints; }
    std::vector<Constraint> & constraints() { return m_constraints; }

    u32 vertexBuffer() const { return m_vertexBuffer; }

    u32 indexBuffer() const { return m_indexBuffer; }

    u32 constraintBuffer() const { return m_constraintBuffer; }

    private:

    std::vector<Vertex> m_vertices;
    std::vector<u32> m_indices;
    std::vector<Constraint> m_constraints;
    u32 m_vertexBuffer;
    u32 m_indexBuffer;
    u32 m_constraintBuffer;
    u32 m_vao;

};

class SubModel {

    friend Model;

    private:

    std::string m_name;
    unq<Mesh> m_mesh;
    mat4 m_modelMat; // final model matrix including local and origin transforms
    mat3 m_normalMat; // final normal matrix including local and origin transforms
    bool m_isOrigin; // true if `originMat` is not identity matrix
    mat4 m_originModelMat; // from local to world
    mat4 m_invOriginModelMat; // from world to local
    mat3 m_originNormalMat; // from local to world directional
    mat3 m_invOriginNormalMat; // from world to local directional

    public:

    SubModel(std::string && name, unq<Mesh> && mesh);
    SubModel(std::string && name, unq<Mesh> && mesh, const mat4 & originMat);

    void localTransform(const mat4 & modelMat, const mat3 & normalMat);

    const std::string & name() const { return m_name; }

    const Mesh & mesh() const { return *m_mesh; }

    const mat4 & modelMat() const { return m_modelMat; }

    const mat3 & normalMat() const { return m_normalMat; }

};

class Model {

    public:

    static unq<Model> load(const std::string & filename);

    private:

    std::vector<SubModel> m_subModels;
    std::unordered_map<std::string, size_t> m_nameMap;

    public:

    Model(std::vector<SubModel> && subModels);
    Model(SubModel && subModel);
    Model(Model && other);

    void draw(const mat4 & modelMat, const mat3 & normalMat, u32 modelMatUniformBinding, u32 normalMatUniformBinding) const;
    void draw() const;

    size_t subModelCount() const { return m_subModels.size(); }

    const std::vector<SubModel> & subModels() const { return m_subModels; }

    SubModel * subModel(const std::string & name);
    const SubModel * subModel(const std::string & name) const;

    private:

    void detNameMap();

};