#include <cage-core/enumerate.h>
#include <cage-core/macros.h>

#include "screens.h"

void setScreenAbout()
{
	regenerateGui(GuiConfig());
	EntityManager *ents = engineGuiEntities();

	Entity *panel = ents->createUnique();
	{
		GuiPanelComponent &panel2 = panel->value<GuiPanelComponent>();
		GuiParentComponent &parent = panel->value<GuiParentComponent>();
		parent.parent = 12;
		GuiLayoutLineComponent &layout = panel->value<GuiLayoutLineComponent>();
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
		Entity *label = engineGuiEntities()->createUnique();
		GuiParentComponent &parent = label->value<GuiParentComponent>();
		parent.parent = panel->name();
		parent.order = numeric_cast<sint32>(it.index);
		GuiLabelComponent &lab = label->value<GuiLabelComponent>();
		GuiTextComponent &txt = label->value<GuiTextComponent>();
		txt.assetName = HashString("degrid/languages/internationalized.textpack");
		txt.textName = *it;
	}
}
