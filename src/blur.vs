#version 430 core

//in vec2 v_texCoords;
out vec2 texCoords;

const vec2 quadVerts[4] = { vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, 1.0) };
const vec2 quadCoords[4] = { vec2(0.0f, 0.0f), vec2(1.0f, 0.0f), vec2(0.0f, 1.0f), vec2(1.0f, 1.0f)};

void main()
{
    gl_Position = vec4(quadVerts[gl_VertexID], 0.0, 1.0);
	texCoords = quadCoords[gl_VertexID];
}