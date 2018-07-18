#version 450 core

out vec4 out_color;

uniform vec2 u_min;
uniform vec2 u_max;
uniform vec2 u_viewSize;
uniform vec2 u_gridSize;

const vec3 k_lineColor = vec3(0.25f);
const vec3 k_axisColor = vec3(0.5f);
const vec3 k_backColor = vec3(0.0f);

void main() {
    vec2 graphSize = u_max - u_min;
    vec2 graphToPixelSpace = u_viewSize / graphSize;
    vec2 p = gl_FragCoord.xy / u_viewSize * graphSize + u_min; // point in value space
    vec2 g = p / u_gridSize; // point in grid space
    g = round(g);
    vec2 gridDist = abs(p - g * u_gridSize) * graphToPixelSpace; // distance in pixel space
    vec2 axisDist = abs(p) * graphToPixelSpace; // distance to grid in pixel space
    float isLine = float(gridDist.x < 1.0f || gridDist.y < 1.0f);
    float isAxis = float(axisDist.x < 1.0f || axisDist.y < 1.0f);
    out_color.rgb = mix(k_backColor, mix(k_lineColor, k_axisColor, isAxis), isLine);
    out_color.a = 1.0f;
}