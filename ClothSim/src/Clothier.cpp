#include "Clothier.hpp"

#include <list>



namespace Clothier {

    using Vertex = SoftMesh::Vertex;
    using Constraint = SoftMesh::Constraint;



    // Reorganizes constraints from a dense array to an array with chunks sized
    // 'groupSize', where each chunk contains as many constraints as possible
    // such that no vertex is used more than once. This is to avoid race
    // conditions when processing constraints in parallel in the shader
    static void decouple(const std::vector<Vertex> & vertices, std::vector<Constraint> & constraints, int groupSize) {
        std::list<Constraint> cList(constraints.cbegin(), constraints.cend());
        constraints.clear();
        unq<bool[]> vertHeld(new bool[vertices.size()]);

        while (!cList.empty()) {
            std::fill_n(vertHeld.get(), vertices.size(), false);

            int n(0);
            auto it(cList.begin());
            while (n < groupSize && it != cList.end()) {
                const Constraint & c(*it);
                if (!vertHeld[c.i] && !vertHeld[c.j]) {
                    constraints.push_back(c);
                    vertHeld[c.i] = true;
                    vertHeld[c.j] = true;
                    it = cList.erase(it);
                    ++n;
                }
                else {
                    ++it;
                }
            }
            constraints.insert(constraints.end(), groupSize - n, {});
        }
    }



    constexpr float k_h(0.866025404f); // height of an equilateral triangle of side length 1

    vec2 triToCart(vec2 p) {
        return vec2(p.y - p.x * 0.5f, p.x * k_h);
    }

    u32 triI(ivec2 p) {
        return (p.y + 1) * p.y / 2 + p.x;
    }

