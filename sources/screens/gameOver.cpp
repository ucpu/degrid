#include "screens.h"
#include "../game.h"

#include <cage-core/files.h>
#include <cage-core/timer.h> // getSystemDateTime

namespace
{
	eventListener<bool(uint32)> guiEvent;
	eventListener<bool(uint32, uint32, modifiersFlags)> keyReleaseListener;

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

	bool keyRelease(uint32 key, uint32, modifiersFlags modifiers)
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
		fileMode fm(false, true);
		fm.textual = true;
		fm.append = true;
		holder<fileHandle> f = newFile("score.ini", fm);
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
		guiConfig c;
		c.backButton = false;
		regenerateGui(c);
	}
	entityManager *ents = gui()->entities();
	guiEvent.bind<&buttonContinue>();
	guiEvent.attach(gui()->widgetEvent);
	keyReleaseListener.attach(window()->events.keyRelease);
	keyReleaseListener.bind<&keyRelease>();

	entity *panel = ents->createUnique();
	{
		CAGE_COMPONENT_GUI(panel, panel2, panel);
		CAGE_COMPONENT_GUI(parent, parent, panel);
		parent.parent = 12;
		CAGE_COMPONENT_GUI(layoutLine, layout, panel);
		layout.vertical = true;
	}

	{
		entity *empOver = ents->createUnique();
		CAGE_COMPONENT_GUI(parent, parent, empOver);
		parent.parent = panel->name();
		parent.order = 1;
		CAGE_COMPONENT_GUI(label, control, empOver);
		CAGE_COMPONENT_GUI(text, txt, empOver);
		txt.assetName = hashString("degrid/languages/internationalized.textpack");
		txt.textName = hashString("gui/gameover/over");
		CAGE_COMPONENT_GUI(textFormat, format, empOver);
		format.align = textAlignEnum::Center;
		format.color = vec3(1, 0, 0);
		format.size = 30;
	}

	{
		entity *empScore = ents->createUnique();
		CAGE_COMPONENT_GUI(parent, parent, empScore);
		parent.parent = panel->name();
		parent.order = 2;
		CAGE_COMPONENT_GUI(label, control, empScore);
		CAGE_COMPONENT_GUI(text, txt, empScore);
		txt.value = string(game.score);
		CAGE_COMPONENT_GUI(textFormat, format, empScore);
		format.align = textAlignEnum::Center;
		format.color = vec3(0, 1, 0);
		format.size = 50;
	}

	{
		entity *butSave = ents->create(100);
		CAGE_COMPONENT_GUI(button, control, butSave);
		CAGE_COMPONENT_GUI(text, txt, butSave);
		txt.assetName = hashString("degrid/languages/internationalized.textpack");
		txt.textName = hashString("gui/gameover/continue");
		CAGE_COMPONENT_GUI(parent, parent, butSave);
		parent.parent = 15;
	}
}
