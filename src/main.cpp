/*
 * Displays the ImGui test window.
 *
 * This is free and unencumbered software released into the public domain. 
 */

#include "bigg.hpp"
#include "value.h"
#include "imgui.h"
#include <imnodes.h>
#include "node_editor.h"
#include "graph_editor.h"

class NNGarden : public bigg::Application
{
	void initialize(int _argc, char** _argv) {
		ImNodes::CreateContext();
		example::NodeEditorInitialize();
		start_test_net();
	}
	void onReset() {
		bgfx::setViewClear( 0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303841ff, 1.0f, 0 );
		bgfx::setViewRect( 0, 0, 0, uint16_t( getWidth() ), uint16_t( getHeight() ) );
	}

	bool graph_open = true;
	bool demo_open = false;

	void update( float dt ) {
		bgfx::touch( 0 );
		ImGui::BeginMainMenuBar();

		if (ImGui::BeginMenu("File"))
		{
			ImGui::MenuItem("New");
			ImGui::MenuItem("Create");
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Window"))
		{
			ImGui::MenuItem("Graph Window", "", &graph_open);
			ImGui::MenuItem("Demo Window", "", &demo_open);

			//ImGui::MenuItem("Graph");
			//ImGui::EndMenu("Control");
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
		ImGui::DockSpaceOverViewport();

		show_graph_editor(&graph_open);

		// Rendering
		//ImGui::Render();
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