    unq<SoftMesh> createRectangle(
        ivec2 lod,
        float weaveSize,
        float totalMass,
        int groupSize,
        const mat4 & transform,
        const bvec4 & pinVerts,
        const bvec4 & pinEdges
    ) {
        ivec2 vertSize(lod + 1);
        int vertCount(vertSize.x * vertSize.y);
        std::vector<Vertex> vertices;
        vertices.reserve(vertCount);
        vec2 clothSize(vec2(lod) * weaveSize);
        float vertMass(totalMass / vertCount);
        for (ivec2 p(0); p.y < vertSize.y; ++p.y) {
            for (p.x = 0; p.x < vertSize.x; ++p.x) {
                Vertex vertex;
                vertex.position = vec3(transform * vec4(p.x * weaveSize, p.y * weaveSize, 0.0f, 1.0f));
                vertex.mass = vertMass;
                vertex.normal = vec3(0.0f, 0.0f, 1.0f);
                vertex.group = 0;
                vertex.prevPosition = vertex.position;
                vertex.force0 = vec3(0.0f);
                vertex.force1 = vec3(0.0f);
                vertices.push_back(vertex);
            }
        }
        if (pinVerts.x) {
            vertices[0].mass = 0.0f;
        }
        if (pinVerts.y) {
            vertices[lod.x].mass = 0.0f;
        }
        if (pinVerts.z) {
            vertices[vertSize.x * lod.y + lod.x].mass = 0.0f;
        }
        if (pinVerts.w) {
            vertices[vertSize.x * lod.y].mass = 0.0f;
        }
        if (pinEdges.x) {
            for (int i(0); i < vertSize.x; ++i) {
                vertices[i].mass = 0.0f;
            }
        }
        if (pinEdges.y) {
            for (int i(0); i < vertSize.y; ++i) {
                vertices[vertSize.x * i + lod.x].mass = 0.0f;
            }
        }
        if (pinEdges.z) {
            for (int i(0); i < vertSize.x; ++i) {
                vertices[vertSize.x * lod.y + i].mass = 0.0f;
            }
        }
        if (pinEdges.w) {
            for (int i(0); i < vertSize.y; ++i) {
                vertices[vertSize.x * i].mass = 0.0f;
            }
        }

        int indexCount = lod.x * lod.y * 2 * 3;
        std::vector<u32> indices;
        for (ivec2 p(0); p.y < lod.y; ++p.y) {
            for (p.x = 0; p.x < lod.x; ++p.x) {
                u32 pi(p.y * vertSize.x + p.x);
                u32 pj(pi + vertSize.x + 1);
                indices.push_back(pi);
                indices.push_back(pi + 1);
                indices.push_back(pj);
                indices.push_back(pj);
                indices.push_back(pj - 1);
                indices.push_back(pi);
            }
        }

        int c1Count(vertSize.x * (vertSize.y - 1) + vertSize.y * (vertSize.x - 1));
        int c2Count((vertSize.x - 1) * (vertSize.y - 1) * 2);
        int c3Count(vertSize.x * (vertSize.y - 2) + vertSize.y * (vertSize.x - 2));
        int c4Count((vertSize.x - 2) * (vertSize.y - 2) * 2);
        int constraintCount = c1Count + c2Count + c3Count + c4Count;
        float d1(weaveSize);
        float d2(d1 * std::sqrt(2.0f));
        float d3(d1 * 2.0f);
        float d4(d2 * 2.0f);
        std::vector<Constraint> constraints;
        constraints.reserve(constraintCount);
        // C1 - small "+"
        for (ivec2 p(0); p.y < vertSize.y; ++p.y) {
            for (p.x = 0; p.x < vertSize.x - 1; ++p.x) {
                u32 pi(p.y * vertSize.x + p.x);
                constraints.push_back({ pi, pi + 1, d1 });
            }
        }
        for (ivec2 p(0); p.y < vertSize.y - 1; ++p.y) {
            for (p.x = 0; p.x < vertSize.x; ++p.x) {
                u32 pi(p.y * vertSize.x + p.x);
                constraints.push_back({ pi, pi + vertSize.x, d1 });
            }
        }
        // C2 - small "x"
        for (ivec2 p(0); p.y < vertSize.y - 1; ++p.y) {
            for (p.x = 0; p.x < vertSize.x - 1; ++p.x) {
                u32 pi(p.y * vertSize.x + p.x);
                constraints.push_back({ pi, pi + vertSize.x + 1, d2 });
                constraints.push_back({ pi + 1, pi + vertSize.x, d2 });
            }
        }
        // C3 - large "+"
        for (ivec2 p(0); p.y < vertSize.y; ++p.y) {
            for (p.x = 0; p.x < vertSize.x - 2; ++p.x) {
                u32 pi(p.y * vertSize.x + p.x);
                constraints.push_back({ pi, pi + 2, d3 });
            }
        }
        for (ivec2 p(0); p.y < vertSize.y - 2; ++p.y) {
            for (p.x = 0; p.x < vertSize.x; ++p.x) {
                u32 pi(p.y * vertSize.x + p.x);
                constraints.push_back({ pi, pi + vertSize.x * 2, d3 });
            }
        }
        // C4 - large "x"
        for (ivec2 p(0); p.y < vertSize.y - 2; ++p.y) {
            for (p.x = 0; p.x < vertSize.x - 2; ++p.x) {
                u32 pi(p.y * vertSize.x + p.x);
                constraints.push_back({ pi, pi + vertSize.x * 2 + 2, d4 });
                constraints.push_back({ pi + 2, pi + vertSize.x * 2, d4 });
            }
        }

        if (groupSize) decouple(vertices, constraints, groupSize);

        return unq<SoftMesh>(new SoftMesh(move(vertices), move(indices), move(constraints)));
    }

