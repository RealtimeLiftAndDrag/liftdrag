#include "ClothMesher.hpp"

#include <list>



static void decouple(const std::vector<SoftVertex> & vertices, std::vector<Constraint> & constraints, int groupSize) {
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



unq<SoftModel> createClothRectangle(ivec2 lod, float weaveSize, int groupSize) {
    ivec2 vertSize(lod + 1);
    int vertCount(vertSize.x * vertSize.y);
    std::vector<SoftVertex> vertices;
    vertices.reserve(vertCount);
    vec2 clothSize(vec2(lod) * weaveSize);
    vec2 corner(clothSize * -0.5f);
    for (ivec2 p(0); p.y < vertSize.y; ++p.y) {
        for (p.x = 0; p.x < vertSize.x; ++p.x) {
            SoftVertex vertex;
            vertex.position = vec3(corner + vec2(p) * weaveSize, 0.0f);
            vertex.mass = 1.0f;
            vertex.normal = vec3(0.0f, 0.0f, 1.0f);
            vertex.force = vec3(0.0f);
            vertex.prevPosition = vertex.position;
            vertices.push_back(vertex);
        }
    }
    //vertices[lod.y * vertSize.x].mass = 0.0f;
    vertices[lod.y * vertSize.x + lod.x].mass = 0.0f;
    //for (int i(0); i < vertSize.x; ++i) {
    //    vertices[lod.y * vertSize.x + i].mass = 0.0f;
    //}
    //for (int i(0); i < vertSize.y; ++i) {
    //    vertices[i * vertSize.x].mass = 0.0f;
    //}

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

    return unq<SoftModel>(new SoftModel(SoftMesh(move(vertices), move(indices), move(constraints))));
}

constexpr float k_h(0.866025404f); // height of an equilateral triangle

vec2 triToCart(vec2 p) {
    return vec2(p.y - p.x * 0.5f, p.x * k_h);
}

u32 triI(ivec2 p) {
    return (p.y + 1) * p.y / 2 + p.x;
}

unq<SoftModel> createClothTriangle(int lod, float weaveSize, int groupSize) {
    int edgeVerts(lod + 1);
    int vertCount((edgeVerts + 1) * edgeVerts / 2);
    float edgeLength(lod * weaveSize);
    std::vector<SoftVertex> vertices;
    vertices.reserve(vertCount);
    vec2 origin(edgeLength * -0.5f, edgeLength * k_h * -0.5f);
    for (ivec2 p(0); p.y < edgeVerts; ++p.y) {
        for (p.x = 0; p.x <= p.y; ++p.x) {
            SoftVertex vertex;
            vertex.position = vec3(origin + triToCart(vec2(p)) * weaveSize, 0.0f);
            vertex.position = -vertex.position;
            //vertex.position = vec3(vertex.position.y + edgeLength * k_h * 0.5f, -vertex.position.x, vertex.position.z);
            vertex.mass = 1.0f;
            vertex.normal = vec3(0.0f, 0.0f, 1.0f);
            vertex.force = vec3(0.0f);
            vertex.prevPosition = vertex.position;
            vertices.push_back(vertex);
        }
    }
    //for (int i(0); i < edgeVerts; ++i) {
    //    vertices[triI(ivec2(0, i))].mass = 0.0f;
    //}
    vertices[triI(ivec2(0, 0))].mass = 0.0f;
    vertices[triI(ivec2(0, lod))].mass = 0.0f;

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

    float d1(weaveSize);
    float d2(weaveSize * k_h * 2.0f);
    std::vector<Constraint> constraints;
    for (ivec2 p(0, 1); p.y <= lod; ++p.y) {
        for (p.x = 0; p.x < p.y; ++p.x) {
            ivec2 q(p);
            constraints.push_back({ triI(q), triI(q + ivec2(1, 0)), d1 });
        }
    }
    // Out from B corner
    for (ivec2 p(0, 1); p.y <= lod; ++p.y) {
        for (p.x = 0; p.x < p.y; ++p.x) {
            ivec2 q(p.y - p.x, lod - p.x);
            constraints.push_back({ triI(q), triI(q + ivec2(-1, -1)), d1 });
        }
    }
    // Out from C corner
    for (ivec2 p(0, 1); p.y <= lod; ++p.y) {
        for (p.x = 0; p.x < p.y; ++p.x) {
            ivec2 q(lod - p.y, lod + p.x - p.y);
            constraints.push_back({ triI(q), triI(q + ivec2(0, 1)), d1 });
        }
    }
    // Out from A corner
    for (ivec2 p(0, 0); p.y < lod - 1; ++p.y) {
        for (p.x = 0; p.x <= p.y; ++p.x) {
            ivec2 q(p);
            constraints.push_back({ triI(q), triI(q + ivec2(1, 2)), d2 });
        }
    }
    // Out from B corner
    for (ivec2 p(0, 0); p.y < lod - 1; ++p.y) {
        for (p.x = 0; p.x <= p.y; ++p.x) {
            ivec2 q(p.y - p.x, lod - p.x);
            constraints.push_back({ triI(q), triI(q + ivec2(1, -1)), d2 });
        }
    }
    // Out from C corner
    for (ivec2 p(0, 0); p.y < lod - 1; ++p.y) {
        for (p.x = 0; p.x <= p.y; ++p.x) {
            ivec2 q(lod - p.y, lod + p.x - p.y);
            constraints.push_back({ triI(q), triI(q + ivec2(-2, -1)), d2 });
        }
    }

    if (groupSize) decouple(vertices, constraints, groupSize);

    return unq<SoftModel>(new SoftModel(SoftMesh(move(vertices), move(indices), move(constraints))));

}
