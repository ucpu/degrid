#include "screens.h"
#include <cage-core/enumerate.h>

void setScreenAbout()
{
	regenerateGui(GuiConfig());
	EntityManager *ents = gui()->entities();

	Entity *panel = ents->createUnique();
	{
		CAGE_COMPONENT_GUI(Panel, panel2, panel);
		CAGE_COMPONENT_GUI(Parent, parent, panel);
		parent.parent = 12;
		CAGE_COMPONENT_GUI(LayoutLine, layout, panel);
		layout.vertical = true;
	}

	static const uint32 textNames[] = {
#define GCHL_GENERATE(N) HashString("gui/credits/" CAGE_STRINGIZE(N)),
		GCHL_GENERATE(0)
		CAGE_EVAL_MEDIUM(CAGE_REPEAT(20, GCHL_GENERATE))
#undef GCHL_GENERATE
	};

	for (auto it : enumerate(textNames))
	{
		Entity *label = gui()->entities()->createUnique();
		CAGE_COMPONENT_GUI(Parent, parent, label);
		parent.parent = panel->name();
		parent.order = numeric_cast<sint32>(it.cnt);
		CAGE_COMPONENT_GUI(Label, lab, label);
		CAGE_COMPONENT_GUI(Text, txt, label);
		txt.assetName = HashString("degrid/languages/internationalized.textpack");
		txt.textName = *it;
	}
}
