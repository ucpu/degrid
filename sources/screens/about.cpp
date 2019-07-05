#include "screens.h"
#include <cage-core/enumerate.h>

void setScreenCredits()
{
	regenerateGui(guiConfig());
	entityManager *ents = gui()->entities();

	entity *panel = ents->createUnique();
	{
		CAGE_COMPONENT_GUI(panel, panel2, panel);
		CAGE_COMPONENT_GUI(parent, parent, panel);
		parent.parent = 12;
		CAGE_COMPONENT_GUI(layoutLine, layout, panel);
		layout.vertical = true;
	}

	static const uint32 textNames[] = {
#define GCHL_GENERATE(N) hashString("gui/credits/" CAGE_STRINGIZE(N)),
		GCHL_GENERATE(0)
		CAGE_EVAL_MEDIUM(CAGE_REPEAT(20, GCHL_GENERATE))
#undef GCHL_GENERATE
	};

	for (auto it : enumerate(textNames))
	{
		entity *label = gui()->entities()->createUnique();
		CAGE_COMPONENT_GUI(parent, parent, label);
		parent.parent = panel->name();
		parent.order = numeric_cast<sint32>(it.cnt);
		CAGE_COMPONENT_GUI(label, lab, label);
		CAGE_COMPONENT_GUI(text, txt, label);
		txt.assetName = hashString("degrid/languages/internationalized.textpack");
		txt.textName = *it;
	}
}
