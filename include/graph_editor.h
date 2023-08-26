#pragma once

void show_graph_editor(bool* open);
void save(const char* filename);
void load(const char* filename);

namespace example {
	void NodeEditorInitialize();
	void NodeEditorShow(float time, bool* open);
	void NodeEditorShutdown();
} // namespace example