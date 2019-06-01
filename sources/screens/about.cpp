#include "screens.h"
#include <cage-core/enumerate.h>

void setScreenCredits()
{
	regenerateGui(guiConfig());
	entityManagerClass *ents = gui()->entities();

	entityClass *panel = ents->createUnique();
	{
		GUI_GET_COMPONENT(panel, panel2, panel);
		GUI_GET_COMPONENT(parent, parent, panel);
		parent.parent = 12;
		GUI_GET_COMPONENT(layoutLine, layout, panel);
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
		entityClass *label = gui()->entities()->createUnique();
		GUI_GET_COMPONENT(parent, parent, label);
		parent.parent = panel->name();
		parent.order = numeric_cast<sint32>(it.cnt);
		GUI_GET_COMPONENT(label, lab, label);
		GUI_GET_COMPONENT(text, txt, label);
		txt.assetName = hashString("degrid/languages/internationalized.textpack");
		txt.textName = *it;
	}
}
