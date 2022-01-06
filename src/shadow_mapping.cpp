//-----------------------------------------------------------------------------
//  [PGR2] Simple Shadow Mapping
//  23/04/2017
//  Extended with Variance and Layered Variance Shadow Mapping
//  04/02/2021
//-----------------------------------------------------------------------------
//  Controls:
//    [i/I]   ... inc/dec value of user integer variable
//    [f/F]   ... inc/dec value of user floating-point variable
//    [z/Z]   ... move scene along z-axis
//    [s]     ... show/hide first depth map texture
//    [d/D]   ... change depth map resolution
//    [l/L]   ... inc/dec light distance from the scene
//    [c]     ... compile shaders
//    [mouse] ... scene rotation (left button)
//    [0/3/5/7/9] ... change Gaussian blur kernel size
//-----------------------------------------------------------------------------
#include "common.h"

// GLOBAL CONSTANTS____________________________________________________________
const char* TEXTURE_FILE_NAME = "../shared/textures/metal01.raw";
enum eTextureType { Diffuse = 0, DepthMap, ZBuffer, ZBufferShadow, NumTextureTypes };

enum eAlgorithmPass {
	DepthTextureGeneration = 0,
	ShadowTest,
	VSMGeneration,
	VSMTest,
	BlurPass,
	NumPasses
};

enum shadowType {
	Primitive, // Z-buffer shadow mapping
	Blur, // blur pass 1
	VSM, // LVSM with one layer, it is VSM
	BlurSecond, // blur pass 2
	NumShadows
};

// two actual layers per layer texture, thus 16 layers max.
enum eLayers { Layer1 = 0, Layer2, Layer3, Layer4, Layer5, Layer6, Layer7, Layer8, NumLayers };

// GLOBAL VARIABLES____________________________________________________________
bool      g_ShowDepthTexture = true;  // Show/hide depth texture
GLint     g_Resolution = 1024;  // FBO size in pixels
GLuint    g_Textures[NumTextureTypes] = { 0 };  // Textures - color (diffuse), depth map, z-buffer, z-buffer for hw shadow map
GLuint    g_ShadowTestSampler = 0;  // Texture sampler for automatic shadow test
GLuint    g_Framebuffer = 0;  // Virtual frame-buffer FBO id
GLuint    g_VSMFramebuffer = 0;  // VSM framebuffer
GLuint    g_FBO[NumShadows] = { 0 };  // FBO array
GLuint    g_ProgramId[NumPasses] = { 0 };  // 1st and 2nd pass shader program IDs
// Transformations
glm::mat4& g_CameraViewMatrix = Variables::Transform.ModelView;  // Camera view transformation ~ scene transformation
glm::mat4& g_CameraProjectionMatrix = Variables::Transform.Projection; // Camera projection transformation ~ scene projection
glm::vec3  g_LightPosition = glm::vec3(0.0f, 20.0f, 0.0f);    // Light orientation 
glm::mat4  g_LightViewMatrix;                                          // Light view transformation
glm::mat4  g_LightProjectionMatrix = glm::frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1000.0f);   // Light projection transformation

// VSM params.
GLint	  g_Layers = 1;	// LVSM Layer count = if 1, it is VSM
GLfloat	  g_PCFsize = 0.1; // how big the PCF fitler neighbourhood
GLfloat	  g_DepthBias = 0.0001; // depth bias  
GLfloat	  g_LightBias = 0.0; // light bias  
GLfloat	  g_ShadowBias = 0.0; // shadow bias 
GLfloat   g_Overlap = 0.0;  // layers overlap delta

GLuint g_Technique = Primitive; // active shadow technique
GLuint g_LayerTextures[NumLayers] = { 0 };  // variance shadow map layer textures
GLuint g_BlurTextures[NumLayers] = { 0 };
GLuint g_BlurSecondTextures[NumLayers] = { 0 };
GLint g_Blursize = 0;
bool g_Blur = false;
bool g_FaceCull = false;
GLfloat g_Middle = 0.5;
GLuint shadowSampler = 0;

