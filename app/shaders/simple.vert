#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in vec3 normal;

layout(location = 0) out vec3 fragColor;

layout( push_constant ) uniform constants {
    mat4 Model;
    mat4 View;
    mat4 Projection;
} PushConstants;

void main() {
    gl_Position = PushConstants.Model * vec4(position, 1.0);
    fragColor = color;
}
