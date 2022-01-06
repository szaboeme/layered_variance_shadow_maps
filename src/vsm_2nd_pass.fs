#version 430 core

#ifdef USER_TEST
#endif

layout (location = 0) out vec4 FragColor;

in vec3 v_Normal;
in vec2 v_TexCoord;
in vec4 v_Vertex;
in vec4 v_LightSpacePos;

layout (location = 13) uniform int u_Layers; 
layout (location = 14) uniform float u_Overlap;
layout (location = 16) uniform float u_LightBias;
layout (location = 17) uniform float u_DepthBias;
layout (location = 18) uniform float u_ShadowBias;
layout (location = 19) uniform float u_Middle;

uniform int   u_UserVariableInt;
uniform float u_UserVariableFloat;
uniform vec4  u_LightPosition;

layout (binding = 0) uniform sampler2D       u_SceneTexture;
layout (binding = 1) uniform sampler2D       u_VarianceTexture[8];

float warp(float p, float q, float depth) {
	float low = max(0.0, p - u_Overlap);
	float hi = min(1.0, q + u_Overlap);
	return (depth <= low) ? 0.0 : ((hi <= depth) ? 1.0 : (depth - low) / (hi - low));
}

void main() {
    // Compute fragment diffuse color
    vec3 N      = normalize(v_Normal);
    vec3 L      = normalize(u_LightPosition.xyz - v_Vertex.xyz);
    float NdotL = max(dot(N, L), 0.0);
    vec4 color  = texture(u_SceneTexture, v_TexCoord) * NdotL;

    vec4 shadow = vec4(0.0);
    float depth = 0.0;

    vec3 texCoord = v_LightSpacePos.xyz / v_LightSpacePos.w;
	texCoord.z -= u_DepthBias;
	texCoord = texCoord * 0.5 + 0.5;
	float real_depth = texCoord.z;
    
    float step = 1.0 / float(u_Layers);

	// get index of layer that we want to sample
	int index = int(real_depth / step);

	if (u_Layers == 2) { // when two layers with moving middle value
		if (real_depth < u_Middle)
			index = 0;
		else 
			index = 1;
	}

    //sample moments from layer
	vec2 moments = vec2(0.0);
	
	if (index % 2 == 0) {
		moments = texture(u_VarianceTexture[index/2], texCoord.xy).xy;
	}
	else {
		moments = texture(u_VarianceTexture[index/2], texCoord.xy).zw;
	}

	if (index > 0) {
		if (u_Layers == 2) // when two layers with moving middle value
			real_depth = warp(u_Middle, 1.0, real_depth);
		else
			real_depth = warp(index * step, (index + 1) * step, real_depth);
    }

	float lit_factor = float((real_depth - moments.x) <= 0.0);
	// Chebyshev's inequality
	float E_x2 = moments.y;
    float Ex_2 = moments.x * moments.x;
	float variance =  min(max(E_x2 - Ex_2, 0.0) + u_ShadowBias, 1.0); 

	float m_d = (moments.x - real_depth);
	float p = variance / (variance + m_d * m_d);
	// apply light bias for light bleeding reduction
	float p_red = clamp((p - u_LightBias) / (1.0 - u_LightBias), 0.0, 1.0);
	shadow = vec4(max(lit_factor, min(p, p_red)));

    // Modulate fragment's color according to result of shadow test
    FragColor = color * max(vec4(0.2), shadow);
}
