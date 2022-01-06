#version 430 core

#ifdef USER_TEST
#endif

uniform int   u_UserVariableInt;
uniform float u_UserVariableFloat;

layout (location = 0) out vec4 FragColor[2];

in vec4 v_LightSpacePos;

void main(void) {
    FragColor[0] = vec4(length(v_LightSpacePos.xyz), dot(v_LightSpacePos.xyz, v_LightSpacePos.xyz), gl_FragCoord.z, 0.0);
    FragColor[1] = vec4(length(v_LightSpacePos.xyz), dot(v_LightSpacePos.xyz, v_LightSpacePos.xyz), gl_FragCoord.z, 0.0);
}
