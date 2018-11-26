#version 450 core

layout (location = 0) in vec3 in_framePos;

layout (location = 0) out vec4 out_color;

uniform vec3 u_frameSize;

float maxComp(vec3 v) {
    return max(max(v.x, v.y), v.z);
}

void main() {
    vec3 pos = in_framePos;
    pos.z *= u_frameSize.z / u_frameSize.x;
    out_color.rgb = vec3(step(maxComp(pos), 0.2f) * 0.5f + 0.25f);
    out_color.a = 1.0f;
}
