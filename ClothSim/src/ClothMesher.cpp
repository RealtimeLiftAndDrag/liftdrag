#include "ClothMesher.hpp"



unq<SoftModel> createClothRectangle(ivec2 lod, float weaveSize) {
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
            vertex.constraintFactor = 0.0f;
            vertex.force = vec3(0.0f);
            vertex.prevPosition = vertex.position;
            vertices.push_back(vertex);
        }
    }
    for (int i(0); i < vertSize.x; ++i) {
        vertices[lod.y * vertSize.x + i].mass = 0.0f;
    }
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
    for (const Constraint & c : constraints) {
        ++vertices[c.i].constraintFactor;
        ++vertices[c.j].constraintFactor;
    }
    for (SoftVertex & p : vertices) {
        p.constraintFactor = p.mass > 0.0f ? 1.0f / p.constraintFactor : 0.0f;
    }

    return unq<SoftModel>(new SoftModel(SoftMesh(move(vertices), move(indices), move(constraints))));
}

vec2 triToCart(vec2 p) {
    constexpr float h(0.866025404f);
    return vec2(p.y * 0.5f + p.x, p.y * h);
}

int triToI(ivec2 p, int edgeVerts) {
    // TODO: optimize
    int topEdgeVerts(edgeVerts - p.y);
    int topVertCount((topEdgeVerts + 1) * topEdgeVerts / 2);
    int vertCount((edgeVerts + 1) * edgeVerts / 2);
    int startI(vertCount - topVertCount);
    return startI + p.x;
}

unq<SoftModel> createClothTriangle(int lod, float weaveSize) {
    int edgeVerts(lod + 1);
    int vertCount((edgeVerts + 1) * edgeVerts / 2);
    float edgeLength(lod * weaveSize);
    std::vector<SoftVertex> vertices;
    vertices.reserve(vertCount);
    for (ivec2 p(0); p.y < edgeVerts; ++p.y) {
        int rowVerts(edgeVerts - p.y);
        for (p.x = 0; p.x < rowVerts; ++p.x) {
            SoftVertex vertex;
            vertex.position = vec3(triToCart(vec2(p)) * weaveSize, 0.0f);
            vertex.position.x -= edgeLength * 0.5f;
            vertex.position.x = -vertex.position.x;
            vertex.position.y = -vertex.position.y;
            vertex.position.y += edgeLength * 0.866025404f * 0.5f;
            vertex.mass = 1.0f;
            vertex.normal = vec3(0.0f, 0.0f, 1.0f);
            vertex.constraintFactor = 0.0f;
            vertex.force = vec3(0.0f);
            vertex.prevPosition = vertex.position;
            vertices.push_back(vertex);
        }
    }
    for (int i(0); i < edgeVerts; ++i) {
        vertices[i].mass = 0.0f;
    }

    int indexCount = lod * lod * 3;
    std::vector<u32> indices;
    indices.reserve(indexCount);
    int i(0);
    for (ivec2 p(0); p.y < lod; ++p.y) {
        int rowVerts(edgeVerts - p.y);
        for (p.x = 0; p.x < rowVerts - 1; ++p.x) {
            indices.push_back(i);
            indices.push_back(i + 1);
            indices.push_back(i + rowVerts);

            if (p.x < rowVerts - 2) {
                indices.push_back(i + rowVerts + 1);
                indices.push_back(i + rowVerts);
                indices.push_back(i + 1);
            }

            ++i;
        }
        ++i;
    }

    /*int c1Count(vertSize.x * (vertSize.y - 1) + vertSize.y * (vertSize.x - 1));
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
    for (const Constraint & c : constraints) {
        ++vertices[c.i].constraintFactor;
        ++vertices[c.j].constraintFactor;
    }
    for (SoftVertex & p : vertices) {
        p.constraintFactor = p.mass > 0.0f ? 1.0f / p.constraintFactor : 0.0f;
    }*/

    std::vector<Constraint> constraints{{ 0, 0, 0.0f }};
    return unq<SoftModel>(new SoftModel(SoftMesh(move(vertices), move(indices), move(constraints))));

}
