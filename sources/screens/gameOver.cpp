#include "screens.h"
#include "../game.h"

#include <cage-core/files.h>
#include <cage-core/timer.h> // getSystemDateTime

namespace
{
	EventListener<bool(uint32)> guiEvent;
	EventListener<bool(uint32, uint32, ModifiersFlags)> keyReleaseListener;

	void endScreen()
	{
		keyReleaseListener.detach();
		game.cinematic = true;
		bool showScore = game.score > 0;
		gameStartEvent().dispatch();
		if (showScore)
			setScreenScores();
		else
			setScreenMainmenu();
	}

	bool keyRelease(uint32 key, uint32, ModifiersFlags modifiers)
	{
		if (key == 256) // esc
		{
			endScreen();
			return true;
		}
		return false;
	}

	bool buttonContinue(uint32 en)
	{
		if (en != 100)
			return false;
		endScreen();
		return true;
	}
}

void setScreenGameover()
{
	if (game.score > 0)
	{
#ifndef DEGRID_TESTING
		FileMode fm(false, true);
		fm.textual = true;
		fm.append = true;
		Holder<File> f = newFile("score.ini", fm);
		f->writeLine("[]");
		{
			uint32 y, m, d, h, mm, s;
			detail::getSystemDateTime(y, m, d, h, mm, s);
			f->writeLine(stringizer() + "date = " + detail::formatDateTime(y, m, d, h, mm, s));
		}
		f->writeLine(stringizer() + "bosses = " + game.defeatedBosses + " / " + achievements.bosses);
		f->writeLine(stringizer() + "achievements = " + achievements.acquired);
		f->writeLine(stringizer() + "duration = " + statistics.updateIteration);
		f->writeLine(stringizer() + "score = " + game.score);
#endif
	}

	{
		GuiConfig c;
		c.backButton = false;
		regenerateGui(c);
	}
	EntityManager *ents = engineGui()->entities();
	guiEvent.bind<&buttonContinue>();
	guiEvent.attach(engineGui()->widgetEvent);
	keyReleaseListener.attach(engineWindow()->events.keyRelease);
	keyReleaseListener.bind<&keyRelease>();

	Entity *panel = ents->createUnique();
	{
		CAGE_COMPONENT_GUI(Panel, panel2, panel);
		CAGE_COMPONENT_GUI(Parent, parent, panel);
		parent.parent = 12;
		CAGE_COMPONENT_GUI(LayoutLine, layout, panel);
		layout.vertical = true;
	}

	{
		Entity *empOver = ents->createUnique();
		CAGE_COMPONENT_GUI(Parent, parent, empOver);
		parent.parent = panel->name();
		parent.order = 1;
		CAGE_COMPONENT_GUI(Label, control, empOver);
		CAGE_COMPONENT_GUI(Text, txt, empOver);
		txt.assetName = HashString("degrid/languages/internationalized.textpack");
		txt.textName = HashString("gui/gameover/over");
		CAGE_COMPONENT_GUI(TextFormat, format, empOver);
		format.align = TextAlignEnum::Center;
		format.color = vec3(1, 0, 0);
		format.size = 30;
	}

	{
		Entity *empScore = ents->createUnique();
		CAGE_COMPONENT_GUI(Parent, parent, empScore);
		parent.parent = panel->name();
		parent.order = 2;
		CAGE_COMPONENT_GUI(Label, control, empScore);
		CAGE_COMPONENT_GUI(Text, txt, empScore);
		txt.value = string(game.score);
		CAGE_COMPONENT_GUI(TextFormat, format, empScore);
		format.align = TextAlignEnum::Center;
		format.color = vec3(0, 1, 0);
		format.size = 50;
	}

	{
		Entity *butSave = ents->create(100);
		CAGE_COMPONENT_GUI(Button, control, butSave);
		CAGE_COMPONENT_GUI(Text, txt, butSave);
		txt.assetName = HashString("degrid/languages/internationalized.textpack");
		txt.textName = HashString("gui/gameover/continue");
		CAGE_COMPONENT_GUI(Parent, parent, butSave);
		parent.parent = 15;
	}
}
