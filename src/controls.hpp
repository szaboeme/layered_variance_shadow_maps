//-----------------------------------------------------------------------------
//  [PGR2] Simple Shadow Mapping
//  23/04/2017
//-----------------------------------------------------------------------------
const char* help_message =
"[PGR2] Simple Shadow Mapping\n\
-------------------------------------------------------------------------------\n\
CONTROLS:\n\
   [i/I]   ... inc/dec value of user integer variable\n\
   [f/F]   ... inc/dec value of user floating-point variable\n\
   [z/Z]   ... move scene along z-axis\n\
   [s]     ... show/hide first depth map texture\n\
   [d/D]   ... change depth map resolution\n\
   [c]     ... compile shaders\n\
   [mouse] ... scene rotation (left button)\n\
-------------------------------------------------------------------------------";

// IMPLEMENTATION______________________________________________________________

//-----------------------------------------------------------------------------
// Name: updateLightViewMatrix()
// Desc: 
//-----------------------------------------------------------------------------
void updateLightViewMatrix() {
	// Compute light view matrix
	g_LightViewMatrix = glm::lookAt(g_LightPosition,
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::normalize(glm::vec3(-g_LightPosition.y, g_LightPosition.x, 0.0f)));
}


//-----------------------------------------------------------------------------
// Name: compileShaders()
// Desc: 
//-----------------------------------------------------------------------------
void compileShaders(void* clientData) {
	// Create shader program object
	Tools::Shader::CreateShaderProgramFromFile(g_ProgramId[DepthTextureGeneration],
		"shadow_mapping_1st_pass.vs", nullptr, nullptr, nullptr, "shadow_mapping_1st_pass.fs");
	Tools::Shader::CreateShaderProgramFromFile(g_ProgramId[ShadowTest], "shadow_mapping_2nd_pass.vs",
		nullptr, nullptr, nullptr, "shadow_mapping_2nd_pass.fs");
	// compile VSM shaders, two passes
	Tools::Shader::CreateShaderProgramFromFile(g_ProgramId[VSMGeneration],
		"vsm_1st_pass.vs", nullptr, nullptr, nullptr, "vsm_1st_pass.fs");
	Tools::Shader::CreateShaderProgramFromFile(g_ProgramId[VSMTest], "vsm_2nd_pass.vs",
		nullptr, nullptr, nullptr, "vsm_2nd_pass.fs");
	// compile texture blurring shader
	Tools::Shader::CreateShaderProgramFromFile(g_ProgramId[BlurPass], "blur.vs",
		nullptr, nullptr, nullptr, "blur.fs");
}


//-----------------------------------------------------------------------------
// Name: showGUI()
// Desc: 
//-----------------------------------------------------------------------------
void showGUI(void* user) {
	ImGui::Begin("Controls");
	ImGui::SetWindowSize(ImVec2(260, 470), ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Render", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("show texture", &g_ShowDepthTexture);
	}

	if (ImGui::CollapsingHeader("Virtual Framebuffer", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SetNextItemWidth(120);
		int resolution = int(glm::round(glm::log2(g_Resolution / 32.0f)));
		if (ImGui::Combo("resolution", &resolution, " 32x32\0 64x64\0 128x128 \0 256x256\0 512x512 \0 1024x1024\0 2048x2048\0")) {
			g_Resolution = 32 << resolution;
		}
	}

	if (ImGui::CollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen)) {
		int type;
		ImGui::SetNextItemWidth(120);
		if (ImGui::Combo("Technique", &type, " Primitive\0 PCF\0 VSM\0")) {
			g_Technique = type;
		}
		ImGui::SetNextItemWidth(120);
		ImGui::SliderFloat("PCF filter", &g_PCFsize, 0, 1, "%.2f");
		ImGui::SetNextItemWidth(120);
		ImGui::SliderInt("Layers", &g_Layers, 1, 16);
		ImGui::SetNextItemWidth(120);
		ImGui::SliderFloat("Layer pos.", &g_Middle, 0.0f, 1.0f, "%.2f");
		ImGui::SetNextItemWidth(120);
		ImGui::SliderFloat("Layer overlap", &g_Overlap, 0.0f, 1.0f, "%.2f");
		ImGui::SetNextItemWidth(120);
		ImGui::SliderFloat("Light bias", &g_LightBias, 0.0f, 1.0f, "%.3f");
		ImGui::SetNextItemWidth(120);
		ImGui::SliderFloat("Depth bias", &g_DepthBias, 0.0f, 0.01f, "%.4f");
		ImGui::SetNextItemWidth(120);
		ImGui::SliderFloat("Shadow bias", &g_ShadowBias, 0.0f, 0.01f, "%.4f");
		ImGui::SetNextItemWidth(120);
		ImGui::Checkbox("Cull faces", &g_FaceCull);
		int kernel;
		ImGui::SetNextItemWidth(100);
		ImGui::Combo("Blur kernel", &g_Blursize, " no blur\0 3x3\0 5x5\0 7x7\0 9x9\0");
	}

	if (ImGui::CollapsingHeader("Queries", ImGuiTreeNodeFlags_DefaultOpen))
	{
		char buf[32];
		ImGui::Text("shadow gen."); ImGui::SameLine(110); sprintf(buf, "%u", g_QueryResult[DepthTextureGeneration]); ImGui::Button(buf, ImVec2(110.0f, 0.0f));
		ImGui::Text("shadow test"); ImGui::SameLine(110); sprintf(buf, "%u", g_QueryResult[ShadowTest]); ImGui::Button(buf, ImVec2(110.0f, 0.0f));
		ImGui::Text("blur"); ImGui::SameLine(110); sprintf(buf, "%u", (g_Blursize > 0) ? g_QueryResult[BlurPass] : 0); ImGui::Button(buf, ImVec2(110.0f, 0.0f));
	}

	if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SetNextItemWidth(190);
		ImGui::SliderFloat("x", &g_LightPosition.x, -30.0f, 30.0f, "%.3f");
		ImGui::SetNextItemWidth(190);
		ImGui::SliderFloat("y", &g_LightPosition.y, -30.0f, 30.0f, "%.3f");
		ImGui::SetNextItemWidth(190);
		ImGui::SliderFloat("z", &g_LightPosition.z, -30.0f, 30.0f, "%.3f");
	}

	ImGui::End();
	*static_cast<int*>(user) = 485;
}


