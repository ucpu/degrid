#include <cage-core/files.h>
#include <cage-core/timer.h> // getSystemDateTime

#include "screens.h"
#include "../game.h"

#include <chrono>
#include <ctime>

namespace
{
	InputListener<InputClassEnum::GuiWidget, InputGuiWidget, bool> guiEvent;
	InputListener<InputClassEnum::KeyRelease, InputKey, bool> keyReleaseListener;

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

	bool keyRelease(InputKey in)
	{
		if (in.key == 256) // esc
		{
			endScreen();
			return true;
		}
		return false;
	}

	bool buttonContinue(InputGuiWidget in)
	{
		if (in.widget != 100)
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
	guiEvent.bind<&buttonContinue>();
	guiEvent.attach(engineGuiManager()->widgetEvent);
	keyReleaseListener.attach(engineWindow()->events);
	keyReleaseListener.bind<&keyRelease>();

	Entity *panel = ents->createUnique();
	{
		GuiPanelComponent &panel2 = panel->value<GuiPanelComponent>();
		GuiParentComponent &parent = panel->value<GuiParentComponent>();
		parent.parent = 12;
		GuiLayoutLineComponent &layout = panel->value<GuiLayoutLineComponent>();
		layout.vertical = true;
	}

	{
		Entity *empOver = ents->createUnique();
		GuiParentComponent &parent = empOver->value<GuiParentComponent>();
		parent.parent = panel->name();
		parent.order = 1;
		GuiLabelComponent &control = empOver->value<GuiLabelComponent>();
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
		GuiLabelComponent &control = empScore->value<GuiLabelComponent>();
		GuiTextComponent &txt = empScore->value<GuiTextComponent>();
		txt.value = Stringizer() + game.score;
		GuiTextFormatComponent &format = empScore->value<GuiTextFormatComponent>();
		format.align = TextAlignEnum::Center;
		format.color = Vec3(0, 1, 0);
		format.size = 50;
	}

	{
		Entity *butSave = ents->create(100);
		GuiButtonComponent &control = butSave->value<GuiButtonComponent>();
		GuiTextComponent &txt = butSave->value<GuiTextComponent>();
		txt.assetName = HashString("degrid/languages/internationalized.textpack");
		txt.textName = HashString("gui/gameover/continue");
		GuiParentComponent &parent = butSave->value<GuiParentComponent>();
		parent.parent = 15;
	}
}
