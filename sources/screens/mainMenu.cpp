#include "screens.h"
#include "../game.h"

#include <cage-core/config.h>
#include <cage-core/files.h>

void reloadLanguage(uint32 index);
extern configUint32 confLanguage;

namespace
{
	eventListener<bool(uint32)> guiEvent;

	bool guiFunction(uint32 en)
	{
		switch (en)
		{
		case 103:
			setScreenGame();
			return true;
		case 104:
			setScreenOptions();
			return true;
		case 105:
			setScreenScores();
			return true;
		case 108:
			setScreenAchievements();
			return true;
		case 106:
			setScreenCredits();
			return true;
		case 107:
			engineStop();
			return true;
		}
		if (en >= 200)
		{
			reloadLanguage(confLanguage = en - 200);
			return true;
		}
		return false;
	}
}

void setScreenMainmenu()
{
	{
		guiConfig c;
		c.backButton = false;
		regenerateGui(c);
	}
	entityManager *ents = gui()->entities();
	guiEvent.bind<&guiFunction>();
	guiEvent.attach(gui()->widgetEvent);

	{ // main menu
		{
			entity *e = ents->get(4);
			CAGE_COMPONENT_GUI(layoutSplitter, ls, e);
			ls.inverse = false;
		}
		{
			entity *e = ents->get(5);
			CAGE_COMPONENT_GUI(layoutSplitter, ls, e);
			ls.inverse = false;
		}
		{
			entity *e = ents->get(15);
			CAGE_COMPONENT_GUI(scrollbars, sc, e);
			sc.alignment = vec2(0.8, 0.7);
		}

		entity *panel = ents->createUnique();
		{
			CAGE_COMPONENT_GUI(panel, panel2, panel);
			CAGE_COMPONENT_GUI(parent, parent, panel);
			parent.parent = 15;
			CAGE_COMPONENT_GUI(layoutLine, layout, panel);
			layout.vertical = true;
		}

		{
			entity *butNewGame = ents->create(103);
			CAGE_COMPONENT_GUI(parent, parent, butNewGame);
			parent.parent = panel->name();
			parent.order = 1;
			CAGE_COMPONENT_GUI(button, control, butNewGame);
			CAGE_COMPONENT_GUI(text, txt, butNewGame);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/mainmenu/newgame");
			CAGE_COMPONENT_GUI(textFormat, format, butNewGame);
			format.color = redPillColor;
		}

		{
			entity *butOptions = ents->create(104);
			CAGE_COMPONENT_GUI(parent, parent, butOptions);
			parent.parent = panel->name();
			parent.order = 2;
			CAGE_COMPONENT_GUI(button, control, butOptions);
			CAGE_COMPONENT_GUI(text, txt, butOptions);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/mainmenu/options");
		}

		if ((pathType("score.ini") & pathTypeFlags::File) == pathTypeFlags::File)
		{
			entity *butScores = ents->create(105);
			CAGE_COMPONENT_GUI(parent, parent, butScores);
			parent.parent = panel->name();
			parent.order = 3;
			CAGE_COMPONENT_GUI(button, control, butScores);
			CAGE_COMPONENT_GUI(text, txt, butScores);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/mainmenu/scores");
		}

		if (achievements.acquired > 0)
		{
			entity *butAchivs = ents->create(108);
			CAGE_COMPONENT_GUI(parent, parent, butAchivs);
			parent.parent = panel->name();
			parent.order = 4;
			CAGE_COMPONENT_GUI(button, control, butAchivs);
			CAGE_COMPONENT_GUI(text, txt, butAchivs);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/mainmenu/achievements");
		}

		{
			entity *butCredits = ents->create(106);
			CAGE_COMPONENT_GUI(parent, parent, butCredits);
			parent.parent = panel->name();
			parent.order = 5;
			CAGE_COMPONENT_GUI(button, control, butCredits);
			CAGE_COMPONENT_GUI(text, txt, butCredits);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/mainmenu/credits");
		}

		{
			entity *butQuit = ents->create(107);
			CAGE_COMPONENT_GUI(parent, parent, butQuit);
			parent.parent = panel->name();
			parent.order = 6;
			CAGE_COMPONENT_GUI(button, control, butQuit);
			CAGE_COMPONENT_GUI(text, txt, butQuit);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/mainmenu/quit");
			CAGE_COMPONENT_GUI(textFormat, format, butQuit);
			format.color = bluePillColor;
		}
	}

	{ // languages
		entity *column = ents->createUnique();
		{
			CAGE_COMPONENT_GUI(parent, parent, column);
			parent.parent = 10;
			CAGE_COMPONENT_GUI(layoutLine, layout, column);
			layout.vertical = true;
		}

		static const uint32 flags[] = {
			hashString("degrid/languages/english.png"),
			hashString("degrid/languages/czech.png")
		};

		for (uint32 i = 0; i < sizeof(flags) / sizeof(flags[0]); i++)
		{
			entity *but = ents->create(200 + i);
			CAGE_COMPONENT_GUI(parent, parent, but);
			parent.parent = column->name();
			parent.order = i;
			CAGE_COMPONENT_GUI(button, control, but);
			CAGE_COMPONENT_GUI(image, img, but);
			img.textureName = flags[i];
			CAGE_COMPONENT_GUI(explicitSize, size, but);
			size.size = vec2(80, 40);
		}
	}
}

namespace
{
	class callbacksClass
	{
		eventListener<void()> engineInitListener;
	public:
		callbacksClass()
		{
			engineInitListener.attach(controlThread().initialize, 200);
			engineInitListener.bind<&setScreenMainmenu>();
		}
	} callbacksInstance;
}
