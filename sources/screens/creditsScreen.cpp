#include "../includes.h"
#include "../screens.h"

void setScreenCredits()
{
	eraseGui();
	generateLogo();
	generateButtonBack();
	entityManagerClass *ents = gui()->entities();

	entityClass *panel = ents->newEntity(ents->generateUniqueName());
	{
		GUI_GET_COMPONENT(groupBox, groupBox, panel);
		groupBox.type = groupBoxTypeEnum::Panel;
		GUI_GET_COMPONENT(position, position, panel);
		position.size.values[1] = 1;
		position.size.units[1] = unitEnum::ScreenHeight;
		GUI_GET_COMPONENT(layoutLine, layout, panel);
		layout.vertical = true;
	}

	static const uint32 textNames[] = {
#define GCHL_GENERATE(N) hashString("gui/credits/" CAGE_STRINGIZE(N)),
		GCHL_GENERATE(0)
		CAGE_EVAL_MEDIUM(CAGE_REPEAT(20, GCHL_GENERATE))
#undef GCHL_GENERATE
	};

	uint32 idx = 0;
	for (auto it : textNames)
	{
		entityClass *label = gui()->entities()->newEntity(idx + 1);
		GUI_GET_COMPONENT(parent, parent, label);
		parent.parent = panel->getName();
		parent.order = idx;
		GUI_GET_COMPONENT(label, lab, label);
		GUI_GET_COMPONENT(text, txt, label);
		txt.assetName = hashString("grid/languages/internationalized.textpack");
		txt.textName = textNames[idx];
		idx++;
	}
}
