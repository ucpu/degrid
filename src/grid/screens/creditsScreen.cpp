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
		GUI_GET_COMPONENT(control, control, panel);
		control.controlType = controlTypeEnum::Panel;
		GUI_GET_COMPONENT(position, position, panel);
		position.x = 0.005;
		position.y = 0.005;
		position.h = 0.99;
		position.xUnit = unitsModeEnum::ScreenHeight;
		position.yUnit = unitsModeEnum::ScreenHeight;
		position.hUnit = unitsModeEnum::ScreenHeight;
	}

	entityClass *column = ents->newEntity(ents->generateUniqueName());
	{
		GUI_GET_COMPONENT(parent, parent, column);
		parent.parent = panel->getName();
		GUI_GET_COMPONENT(layout, layout, column);
		layout.layoutMode = layoutModeEnum::Column;
	}

	static const uint32 textNames[] = {
#define GCHL_GENERATE(N) hashString("gui/credits/" CAGE_STRINGIZE(N)),
		CAGE_EVAL_MEDIUM(CAGE_REPEAT(20, GCHL_GENERATE))
#undef GCHL_GENERATE
	};

	for (uint32 i = 0; i < sizeof(textNames) / sizeof(textNames[0]); i++)
	{
		entityClass *label = gui()->entities()->newEntity(i + 1);
		GUI_GET_COMPONENT(parent, parent, label);
		parent.parent = column->getName();
		parent.ordering = i;
		GUI_GET_COMPONENT(control, control, label);
		control.controlType = controlTypeEnum::Empty;
		GUI_GET_COMPONENT(text, txt, label);
		txt.assetName = hashString("grid/languages/internationalized.textpack");
		txt.textName = textNames[i];
	}
}