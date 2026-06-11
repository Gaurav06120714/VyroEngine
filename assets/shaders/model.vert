// VyroEngine — model vertex shader (Vulkan)
#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aColor;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    mat4 model;
} pc;

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec3 vColor;

void main()
{
    gl_Position = pc.mvp * vec4(aPos, 1.0);
    vNormal = mat3(pc.model) * aNormal;
    vColor = aColor;
}
