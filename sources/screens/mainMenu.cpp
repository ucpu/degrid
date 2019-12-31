#include "screens.h"
#include "../game.h"

#include <cage-core/config.h>
#include <cage-core/files.h>

void reloadLanguage(uint32 index);
extern ConfigUint32 confLanguage;

namespace
{
	EventListener<bool(uint32)> guiEvent;

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
			setScreenAbout();
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
		GuiConfig c;
		c.backButton = false;
		regenerateGui(c);
	}
	EntityManager *ents = gui()->entities();
	guiEvent.bind<&guiFunction>();
	guiEvent.attach(gui()->widgetEvent);

	{ // main menu
		{
			Entity *e = ents->get(4);
			CAGE_COMPONENT_GUI(LayoutSplitter, ls, e);
			ls.inverse = false;
		}
		{
			Entity *e = ents->get(5);
			CAGE_COMPONENT_GUI(LayoutSplitter, ls, e);
			ls.inverse = false;
		}
		{
			Entity *e = ents->get(15);
			CAGE_COMPONENT_GUI(Scrollbars, sc, e);
			sc.alignment = vec2(0.8, 0.7);
		}

		Entity *panel = ents->createUnique();
		{
			CAGE_COMPONENT_GUI(Panel, panel2, panel);
			CAGE_COMPONENT_GUI(Parent, parent, panel);
			parent.parent = 15;
			CAGE_COMPONENT_GUI(LayoutLine, layout, panel);
			layout.vertical = true;
		}

		{
			Entity *butNewGame = ents->create(103);
			CAGE_COMPONENT_GUI(Parent, parent, butNewGame);
			parent.parent = panel->name();
			parent.order = 1;
			CAGE_COMPONENT_GUI(Button, control, butNewGame);
			CAGE_COMPONENT_GUI(Text, txt, butNewGame);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/mainmenu/newgame");
			CAGE_COMPONENT_GUI(TextFormat, format, butNewGame);
			format.color = redPillColor;
		}

		{
			Entity *butOptions = ents->create(104);
			CAGE_COMPONENT_GUI(Parent, parent, butOptions);
			parent.parent = panel->name();
			parent.order = 2;
			CAGE_COMPONENT_GUI(Button, control, butOptions);
			CAGE_COMPONENT_GUI(Text, txt, butOptions);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/mainmenu/options");
		}

		if ((pathType("score.ini") & PathTypeFlags::File) == PathTypeFlags::File)
		{
			Entity *butScores = ents->create(105);
			CAGE_COMPONENT_GUI(Parent, parent, butScores);
			parent.parent = panel->name();
			parent.order = 3;
			CAGE_COMPONENT_GUI(Button, control, butScores);
			CAGE_COMPONENT_GUI(Text, txt, butScores);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/mainmenu/scores");
		}

		if (achievements.acquired > 0)
		{
			Entity *butAchivs = ents->create(108);
			CAGE_COMPONENT_GUI(Parent, parent, butAchivs);
			parent.parent = panel->name();
			parent.order = 4;
			CAGE_COMPONENT_GUI(Button, control, butAchivs);
			CAGE_COMPONENT_GUI(Text, txt, butAchivs);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/mainmenu/achievements");
		}

		{
			Entity *butCredits = ents->create(106);
			CAGE_COMPONENT_GUI(Parent, parent, butCredits);
			parent.parent = panel->name();
			parent.order = 5;
			CAGE_COMPONENT_GUI(Button, control, butCredits);
			CAGE_COMPONENT_GUI(Text, txt, butCredits);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/mainmenu/credits");
		}

		{
			Entity *butQuit = ents->create(107);
			CAGE_COMPONENT_GUI(Parent, parent, butQuit);
			parent.parent = panel->name();
			parent.order = 6;
			CAGE_COMPONENT_GUI(Button, control, butQuit);
			CAGE_COMPONENT_GUI(Text, txt, butQuit);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/mainmenu/quit");
			CAGE_COMPONENT_GUI(TextFormat, format, butQuit);
			format.color = bluePillColor;
		}
	}

	{ // languages
		Entity *column = ents->createUnique();
		{
			CAGE_COMPONENT_GUI(Parent, parent, column);
			parent.parent = 10;
			CAGE_COMPONENT_GUI(LayoutLine, layout, column);
			layout.vertical = true;
		}

		static const uint32 flags[] = {
			HashString("degrid/languages/english.png"),
			HashString("degrid/languages/czech.png")
		};

		for (uint32 i = 0; i < sizeof(flags) / sizeof(flags[0]); i++)
		{
			Entity *but = ents->create(200 + i);
			CAGE_COMPONENT_GUI(Parent, parent, but);
			parent.parent = column->name();
			parent.order = i;
			CAGE_COMPONENT_GUI(Button, control, but);
			CAGE_COMPONENT_GUI(Image, img, but);
			img.textureName = flags[i];
			CAGE_COMPONENT_GUI(ExplicitSize, size, but);
			size.size = vec2(80, 40);
		}
	}
}

namespace
{
	class Callbacks
	{
		EventListener<void()> engineInitListener;
	public:
		Callbacks()
		{
			engineInitListener.attach(controlThread().initialize, 200);
			engineInitListener.bind<&setScreenMainmenu>();
		}
	} callbacksInstance;
}
