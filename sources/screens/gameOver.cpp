#include <cage-core/files.h>
#include <cage-core/timer.h> // getSystemDateTime

#include "screens.h"
#include "../game.h"

#include <chrono>
#include <ctime>

namespace
{
	EventListener<bool(const GenericInput &)> keyReleaseListener;

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
			const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			char buffer[50];
			std::strftime(buffer, 50, "%Y-%m-%d %H:%M:%S", std::localtime(&now));
			f->writeLine(Stringizer() + "date = " + buffer);
		}
		f->writeLine(Stringizer() + "bosses = " + game.defeatedBosses + " / " + achievements.bosses);
		f->writeLine(Stringizer() + "achievements = " + achievements.acquired);
		f->writeLine(Stringizer() + "duration = " + statistics.updateIteration);
		f->writeLine(Stringizer() + "score = " + game.score);
#endif
	}

	{
		GuiConfig c;
		c.backButton = false;
		regenerateGui(c);
	}
	EntityManager *ents = engineGuiEntities();
	keyReleaseListener.attach(engineWindow()->events);
	keyReleaseListener.bind(inputListener<InputClassEnum::KeyRelease, InputKey>([](InputKey in) {
		if (in.key == 256) // esc
		{
			endScreen();
			return true;
		}
		return false;
	}));

	Entity *panel = ents->createUnique();
	{
		panel->value<GuiPanelComponent>();
		panel->value<GuiParentComponent>().parent = 12;
		panel->value<GuiLayoutLineComponent>().vertical = true;
	}

	{
		Entity *empOver = ents->createUnique();
		GuiParentComponent &parent = empOver->value<GuiParentComponent>();
		parent.parent = panel->name();
		parent.order = 1;
		empOver->value<GuiLabelComponent>();
		GuiTextComponent &txt = empOver->value<GuiTextComponent>();
		txt.assetName = HashString("degrid/languages/internationalized.textpack");
		txt.textName = HashString("gui/gameover/over");
		GuiTextFormatComponent &format = empOver->value<GuiTextFormatComponent>();
		format.align = TextAlignEnum::Center;
		format.color = Vec3(1, 0, 0);
		format.size = 30;
	}

	{
		Entity *empScore = ents->createUnique();
		GuiParentComponent &parent = empScore->value<GuiParentComponent>();
		parent.parent = panel->name();
		parent.order = 2;
		empScore->value<GuiLabelComponent>();
		empScore->value<GuiTextComponent>().value = Stringizer() + game.score;
		GuiTextFormatComponent &format = empScore->value<GuiTextFormatComponent>();
		format.align = TextAlignEnum::Center;
		format.color = Vec3(0, 1, 0);
		format.size = 50;
	}

	{
		Entity *butSave = ents->create(100);
		butSave->value<GuiButtonComponent>();
		GuiTextComponent &txt = butSave->value<GuiTextComponent>();
		txt.assetName = HashString("degrid/languages/internationalized.textpack");
		txt.textName = HashString("gui/gameover/continue");
		butSave->value<GuiParentComponent>().parent = 15;
		butSave->value<GuiEventComponent>().event.bind([](Entity *e) { endScreen(); return true; });
	}
}
