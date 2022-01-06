#version 430 core

#ifdef USER_TEST
#endif

layout (location = 0) out vec4 FragColor;

in vec3 v_Normal;
in vec2 v_TexCoord;
in vec4 v_Vertex;
in vec4 v_LightSpacePos;

layout (location = 14) uniform int u_Technique; //u_Technique = 0 -- primitive shadow; 1 -- PCF; 
layout (location = 15) uniform float u_PCFsize;

uniform int   u_UserVariableInt;
uniform float u_UserVariableFloat;
uniform vec4  u_LightPosition;

layout (binding = 0) uniform sampler2D       u_SceneTexture;
layout (binding = 1) uniform sampler2D       u_DepthMapTexture;
layout (binding = 2) uniform sampler2D       u_ZBufferTexture;
layout (binding = 3) uniform sampler2DShadow u_ZBufferShadowTexture;

void main() {
    // Compute fragment diffuse color
    vec3 N      = normalize(v_Normal);
    vec3 L      = normalize(u_LightPosition.xyz - v_Vertex.xyz);
    float NdotL = max(dot(N, L), 0.0);
    vec4 color  = texture(u_SceneTexture, v_TexCoord) * NdotL;

    vec4 shadow = vec4(0.0);

    // primitive shadow
    if (u_Technique == 0) {
        shadow = textureProj(u_ZBufferShadowTexture, v_LightSpacePos).rrrr;
    }
    // PCF shadow
    else if (u_Technique == 1) {
        int filter_size = int(u_PCFsize * 10.0);
        for (int x = - filter_size; x <= filter_size; x++) {
            for (int y = - filter_size; y <= filter_size; y++) {
                shadow += textureProjOffset(u_ZBufferShadowTexture, v_LightSpacePos, ivec2(x, y)).rrrr;
            }
        }
        shadow /= (2*filter_size + 1) * (2*filter_size + 1);
    }
    
    // Modulate fragment's color according to result of shadow test
    FragColor = color * max(vec4(0.2), shadow);
}
