#version 430 core

in vec2 texCoords;

layout (binding = 0) uniform sampler2D inputTexture[8];

layout (location = 21) uniform bool horizontal;
layout (location = 22) uniform int u_Blursize;
layout (location = 23) uniform int u_Layers;

// Gaussian kernels
const float blurKernel[36] = float[](
	0.319466,	0.361069,	0.319466,	0.0,		0.0,		0.0,		0.0,		0.0,		0.0,	 // kernel 3, sigma = 2
	0.153388,	0.221461,	0.250301,	0.221461,	0.153388,	0.0,		0.0,		0.0,		0.0,	 // kernel 5, sigma = 2
	0.071303,	0.131514,	0.189879,	0.214607,	0.189879,	0.131514,	0.071303,	0.0,		0.0,	 // kernel 7, sigma = 2
	0.028532,	0.067234,	0.124009,	0.179044,	0.20236,	0.179044,	0.124009,	0.067234,	0.028532 // kernel 9, sigma = 2
);

layout (location = 0) out vec4 FragColor[8]; // MRT blur

void main(void) {
	vec4 color[8];
	// init
	int offset = 9 * (u_Blursize - 1);
	int limit = (u_Layers % 2 == 1) ? u_Layers / 2 + 1 : u_Layers / 2;
	//int limit = (u_Layers + 1) / 2;
	for (int k = 0; k < 8; k++) color[k] = vec4(0.0);
	// horizontal blur pass
	if (horizontal) {
		for (int i = 0; i < limit; i++) {
			for (int x = -u_Blursize; x <= u_Blursize; x++) {
				color[i] += textureOffset(inputTexture[i], texCoords, ivec2(x, 0.0)) * blurKernel[x + u_Blursize + offset];
			}
		}
	} // vertical blur pass
	else {
		for (int i = 0; i < limit; i++) {
			for (int y = -u_Blursize; y <= u_Blursize; y++) {
				color[i] += textureOffset(inputTexture[i], texCoords, ivec2(0.0, y)) * blurKernel[y + u_Blursize + offset];
			}
		}
	}
	// set fragment colors
	for (int k = 0; k < 8; k++) FragColor[k] = color[k];
}