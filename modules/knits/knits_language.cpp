/**************************************************************************/
/*  knits_language.cpp                                                    */
/**************************************************************************/

#include "knits_language.h"
#include "knits_script.h"
#include "core/config/engine.h"

KnitsScriptLanguage *KnitsScriptLanguage::singleton = nullptr;

void KnitsScriptLanguage::_bind_methods() {
}

void KnitsScriptLanguage::init() {
}

void KnitsScriptLanguage::finish() {
}

Ref<Script> KnitsScriptLanguage::make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const {
	Ref<KnitsScript> script;
	script.instantiate();

	Ref<KnitsGraph> graph;
	graph.instantiate();
	graph->graph_name = p_class_name.is_empty() ? "MainGraph" : p_class_name;

	// Create default entry event node (_ready)
	Ref<KnitNode> ready_event = graph->create_node(KnitNodeCategory::Event, "_ready", Vector2(100, 150));
	KnitTypeSignature sig_exec;
	sig_exec.kind = KnitDataType::Execution;
	ready_event->add_output_pin("FlowOut", KnitPinKind::Execution, sig_exec);

	script->set_graph(graph);
	return script;
}

void KnitsScriptLanguage::frame() {
	double delta = 1.0 / 60.0;
	shared_vm.tick_coroutines(delta);
}

KnitsScriptLanguage::KnitsScriptLanguage() {
	singleton = this;
}

KnitsScriptLanguage::~KnitsScriptLanguage() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
