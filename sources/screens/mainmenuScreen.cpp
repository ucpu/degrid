#include "../includes.h"
#include "../screens.h"

void reloadLanguage(uint32 index);
extern configUint32 confLanguage;

namespace
{
	eventListener<bool(uint32)> guiEvent;

	bool guiFunction(uint32 en)
	{
		switch (en)
		{
		case 13:
			setScreenGame();
			return true;
		case 14:
			setScreenOptions();
			return true;
		case 15:
			setScreenScores();
			return true;
		case 16:
			setScreenCredits();
			return true;
		case 17:
			engineStop();
			return true;
		}
		if (en >= 100 && en < 150)
		{
			reloadLanguage(confLanguage = en - 100);
			return true;
		}
		return false;
	}
}

void setScreenMainmenu()
{
	eraseGui();
	generateLogo();
	entityManagerClass *ents = gui()->entities();
	guiEvent.bind<&guiFunction>();
	guiEvent.attach(gui()->widgetEvent);

	{ // main menu
		entityClass *panel = ents->newUniqueEntity();
		{
			GUI_GET_COMPONENT(groupBox, groupBox, panel);
			groupBox.type = groupBoxTypeEnum::Panel;
			GUI_GET_COMPONENT(position, position, panel);
			position.anchor = vec2(0.5, 0.5);
			position.position.values[0] = 0.8;
			position.position.units[0] = unitEnum::ScreenWidth;
			position.position.values[1] = 0.6;
			position.position.units[1] = unitEnum::ScreenHeight;
			GUI_GET_COMPONENT(layoutLine, layout, panel);
			layout.vertical = true;
		}

		{
			entityClass *butNewGame = ents->newEntity(13);
			GUI_GET_COMPONENT(parent, parent, butNewGame);
			parent.parent = panel->getName();
			parent.order = 1;
			GUI_GET_COMPONENT(button, control, butNewGame);
			GUI_GET_COMPONENT(text, txt, butNewGame);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/mainmenu/newgame");
		}

		{
			entityClass *butOptions = ents->newEntity(14);
			GUI_GET_COMPONENT(parent, parent, butOptions);
			parent.parent = panel->getName();
			parent.order = 2;
			GUI_GET_COMPONENT(button, control, butOptions);
			GUI_GET_COMPONENT(text, txt, butOptions);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/mainmenu/options");
		}

		{
			entityClass *butScores = ents->newEntity(15);
			GUI_GET_COMPONENT(parent, parent, butScores);
			parent.parent = panel->getName();
			parent.order = 3;
			GUI_GET_COMPONENT(button, control, butScores);
			GUI_GET_COMPONENT(text, txt, butScores);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/mainmenu/scores");
		}

		{
			entityClass *butCredits = ents->newEntity(16);
			GUI_GET_COMPONENT(parent, parent, butCredits);
			parent.parent = panel->getName();
			parent.order = 4;
			GUI_GET_COMPONENT(button, control, butCredits);
			GUI_GET_COMPONENT(text, txt, butCredits);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/mainmenu/credits");
		}

		{
			entityClass *butQuit = ents->newEntity(17);
			GUI_GET_COMPONENT(parent, parent, butQuit);
			parent.parent = panel->getName();
			parent.order = 5;
			GUI_GET_COMPONENT(button, control, butQuit);
			GUI_GET_COMPONENT(text, txt, butQuit);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/mainmenu/quit");
		}
	}

	{ // languages
		entityClass *column = ents->newUniqueEntity();
		{
			GUI_GET_COMPONENT(layoutLine, layout, column);
			layout.vertical = true;
		}

		static const uint32 flags[] = {
			hashString("grid/languages/english.png"),
			hashString("grid/languages/czech.png")
		};

		for (uint32 i = 0; i < sizeof(flags) / sizeof(flags[0]); i++)
		{
			entityClass *but = ents->newEntity(100 + i);
			GUI_GET_COMPONENT(parent, parent, but);
			parent.parent = column->getName();
			parent.order = i;
			GUI_GET_COMPONENT(button, control, but);
			GUI_GET_COMPONENT(image, img, but);
			img.textureName = flags[i];
			GUI_GET_COMPONENT(position, position, but);
			position.anchor = vec2(0, 0);
			position.size.values[0] = 0.1;
			position.size.units[0] = unitEnum::ScreenHeight;
			position.size.values[1] = 0.05;
			position.size.units[1] = unitEnum::ScreenHeight;
		}
	}

	gui()->skipAllEventsUntilNextUpdate();
}
