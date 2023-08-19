#pragma once

void show_graph_editor(bool* open);

namespace example {
	void NodeEditorInitialize();
	void NodeEditorShow(float time, bool* open);
	void NodeEditorShutdown();
} // namespace example