// QUERIES
GLuint g_Queries[NumPasses] = { 0 };
GLuint g_QueryResult[NumPasses] = { 0 };

// FORWARD DECLARATIONS________________________________________________________
void createVirtualFramebuffer();
void updateLightViewMatrix();

// IMPLEMENTATION______________________________________________________________

//-----------------------------------------------------------------------------
// Name: display()
// Desc: 
//-----------------------------------------------------------------------------
void display() {
	// Create a frame-buffer object
	createVirtualFramebuffer();

	// Update camera & light transformations
	updateLightViewMatrix();

	// DEPTH TEXTURE GENERATION -----------------------------------------------
	// pick correct shaders
	GLuint pid = (g_Technique == VSM) ? g_ProgramId[VSMGeneration] : g_ProgramId[DepthTextureGeneration];
	glUseProgram(pid);

	// bind correct framebuffer
	if (g_Technique == VSM) {  // VSM & LVSM use the same FBO
		glBindFramebuffer(GL_FRAMEBUFFER, g_FBO[VSM]);
		glUniform1i(21, g_Layers);
		glUniform1f(22, g_Overlap);
		glUniform1f(23, g_Middle);
	}
	else { // Primitive & PCF use the same FBO
		glBindFramebuffer(GL_FRAMEBUFFER, g_FBO[Primitive]);
	}

	glViewport(0, 0, g_Resolution, g_Resolution);
	//glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUniformMatrix4fv(1, 1, GL_FALSE, &g_LightProjectionMatrix[0][0]);
	glUniformMatrix4fv(0, 1, GL_FALSE, &g_LightViewMatrix[0][0]);

	glPolygonOffset(4.0f, 4.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);

	// draw scene - MRT into textures
	if (g_FaceCull) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
	}
	glBeginQuery(GL_TIME_ELAPSED, g_Queries[DepthTextureGeneration]);
	Tools::DrawScene();
	glEndQuery(GL_TIME_ELAPSED);
	if (g_FaceCull) {
		glCullFace(GL_BACK);
		glDisable(GL_CULL_FACE);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// blur variance textures
	// for LVSM, blur all of the layers - MRT
	if ((g_Blursize > 0) && (g_Technique == VSM)) {
		glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
		glViewport(0, 0, g_Resolution, g_Resolution);
		pid = g_ProgramId[BlurPass];
		glUseProgram(pid);

		glBindFramebuffer(GL_FRAMEBUFFER, g_FBO[Blur]); // to BlurTextures

		// horizontal
		glBindTextures(0, NumLayers, g_LayerTextures);  // from LayerTextures
		glUniform1i(21, true);
		glUniform1i(22, g_Blursize);
		glUniform1i(23, g_Layers);
		glBeginQuery(GL_TIME_ELAPSED, g_Queries[BlurPass]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindFramebuffer(GL_FRAMEBUFFER, g_FBO[BlurSecond]); // to SecondBlurTextures

		// vertical
		glBindTextures(0, NumLayers, g_BlurTextures);  // from first pass: BlurTextures
		glUniform1i(21, false);
		glUniform1i(22, g_Blursize);
		glUniform1i(23, g_Layers);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glEndQuery(GL_TIME_ELAPSED);
	}

	glViewport(0, 0, Variables::WindowSize.x, Variables::WindowSize.y);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);

	// SHADOW GENERATION ------------------------------------------------------
	// pick correct shaders
	pid = (g_Technique == VSM) ? g_ProgramId[VSMTest] : g_ProgramId[ShadowTest];
	glUseProgram(pid);
	glClearColor(0.65f, 0.81f, 0.60f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (g_Technique == VSM) {
		// bind surface texture -- diffuse
		glBindSampler(3, 0);
		glBindTextures(0, 1, g_Textures);

		// bind variance map textures
		// blurred, is true
		if (g_Blursize > 0)
			glBindTextures(1, NumLayers, g_BlurSecondTextures);
		else
			glBindTextures(1, NumLayers, g_LayerTextures);
		// bind uniforms
		glUniform1i(13, g_Layers);
		glUniform1f(14, g_Overlap);
		glUniform1f(16, g_LightBias);
		glUniform1f(17, g_DepthBias);
		glUniform1f(18, g_ShadowBias);
		glUniform1f(19, g_Middle);
	}
	else {
		// bind textures: Diffuse, DepthMap, ZBuffer, ZBufferShadow 
		glBindTextures(0, NumTextureTypes, g_Textures);
		// binding the shadow sampler for ZBufferShadow
		glBindSampler(3, shadowSampler);
		// bind uniforms
		glUniform1i(14, g_Technique);
		glUniform1f(15, g_PCFsize);
	}
	// bind transformation matrices
	glUniformMatrix4fv(0, 1, GL_FALSE, &g_CameraProjectionMatrix[0][0]);
	glUniformMatrix4fv(1, 1, GL_FALSE, &g_CameraViewMatrix[0][0]);
	glUniformMatrix4fv(3, 1, GL_FALSE, &g_LightProjectionMatrix[0][0]);
	glUniformMatrix4fv(2, 1, GL_FALSE, &g_LightViewMatrix[0][0]);
	// light position
	const glm::vec4 light_position = (g_CameraViewMatrix * glm::inverse(g_LightViewMatrix)) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	glUniform4fv(glGetUniformLocation(pid, "u_LightPosition"), 1, &light_position.x);
	// shadow transformation matrix
	glm::mat4 matScale = glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(0.5f)), glm::vec3(0.5f));
	glm::mat4 shadowTransformMatrixVSM = g_LightProjectionMatrix * g_LightViewMatrix;
	glm::mat4 shadowTransformMatrix = matScale * shadowTransformMatrixVSM;
	if (g_Technique == VSM)
		glUniformMatrix4fv(4, 1, GL_FALSE, &shadowTransformMatrixVSM[0][0]);
	else
		glUniformMatrix4fv(4, 1, GL_FALSE, &shadowTransformMatrix[0][0]);
	// draw
	glBeginQuery(GL_TIME_ELAPSED, g_Queries[ShadowTest]);
	Tools::DrawScene();
	glEndQuery(GL_TIME_ELAPSED);
	glUseProgram(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Show depth maps
	if (g_ShowDepthTexture) {
		if (g_Technique == VSM) {
			if (g_Blursize > 0) {
				Tools::Texture::Show2DTexture(g_BlurSecondTextures[Layer1], Variables::WindowSize.x - 250, Variables::WindowSize.y - 250, 250, 250);
				Tools::Texture::Show2DTexture(g_BlurSecondTextures[Layer2], Variables::WindowSize.x - 250, Variables::WindowSize.y - 500, 250, 250);
				Tools::Texture::Show2DTexture(g_BlurSecondTextures[Layer3], Variables::WindowSize.x - 250, Variables::WindowSize.y - 750, 250, 250);
				Tools::Texture::Show2DTexture(g_BlurSecondTextures[Layer4], Variables::WindowSize.x - 250, Variables::WindowSize.y - 1000, 250, 250);

			}
			else {
				Tools::Texture::Show2DTexture(g_LayerTextures[Layer1], Variables::WindowSize.x - 250, Variables::WindowSize.y - 250, 250, 250);
				Tools::Texture::Show2DTexture(g_LayerTextures[Layer2], Variables::WindowSize.x - 250, Variables::WindowSize.y - 500, 250, 250);
				Tools::Texture::Show2DTexture(g_LayerTextures[Layer3], Variables::WindowSize.x - 250, Variables::WindowSize.y - 750, 250, 250);
				Tools::Texture::Show2DTexture(g_LayerTextures[Layer4], Variables::WindowSize.x - 250, Variables::WindowSize.y - 1000, 250, 250);
			}
		}
		else {
			Tools::Texture::Show2DTexture(g_Textures[DepthMap], Variables::WindowSize.x - 250, Variables::WindowSize.y - 250, 250, 250);
			Tools::Texture::Show2DTexture(g_Textures[ZBuffer], Variables::WindowSize.x - 250, Variables::WindowSize.y - 500, 250, 250);
		}
	}

	// retrieve queries
	glGetQueryObjectuiv(g_Queries[DepthTextureGeneration], GL_QUERY_RESULT, &g_QueryResult[DepthTextureGeneration]);
	glGetQueryObjectuiv(g_Queries[ShadowTest], GL_QUERY_RESULT, &g_QueryResult[ShadowTest]);
	if (g_Blursize > 0)
		glGetQueryObjectuiv(g_Queries[BlurPass], GL_QUERY_RESULT, &g_QueryResult[BlurPass]);
}


