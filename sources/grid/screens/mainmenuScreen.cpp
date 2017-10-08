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
			GUI_GET_COMPONENT(control, c, gui()->entities()->getEntity(en));
			reloadLanguage(confLanguage = c.ival);
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
	guiEvent.attach(gui()->genericEvent);

	{ // main menu
		entityClass *panel = ents->newEntity(ents->generateUniqueName());
		{
			GUI_GET_COMPONENT(control, control, panel);
			control.controlType = controlTypeEnum::Panel;
			GUI_GET_COMPONENT(position, position, panel);
			position.x = 0.8;
			position.y = 0.6;
			position.xUnit = unitsModeEnum::ScreenWidth;
			position.yUnit = unitsModeEnum::ScreenHeight;
			position.anchorX = 0.5;
			position.anchorY = 0.5;
		}

		entityClass *column = ents->newEntity(ents->generateUniqueName());
		{
			GUI_GET_COMPONENT(parent, parent, column);
			parent.parent = panel->getName();
			GUI_GET_COMPONENT(layout, layout, column);
			layout.layoutMode = layoutModeEnum::Column;
			GUI_GET_COMPONENT(format, format, column);
			format.align = textAlignEnum::Center;
		}

		{
			entityClass *butNewGame = ents->newEntity(13);
			GUI_GET_COMPONENT(parent, parent, butNewGame);
			parent.parent = column->getName();
			parent.ordering = 1;
			GUI_GET_COMPONENT(control, control, butNewGame);
			control.controlType = controlTypeEnum::Button;
			GUI_GET_COMPONENT(text, txt, butNewGame);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/mainmenu/newgame");
		}

		{
			entityClass *butOptions = ents->newEntity(14);
			GUI_GET_COMPONENT(parent, parent, butOptions);
			parent.parent = column->getName();
			parent.ordering = 2;
			GUI_GET_COMPONENT(control, control, butOptions);
			control.controlType = controlTypeEnum::Button;
			GUI_GET_COMPONENT(text, txt, butOptions);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/mainmenu/options");
		}

		{
			entityClass *butScores = ents->newEntity(15);
			GUI_GET_COMPONENT(parent, parent, butScores);
			parent.parent = column->getName();
			parent.ordering = 3;
			GUI_GET_COMPONENT(control, control, butScores);
			control.controlType = controlTypeEnum::Button;
			GUI_GET_COMPONENT(text, txt, butScores);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/mainmenu/scores");
		}

		{
			entityClass *butCredits = ents->newEntity(16);
			GUI_GET_COMPONENT(parent, parent, butCredits);
			parent.parent = column->getName();
			parent.ordering = 4;
			GUI_GET_COMPONENT(control, control, butCredits);
			control.controlType = controlTypeEnum::Button;
			GUI_GET_COMPONENT(text, txt, butCredits);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/mainmenu/credits");
		}

		{
			entityClass *butQuit = ents->newEntity(17);
			GUI_GET_COMPONENT(parent, parent, butQuit);
			parent.parent = column->getName();
			parent.ordering = 5;
			GUI_GET_COMPONENT(control, control, butQuit);
			control.controlType = controlTypeEnum::Button;
			GUI_GET_COMPONENT(text, txt, butQuit);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/mainmenu/quit");
		}
	}

	{ // languages
		entityClass *column = ents->newEntity(ents->generateUniqueName());
		{
			GUI_GET_COMPONENT(layout, layout, column);
			layout.layoutMode = layoutModeEnum::Column;
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
			parent.ordering = i;
			GUI_GET_COMPONENT(control, control, but);
			control.controlType = controlTypeEnum::Button;
			control.ival = i;
			GUI_GET_COMPONENT(image, img, but);
			img.textureName = flags[i];
			GUI_GET_COMPONENT(position, position, but);
			position.w = 0.1;
			position.h = 0.05;
			position.wUnit = position.hUnit = unitsModeEnum::ScreenHeight;
		}
	}
}