#version 430 core

#ifdef USER_TEST
#endif

in vec4 a_Vertex;
in vec3 a_Normal;
in vec2 a_TexCoord;

out vec3 v_Normal;
out vec2 v_TexCoord;
out vec4 v_Vertex;
out vec4 v_LightSpacePos;

layout (location = 0) uniform mat4  u_ProjectionMatrix;
layout (location = 1) uniform mat4  u_ModelViewMatrix;
layout (location = 2) uniform mat4  u_LightViewMatrix;		// Use these two matrixes to calculate vertex position in ...
layout (location = 3) uniform mat4  u_LightProjectionMatrix;  // ...light view space, or
layout (location = 4) uniform mat4  u_ShadowTransformMatrix;	// calculate transformation in app and pass it in this variable into shader
uniform int   u_UserVariableInt;
uniform float u_UserVariableFloat;

layout (location = 14) uniform int u_Technique;

void main() {
    v_Vertex   = u_ModelViewMatrix * a_Vertex;
    v_Normal   = mat3(u_ModelViewMatrix) * a_Normal;
    v_TexCoord = a_TexCoord;

    //mat4 shadowTransform;
    //shadowTransform = u_LightViewMatrix;
    //if (u_Technique >= 0) {
    //    shadowTransform = u_ShadowTransformMatrix;
    //}
    //else {
    //    shadowTransform = u_LightProjectionMatrix * u_LightViewMatrix;
    //}

    v_LightSpacePos = u_ShadowTransformMatrix * a_Vertex;

    gl_Position = u_ProjectionMatrix * v_Vertex;
}
