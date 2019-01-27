#include "screens.h"
#include "../game.h"

#include <cage-core/filesystem.h>
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
#ifndef GRID_TESTING
		holder<fileClass> f = newFile("score.ini", fileMode(false, true, true, true));
		f->writeLine("[]");
		{
			uint32 y, m, d, h, mm, s;
			detail::getSystemDateTime(y, m, d, h, mm, s);
			f->writeLine(string() + "date = " + detail::formatDateTime(y, m, d, h, mm, s));
		}
		f->writeLine(string() + "bosses = " + game.defeatedBosses + " / " + achievements.bosses);
		f->writeLine(string() + "achievements = " + achievements.acquired);
		f->writeLine(string() + "duration = " + statistics.updateIteration);
		f->writeLine(string() + "score = " + game.score);
#endif
	}

	{
		guiConfig c;
		c.backButton = false;
		regenerateGui(c);
	}
	entityManagerClass *ents = gui()->entities();
	guiEvent.bind<&buttonContinue>();
	guiEvent.attach(gui()->widgetEvent);
	keyReleaseListener.attach(window()->events.keyRelease);
	keyReleaseListener.bind<&keyRelease>();

	entityClass *panel = ents->createUnique();
	{
		GUI_GET_COMPONENT(panel, panel2, panel);
		GUI_GET_COMPONENT(parent, parent, panel);
		parent.parent = 12;
		GUI_GET_COMPONENT(layoutLine, layout, panel);
		layout.vertical = true;
	}

	{
		entityClass *empOver = ents->createUnique();
		GUI_GET_COMPONENT(parent, parent, empOver);
		parent.parent = panel->name();
		parent.order = 1;
		GUI_GET_COMPONENT(label, control, empOver);
		GUI_GET_COMPONENT(text, txt, empOver);
		txt.assetName = hashString("grid/languages/internationalized.textpack");
		txt.textName = hashString("gui/gameover/over");
		GUI_GET_COMPONENT(textFormat, format, empOver);
		format.align = textAlignEnum::Center;
		format.color = vec3(1, 0, 0);
		format.size = 30;
	}

	{
		entityClass *empScore = ents->createUnique();
		GUI_GET_COMPONENT(parent, parent, empScore);
		parent.parent = panel->name();
		parent.order = 2;
		GUI_GET_COMPONENT(label, control, empScore);
		GUI_GET_COMPONENT(text, txt, empScore);
		txt.value = string(game.score);
		GUI_GET_COMPONENT(textFormat, format, empScore);
		format.align = textAlignEnum::Center;
		format.color = vec3(0, 1, 0);
		format.size = 50;
	}

	{
		entityClass *butSave = ents->create(100);
		GUI_GET_COMPONENT(button, control, butSave);
		GUI_GET_COMPONENT(text, txt, butSave);
		txt.assetName = hashString("grid/languages/internationalized.textpack");
		txt.textName = hashString("gui/gameover/continue");
		GUI_GET_COMPONENT(parent, parent, butSave);
		parent.parent = 15;
	}
}
