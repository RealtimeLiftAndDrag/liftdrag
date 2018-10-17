#version 450 core

// Types -----------------------------------------------------------------------

// Inputs ----------------------------------------------------------------------

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;
layout (location = 2) in vec2 in_texCoord;
layout (location = 3) in vec3 in_lightPos;

// Outputs ---------------------------------------------------------------------

layout (location = 0) out vec4 out_color;

// Functions -------------------------------------------------------------------

void main() {
    // Variables
    vec3 lightDir = normalize(in_lightPos - in_pos);
    vec3 viewDir = normalize(-in_pos);
    vec3 normal = normalize(in_norm);
    
    // Ambient calculation
    vec3 ambientColor = vec3(0.2f, 0.2f, 0.2f);
    
    // Diffuse calculation
    //float diffuseFactor = max(dot(lightDir, normal), 0); // Clamp to prevent color from reaching back side
    float diffuseFactor = (dot(lightDir, normal) + 1) / 2; // Normalize so the color bleeds onto the back side
    vec3 diffuseColor = vec3(0.5, 0.5, 0.5) * diffuseFactor;
    
    // Specular calculation
    float specularStrength = 0.5f;
    vec3 reflectDir = reflect(-lightDir, normal);
    float shininess = 3.0f;
    float specularFactor = pow(max(dot(viewDir, reflectDir), 0), shininess);
    vec3 specularColor = vec3(1) * specularFactor * specularStrength;

    //output color
    out_color = vec4(ambientColor + diffuseColor + specularColor, 1);
    //out_color = vec4(normal, 1) * 0.5 + 0.5;
}
