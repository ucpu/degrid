#include <cage-core/config.h>
#include <cage-core/files.h>

#include "screens.h"
#include "../game.h"

void reloadLanguage(uint32 index);
extern ConfigUint32 confLanguage;

namespace
{
	InputListener<InputClassEnum::GuiWidget, InputGuiWidget, bool> guiEvent;

	bool guiFunction(InputGuiWidget in)
	{
		switch (in.widget)
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
		if (in.widget >= 200)
		{
			reloadLanguage(confLanguage = in.widget - 200);
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
	EntityManager *ents = engineGuiEntities();
	guiEvent.bind<&guiFunction>();
	guiEvent.attach(engineGuiManager()->widgetEvent);

	{ // main menu
		{
			Entity *e = ents->get(4);
			e->value<GuiLayoutLineComponent>().begin = LineEdgeModeEnum::Flexible;
			e->value<GuiLayoutLineComponent>().end = LineEdgeModeEnum::Flexible;
		}
		{
			Entity *e = ents->get(5);
			e->value<GuiLayoutLineComponent>().begin = LineEdgeModeEnum::Flexible;
			e->value<GuiLayoutLineComponent>().end = LineEdgeModeEnum::Flexible;
		}
		{
			Entity *e = ents->get(15);
			e->value<GuiLayoutAlignmentComponent>().alignment = Vec2(0.8, 0.7);
		}

		Entity *panel = ents->createUnique();
		{
			panel->value<GuiPanelComponent>();
			panel->value<GuiParentComponent>().parent = 15;
			panel->value<GuiLayoutLineComponent>().vertical = true;
			panel->value<GuiWidgetStateComponent>().skinIndex = 1; // large skin
		}

		{
			Entity *butNewGame = ents->create(103);
			GuiParentComponent &parent = butNewGame->value<GuiParentComponent>();
			parent.parent = panel->name();
			parent.order = 1;
			butNewGame->value<GuiButtonComponent>();
			GuiTextComponent &txt = butNewGame->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/mainmenu/newgame");
			butNewGame->value<GuiTextFormatComponent>().color = RedPillColor;
		}

		{
			Entity *butOptions = ents->create(104);
			GuiParentComponent &parent = butOptions->value<GuiParentComponent>();
			parent.parent = panel->name();
			parent.order = 2;
			butOptions->value<GuiButtonComponent>();
			GuiTextComponent &txt = butOptions->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/mainmenu/options");
		}

		if ((pathType("score.ini") & PathTypeFlags::File) == PathTypeFlags::File)
		{
			Entity *butScores = ents->create(105);
			GuiParentComponent &parent = butScores->value<GuiParentComponent>();
			parent.parent = panel->name();
			parent.order = 3;
			butScores->value<GuiButtonComponent>();
			GuiTextComponent &txt = butScores->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/mainmenu/scores");
		}

		if (achievements.acquired > 0)
		{
			Entity *butAchivs = ents->create(108);
			GuiParentComponent &parent = butAchivs->value<GuiParentComponent>();
			parent.parent = panel->name();
			parent.order = 4;
			butAchivs->value<GuiButtonComponent>();
			GuiTextComponent &txt = butAchivs->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/mainmenu/achievements");
		}

		{
			Entity *butCredits = ents->create(106);
			GuiParentComponent &parent = butCredits->value<GuiParentComponent>();
			parent.parent = panel->name();
			parent.order = 5;
			butCredits->value<GuiButtonComponent>();
			GuiTextComponent &txt = butCredits->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/mainmenu/credits");
		}

		{
			Entity *butQuit = ents->create(107);
			GuiParentComponent &parent = butQuit->value<GuiParentComponent>();
			parent.parent = panel->name();
			parent.order = 6;
			butQuit->value<GuiButtonComponent>();
			GuiTextComponent &txt = butQuit->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/mainmenu/quit");
			butQuit->value<GuiTextFormatComponent>().color = BluePillColor;
		}
	}

	{ // languages
		Entity *column = ents->createUnique();
		{
			column->value<GuiParentComponent>().parent = 10;
			column->value<GuiLayoutLineComponent>().vertical = true;
		}

		static constexpr const uint32 Flags[] = {
			HashString("degrid/languages/english.png"),
			HashString("degrid/languages/czech.png")
		};

		for (uint32 i = 0; i < sizeof(Flags) / sizeof(Flags[0]); i++)
		{
			Entity *but = ents->create(200 + i);
			GuiParentComponent &parent = but->value<GuiParentComponent>();
			parent.parent = column->name();
			parent.order = i;
			but->value<GuiButtonComponent>();
			but->value<GuiImageComponent>().textureName = Flags[i];
			but->value<GuiExplicitSizeComponent>().size = Vec2(80, 40);
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
