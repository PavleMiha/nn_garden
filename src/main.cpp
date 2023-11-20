/*
 * Displays the ImGui test window.
 *
 * This is free and unencumbered software released into the public domain. 
 */
#define IMGUI_DEFINE_MATH_OPERATORS


#include "bigg.hpp"
#include "value.h"
#include <imnodes.h>
#include "imgui.h"
#include "node_editor.h"
#include "graph_editor.h"
#include "imgui_canvas.h"

class NNGarden : public bigg::Application
{
	void initialize(int _argc, char** _argv) {
		ImNodes::CreateContext(0);
		//load("graph.json");

		//example::NodeEditorInitialize();
	}
	void onReset() {
		bgfx::setViewClear( 0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303841ff, 1.0f, 0 );
		bgfx::setViewRect( 0, 0, 0, uint16_t( getWidth() ), uint16_t( getHeight() ) );
	}

	int  frame_count = 0;
	bool graph_open = true;
	bool functions_open = true;
	bool demo_open = false;
	bool perf_open = false;

	void update( float dt ) {
		bgfx::touch( 0 );
		if (frame_count == 1) {
			load("graph.json");
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

		ImGui::DockSpaceOverViewport();

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
	}
public:
	NNGarden()
		: bigg::Application( "NNGarden" ) {}
};

int main( int argc, char** argv ) {
	NNGarden app;
	return app.run( argc, argv );
}