//-----------------------------------------------------------------------------
// Name: createVirtualFramebuffer()
// Desc: 
//-----------------------------------------------------------------------------
void createVirtualFramebuffer() {
	static GLint s_Resolution = 0;
	static GLint s_Technique = 0;

	if ((s_Resolution == g_Resolution) && (s_Technique == g_Technique))
		return;

	glDeleteTextures(NumTextureTypes - 1, &g_Textures[DepthMap]);
	glDeleteTextures(NumLayers, &g_LayerTextures[Layer1]);
	glDeleteTextures(NumLayers, &g_BlurTextures[Layer1]);
	glDeleteTextures(NumLayers, &g_BlurSecondTextures[Layer1]);
	glDeleteFramebuffers(4, g_FBO);

	// generate the layer textures
	if (g_Technique == VSM) {
		// create VSM FBO
		glCreateFramebuffers(1, &g_FBO[VSM]);
		// use PCF FBO index for blur
		glCreateFramebuffers(1, &g_FBO[Blur]);
		glCreateFramebuffers(1, &g_FBO[BlurSecond]);
		// create the variance layer textures for MRT
		for (unsigned int i = 0; i < NumLayers; i++) {
			glCreateTextures(GL_TEXTURE_2D, 1, &g_LayerTextures[i]);
			glTextureParameteri(g_LayerTextures[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(g_LayerTextures[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTextureStorage2D(g_LayerTextures[i], 1, GL_RGBA32F, g_Resolution, g_Resolution);
			glNamedFramebufferTexture(g_FBO[VSM], GL_COLOR_ATTACHMENT0 + i, g_LayerTextures[Layer1 + i], 0);

			// create blur texture for the layer
			glCreateTextures(GL_TEXTURE_2D, 1, &g_BlurTextures[i]);
			glTextureParameteri(g_BlurTextures[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(g_BlurTextures[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTextureParameteri(g_BlurTextures[i], GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(g_BlurTextures[i], GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTextureStorage2D(g_BlurTextures[i], 1, GL_RGBA32F, g_Resolution, g_Resolution);
			glNamedFramebufferTexture(g_FBO[Blur], GL_COLOR_ATTACHMENT0 + i, g_BlurTextures[Layer1 + i], 0);

			glCreateTextures(GL_TEXTURE_2D, 1, &g_BlurSecondTextures[i]);
			glTextureParameteri(g_BlurSecondTextures[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(g_BlurSecondTextures[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTextureParameteri(g_BlurSecondTextures[i], GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTextureParameteri(g_BlurSecondTextures[i], GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTextureStorage2D(g_BlurSecondTextures[i], 1, GL_RGBA32F, g_Resolution, g_Resolution);
			glNamedFramebufferTexture(g_FBO[BlurSecond], GL_COLOR_ATTACHMENT0 + i, g_BlurSecondTextures[Layer1 + i], 0);
		}
		// color attachment list
		GLenum attach_buffer[] = {
			GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
			GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
			GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5,
			GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7
		};
		// set color attachments for VSM FBO
		glNamedFramebufferDrawBuffers(g_FBO[VSM], 8, attach_buffer);
		// set color attachments for the VSM blur textures
		glNamedFramebufferDrawBuffers(g_FBO[Blur], 8, attach_buffer);
		glNamedFramebufferDrawBuffers(g_FBO[BlurSecond], 8, attach_buffer);
	}
	else {
		glCreateTextures(GL_TEXTURE_2D, 1, &g_Textures[DepthMap]);
		glTextureParameteri(g_Textures[DepthMap], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(g_Textures[DepthMap], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureStorage2D(g_Textures[DepthMap], 1, GL_RGBA32F, g_Resolution, g_Resolution);

		glCreateTextures(GL_TEXTURE_2D, 1, &g_Textures[ZBuffer]);
		glTextureParameteri(g_Textures[ZBuffer], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(g_Textures[ZBuffer], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureStorage2D(g_Textures[ZBuffer], 1, GL_DEPTH_COMPONENT32, g_Resolution, g_Resolution);

		glCreateFramebuffers(1, &g_FBO[Primitive]);
		glCreateFramebuffers(1, &g_FBO[Primitive]);
		glNamedFramebufferTexture(g_FBO[Primitive], GL_COLOR_ATTACHMENT0, g_Textures[DepthMap], 0);
		glNamedFramebufferTexture(g_FBO[Primitive], GL_DEPTH_ATTACHMENT, g_Textures[ZBuffer], 0);

		// fill the shadow buffer with the ZBuffer data
		g_Textures[ZBufferShadow] = g_Textures[ZBuffer];
	}

	// Check framebuffer status
	if (g_FBO[Primitive] > 0)
	{
		GLenum const error = glGetError();
		GLenum const status = glCheckNamedFramebufferStatus(g_FBO[Primitive], GL_FRAMEBUFFER);

		if ((status == GL_FRAMEBUFFER_COMPLETE) && (error == GL_NO_ERROR)) {
			s_Resolution = g_Resolution;
			s_Technique = g_Technique;
		}
		else
			printf("FBO creation failed, glCheckFramebufferStatus() = 0x%x, glGetError() = 0x%x\n", status, error);
	}
	if (g_FBO[VSM] > 0)
	{
		GLenum const error = glGetError();
		GLenum const status = glCheckNamedFramebufferStatus(g_FBO[VSM], GL_FRAMEBUFFER);

		if ((status == GL_FRAMEBUFFER_COMPLETE) && (error == GL_NO_ERROR)) {
			s_Resolution = g_Resolution;
			s_Technique = g_Technique;
		}
		else
			printf("VSM FBO creation failed, glCheckFramebufferStatus() = 0x%x, glGetError() = 0x%x\n", status, error);
	}

	if (shadowSampler == 0) {
		glDeleteSamplers(1, &shadowSampler);
		glCreateSamplers(1, &shadowSampler);
		glSamplerParameteri(shadowSampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glSamplerParameteri(shadowSampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glSamplerParameteri(shadowSampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glSamplerParameteri(shadowSampler, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	}

	if (g_Queries[0] == 0)
		glGenQueries(NumPasses, g_Queries);
}


//-----------------------------------------------------------------------------
// Name: initGL()
// Desc: 
//-----------------------------------------------------------------------------
void initGL() {
	// Default scene distance
	Variables::Shader::SceneZOffset = 24.0f;

	// Set OpenGL state variables
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);

	// Load and create texture for scene
	g_Textures[Diffuse] = Tools::Texture::LoadRGB8(TEXTURE_FILE_NAME);

	// Load shader program
	compileShaders();
}


// Include GUI and control stuff
#include "controls.hpp"
