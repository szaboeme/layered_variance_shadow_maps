#version 430 core

#ifdef USER_TEST
#endif

uniform int   u_UserVariableInt;
uniform float u_UserVariableFloat;

layout (location = 21) uniform int u_Layers; 
layout (location = 22) uniform float u_Overlap;
layout (location = 23) uniform float u_Middle;

layout (location = 0) out vec4 FragColor[8];

in vec4 v_LightSpacePos;

float warp(float p, float q, float depth) {
	float low = max(0.0, p - u_Overlap);
	float hi = min(1.0, q + u_Overlap);
	if (depth <= low) return 0.0;
	if (hi <= depth) return 1.0;
	return (depth - low) / (hi - low);
}

vec2 warp2(float p, float q, float step, float depth) {
	vec2 tmp;
	tmp.x = warp(p, q, depth);
	p += step;
	q += step;
	tmp.y = warp(p, q, depth);
	return tmp;
}

void main(void) {
    
	float depth = v_LightSpacePos.z / v_LightSpacePos.w;
	depth = depth * 0.5f + 0.5f;
	float step = 1.0 / float(u_Layers);
	float from = 0.0;
	float to = step;
	vec2 temp; 

	// first layer always exists
	FragColor[0] = vec4(depth, depth * depth, 0.0, 0.0);
	int counter = 0;

	if (u_Layers == 2) {
		float d1 = warp(0.0, u_Middle, depth);
		float d2 = warp(u_Middle, 1.0, depth);
		FragColor[0] = vec4(d1, d1 * d1, d2, d2 * d2);
		counter = 2;
	}

	while (u_Layers - 1 > counter) {
		temp = warp2(from, to, step, depth);
		FragColor[counter / 2] = vec4(temp.x, temp.x * temp.x, temp.y, temp.y * temp.y);
		from += 2 * step;
		to += 2 * step;
		counter += 2;
	}
	if (u_Layers - 1 == counter) { // odd number of layers
		float d = warp(from, to, depth);
		FragColor[counter / 2] = vec4(d, d * d, 0.0, 0.0);
		counter += 2;
	}

	while (counter < 8) {
		FragColor[counter / 2] = vec4(0.0);
		counter += 2;
	}
}