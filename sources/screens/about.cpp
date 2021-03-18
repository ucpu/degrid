#include <cage-core/enumerate.h>
#include <cage-core/macros.h>

#include "screens.h"

void setScreenAbout()
{
	regenerateGui(GuiConfig());
	EntityManager *ents = engineGui()->entities();

	Entity *panel = ents->createUnique();
	{
		CAGE_COMPONENT_GUI(Panel, panel2, panel);
		CAGE_COMPONENT_GUI(Parent, parent, panel);
		parent.parent = 12;
		CAGE_COMPONENT_GUI(LayoutLine, layout, panel);
		layout.vertical = true;
	}

	constexpr const uint32 TextNames[] = {
#define GCHL_GENERATE(N) HashString("gui/credits/" CAGE_STRINGIZE(N)),
		GCHL_GENERATE(0)
		CAGE_EVAL_MEDIUM(CAGE_REPEAT(20, GCHL_GENERATE))
#undef GCHL_GENERATE
	};

	for (auto it : enumerate(TextNames))
	{
		Entity *label = engineGui()->entities()->createUnique();
		CAGE_COMPONENT_GUI(Parent, parent, label);
		parent.parent = panel->name();
		parent.order = numeric_cast<sint32>(it.index);
		CAGE_COMPONENT_GUI(Label, lab, label);
		CAGE_COMPONENT_GUI(Text, txt, label);
		txt.assetName = HashString("degrid/languages/internationalized.textpack");
		txt.textName = *it;
	}
}