//-----------------------------------------------------------------------------
// Name: keyboardChanged()
// Desc:
//-----------------------------------------------------------------------------
void keyboardChanged(int key, int action, int mods) {
	switch (key) {
	case GLFW_KEY_S: g_ShowDepthTexture = !g_ShowDepthTexture; break;
	// change depth map resolution
	case GLFW_KEY_D: g_Resolution = (mods == GLFW_MOD_SHIFT) ? ((g_Resolution == 32) ? 2048 : (g_Resolution >> 1)) : ((g_Resolution == 2048) ? 32 : (g_Resolution << 1)); break;
	// change shadow mode
	case GLFW_KEY_T: g_Technique = (mods == GLFW_MOD_SHIFT) ? ((g_Technique == 0) ? 2 : g_Technique - 1) : ((g_Technique == 2) ? 0 : g_Technique + 1);
	case GLFW_KEY_0: if (g_Technique == VSM) g_Blursize = 0; break; // no blur
	case GLFW_KEY_3: if (g_Technique == VSM) g_Blursize = 1; break; // kernel 3
	case GLFW_KEY_5: if (g_Technique == VSM) g_Blursize = 2; break; // kernel 5
	case GLFW_KEY_7: if (g_Technique == VSM) g_Blursize = 3; break; // kernel 7
	case GLFW_KEY_9: if (g_Technique == VSM) g_Blursize = 4; break; // kernel 9
	}
}


//-----------------------------------------------------------------------------
// Name: main()
// Desc: 
//-----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
	int OGL_CONFIGURATION[] = {
		GLFW_CONTEXT_VERSION_MAJOR,  4,
		GLFW_CONTEXT_VERSION_MINOR,  0,
		GLFW_OPENGL_FORWARD_COMPAT,  GL_FALSE,
		GLFW_OPENGL_DEBUG_CONTEXT,   GL_TRUE,
		GLFW_OPENGL_PROFILE,         GLFW_OPENGL_COMPAT_PROFILE, // GLFW_OPENGL_CORE_PROFILE
		PGR2_SHOW_MEMORY_STATISTICS, GL_TRUE,
		0
	};

	printf("%s\n", help_message);

	return common_main(1200, 900, "[PGR2] Simple Shadow Mapping",
		OGL_CONFIGURATION, // OGL configuration hints
		initGL,            // Init GL callback function
		nullptr,           // Release GL callback function
		showGUI,           // Show GUI callback function
		display,           // Display callback function
		nullptr,           // Window resize callback function
		keyboardChanged,   // Keyboard callback function
		nullptr,           // Mouse button callback function
		nullptr);          // Mouse motion callback function
}