    unq<SoftMesh> createTriangle(
        const vec3 & a,
        const vec3 & b,
        const vec3 & c,
        int lod,
        float totalMass,
        int groupSize,
        const bvec3 & pinVerts,
        const bvec3 & pinEdges
    ) {
        int edgeVerts(lod + 1);
        int vertCount((edgeVerts + 1) * edgeVerts / 2);
        std::vector<Vertex> vertices;
        vertices.reserve(vertCount);
        float vertMass(totalMass / vertCount);
        float invLOD(1.0f / float(lod));
        vec3 normal(glm::normalize(glm::cross(b - a, c - a)));
        for (ivec2 p(0); p.y < edgeVerts; ++p.y) {
            for (p.x = 0; p.x <= p.y; ++p.x) {
                float alpha(1.0f - p.y * invLOD);
                float beta(1.0f - (lod - p.y + p.x) * invLOD);
                float gamma(1.0f - alpha - beta);
                Vertex vertex;
                vertex.position = alpha * a + beta * b + gamma * c;
                vertex.mass = vertMass;
                vertex.normal = normal;
                vertex.group = 0;
                vertex.force0 = vec3(0.0f);
                vertex.force1 = vec3(0.0f);
                vertex.prevPosition = vertex.position;
                vertices.push_back(vertex);
            }
        }
        if (pinVerts.x) {
            vertices[triI(ivec2(0, 0))].mass = 0.0f;
        }
        if (pinVerts.y) {
            vertices[triI(ivec2(0, lod))].mass = 0.0f;
        }
        if (pinVerts.z) {
            vertices[triI(ivec2(lod, lod))].mass = 0.0f;
        }
        if (pinEdges.x) {
            for (int i(0); i < edgeVerts; ++i) {
                vertices[triI(ivec2(0, i))].mass = 0.0f;
            }
        }
        if (pinEdges.y) {
            for (int i(0); i < edgeVerts; ++i) {
                vertices[triI(ivec2(i, lod))].mass = 0.0f;
            }
        }
        if (pinEdges.z) {
            for (int i(0); i < edgeVerts; ++i) {
                vertices[triI(ivec2(i, i))].mass = 0.0f;
            }
        }

        int indexCount = lod * lod * 3;
        std::vector<u32> indices;
        indices.reserve(indexCount);
        for (ivec2 p(0); p.y < lod; ++p.y) {
            for (p.x = 0; p.x <= p.y; ++p.x) {
                u32 i(triI(p));
                u32 j(i + p.y + 1);
                indices.push_back(i);
                indices.push_back(j);
                indices.push_back(j + 1);
                if (p.x < p.y) {
                    indices.push_back(j + 1);
                    indices.push_back(i + 1);
                    indices.push_back(i);
                }
            }
        }

        float minDistA(glm::distance(b, c) * invLOD);
        float minDistB(glm::distance(c, a) * invLOD);
        float minDistC(glm::distance(a, b) * invLOD);
        float majDistA(glm::length((b - a) + (c - a)) * invLOD);
        float majDistB(glm::length((c - b) + (a - b)) * invLOD);
        float majDistC(glm::length((a - c) + (b - c)) * invLOD);
        // Minor flat out from A corner
        std::vector<Constraint> constraints;
        for (ivec2 p(0, 1); p.y <= lod; ++p.y) {
            for (p.x = 0; p.x < p.y; ++p.x) {
                ivec2 q(p);
                constraints.push_back({ triI(q), triI(q + ivec2(1, 0)), minDistA });
            }
        }
        // Minor flat out from B corner
        for (ivec2 p(0, 1); p.y <= lod; ++p.y) {
            for (p.x = 0; p.x < p.y; ++p.x) {
                ivec2 q(p.y - p.x, lod - p.x);
                constraints.push_back({ triI(q), triI(q + ivec2(-1, -1)), minDistB });
            }
        }
        // Minor flat out from C corner
        for (ivec2 p(0, 1); p.y <= lod; ++p.y) {
            for (p.x = 0; p.x < p.y; ++p.x) {
                ivec2 q(lod - p.y, lod + p.x - p.y);
                constraints.push_back({ triI(q), triI(q + ivec2(0, 1)), minDistC });
            }
        }
        // Major straight out from A corner
        for (ivec2 p(0, 0); p.y < lod - 1; ++p.y) {
            for (p.x = 0; p.x <= p.y; ++p.x) {
                ivec2 q(p);
                constraints.push_back({ triI(q), triI(q + ivec2(1, 2)), majDistA });
            }
        }
        //  Major straight out from B corner
        for (ivec2 p(0, 0); p.y < lod - 1; ++p.y) {
            for (p.x = 0; p.x <= p.y; ++p.x) {
                ivec2 q(p.y - p.x, lod - p.x);
                constraints.push_back({ triI(q), triI(q + ivec2(1, -1)), majDistB });
            }
        }
        //  Major straight out from C corner
        for (ivec2 p(0, 0); p.y < lod - 1; ++p.y) {
            for (p.x = 0; p.x <= p.y; ++p.x) {
                ivec2 q(lod - p.y, lod + p.x - p.y);
                constraints.push_back({ triI(q), triI(q + ivec2(-2, -1)), majDistC });
            }
        }

        if (groupSize) decouple(vertices, constraints, groupSize);

        return unq<SoftMesh>(new SoftMesh(move(vertices), move(indices), move(constraints)));
    }

}
