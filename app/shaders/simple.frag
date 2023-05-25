#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    // render depth buffer
//    outColor = vec4(0,0,gl_FragCoord.w, 1.0);

    outColor = vec4(fragColor, 1.0);
}
