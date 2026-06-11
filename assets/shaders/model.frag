// VyroEngine — model fragment shader (Vulkan)
#version 450

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 vColor;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 light_dir = normalize(vec3(-0.4, -1.0, -0.6));
    float d = max(dot(normalize(vNormal), -light_dir), 0.0);
    outColor = vec4(vColor * (0.35 + 0.65 * d), 1.0);
}
