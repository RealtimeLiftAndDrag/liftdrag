#version 450 core

layout (location = 0) out vec4 out_color;

uniform vec3 u_color;

void main() {
    out_color.rgb = u_color;
    out_color.a = 1.0f;
    /*
    vec2 viewSize = u_viewMax - u_viewMin;
    vec2 graphToPixelSpace = u_viewportSize / viewSize;

    vec2 p = in_pos * viewSize + u_viewMin; // point in value space

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
    */
}