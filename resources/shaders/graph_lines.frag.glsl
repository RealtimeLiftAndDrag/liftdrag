#version 450 core

in vec2 v2f_pos;

out vec4 out_color;

uniform vec2 u_viewMin;
uniform vec2 u_viewMax;
uniform vec2 u_gridSize;
uniform vec2 u_viewportSize;
uniform bool u_isFocusX;
uniform float u_focusX;

const vec3 k_backColor = vec3(0.0f);
const vec3 k_gridColor = vec3(0.15f);
const vec3 k_axisColor = vec3(0.3f);
const vec3 k_focusColor = vec3(0.5f);

void main() {
    vec2 viewSize = u_viewMax - u_viewMin;
    vec2 graphToPixelSpace = u_viewportSize / viewSize;

    vec2 p = v2f_pos * viewSize + u_viewMin; // point in value space

    out_color.rgb = k_backColor;
    out_color.a = 1.0f;

    vec2 g = p / u_gridSize; // point in grid space
    g = round(g);
    vec2 gridDist = (p - g * u_gridSize) * graphToPixelSpace;
    if (gridDist.x > 0.0f && gridDist.x < 1.0f || gridDist.y > 0.0f && gridDist.y < 1.0f) out_color.rgb = k_gridColor;

    vec2 axisDist = p * graphToPixelSpace;
    if (axisDist.x > 0.0f && axisDist.x < 1.0f || axisDist.y > 0.0f && axisDist.y < 1.0f) out_color.rgb = k_axisColor;

    if (u_isFocusX) {
        float focusDist = (p.x - u_focusX) * graphToPixelSpace.x;
        if (focusDist > 0.0f && focusDist < 1.0f) out_color.rgb = k_focusColor;
    }
}
