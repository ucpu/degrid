#include <cage-core/enumerate.h>
#include <cage-core/macros.h>

#include "screens.h"

void setScreenAbout()
{
	regenerateGui(GuiConfig());
	EntityManager *ents = engineGuiEntities();

	Entity *panel = ents->createUnique();
	{
		panel->value<GuiPanelComponent>();
		panel->value<GuiParentComponent>().parent = 12;
		panel->value<GuiLayoutLineComponent>().vertical = true;
		panel->value<GuiWidgetStateComponent>().skinIndex = 2; // compact skin
	}

	static constexpr const uint32 TextNames[] = {
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
		label->value<GuiLabelComponent>();
		GuiTextComponent &txt = label->value<GuiTextComponent>();
		txt.assetName = HashString("degrid/languages/internationalized.textpack");
		txt.textName = *it;
	}
}
