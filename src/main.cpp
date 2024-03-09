/*
 * Displays the ImGui test window.
 *
 * This is free and unencumbered software released into the public domain. 
 */
#define IMGUI_DEFINE_MATH_OPERATORS


#include "value.h"
#include <imnodes.h>
#include "bgfx_utils.h"
#include "imgui.h"
#include "imgui/imgui.h"
#include "node_editor.h"
#include "context.h"
#include "imgui_canvas.h"
#include "common.h"

class NNGarden : public entry::AppI
{
public:
	NNGarden(const char* _name, const char* _description, const char* _url)
		: entry::AppI(_name, _description, _url)
	{
	}

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;
	bgfx::ProgramHandle m_program;
	int64_t m_timeOffset;

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		Args args(_argc, _argv);

		m_width = _width;
		m_height = _height;
		m_debug = BGFX_DEBUG_NONE;
		m_reset = BGFX_RESET_VSYNC;

		bgfx::Init init;
		init.type = args.m_type;
		init.vendorId = args.m_pciId;
		init.platformData.nwh = entry::getNativeWindowHandle(entry::kDefaultWindowHandle);
		init.platformData.ndt = entry::getNativeDisplayHandle();
		init.platformData.type = entry::getNativeWindowHandleType();
		init.resolution.width = m_width;
		init.resolution.height = m_height;
		init.resolution.reset = m_reset;
		bgfx::init(init);

		// Enable debug text.
		bgfx::setDebug(m_debug);

		// Set view 0 clear state.
		bgfx::setViewClear(0
			, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
			, 0x303030ff
			, 1.0f
			, 0
		);

		// Create program from shaders.
		m_program = loadProgram("vs_cubes", "fs_cubes");

		m_timeOffset = bx::getHPCounter();

		imguiCreate();

		ImNodes::CreateContext(0);
		int width = 0;
		int height = 0;
		int comp = 0;

		s_context.main_graph.initialise();
		load("graph.json");
	}

	virtual int shutdown() override
	{
		imguiDestroy();

		bgfx::destroy(m_program);

		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}

	Context s_context;

	int  frame_count = 0;
	bool graph_open = true;
	bool functions_open = true;
	bool demo_open = false;
	bool perf_open = false;

	void show_function_list(bool* open) {
		s_context.show_function_list(open);
	}

	void show_graph_editor(bool* open) {
		s_context.show(open);
	}

	void save(const char* filename) {
		s_context.save(filename);
	}

	void load(const char* filename) {
		s_context.load(filename);
	}

	bool update() override {
		bgfx::touch( 0 );
		if (frame_count == 0) {
		}

		frame_count++;

		ImGui::BeginMainMenuBar();

		if (ImGui::BeginMenu("File"))
		{

			if (ImGui::MenuItem("Save")) {
				save("graph.json");
			}

			if (ImGui::MenuItem("Load")) {
				load("graph.json");
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Window"))
		{
			ImGui::MenuItem("Graph Window", "", &graph_open);
			ImGui::MenuItem("Functions", "", &functions_open);
			ImGui::MenuItem("Demo Window", "", &demo_open);
			ImGui::MenuItem("Performance", "", &perf_open);

			//ImGui::MenuItem("Graph");
			//ImGui::EndMenu("Control");
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();

		if (ImGui::BeginPopup("SavePopup", ImGuiWindowFlags_MenuBar)) {
			ImGui::Text("Save");
			ImGui::EndPopup();
		}

		//ImGui::DockSpaceOverViewport();

		//example::NodeEditorShow(dt, &graph_open);

		show_function_list(&functions_open);
		show_graph_editor(&graph_open);
		

		// Rendering
		//ImGui::Render();
		if (perf_open) {
			ImGui::Begin("Performance", &perf_open);
			ImGui::Text("%.1f", ImGui::GetIO().Framerate);
			ImGui::End();
		}
		if (demo_open)
			ImGui::ShowDemoWindow(&demo_open);

		return true;
	}
};

ENTRY_IMPLEMENT_MAIN(
	NNGarden
	, "NNGarden"
	, "Rendering simple static mesh."
	, "https://bkaradzic.github.io/bgfx/examples.html#cubes"
);