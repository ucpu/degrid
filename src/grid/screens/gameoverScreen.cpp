#include "../includes.h"
#include "../screens.h"
#include "../game.h"

namespace
{
	uint32 scoreValue;
	uint32 nameEntity;

	eventListener<bool(uint32)> guiEvent;

	bool buttonSave(uint32 en)
	{
		if (en != 12) return false;

#ifndef GRID_TESTING
		{
			GUI_GET_COMPONENT(text, txt, gui()->entities()->getEntity(nameEntity));
			grid::confPlayerName = txt.value;
			holder<fileClass> f = newFile("score.ini", fileMode(false, true, true, true));
			f->writeLine("[]");
			{
				uint32 y, m, d, h, mm, s;
				getSystemDateTime(y, m, d, h, mm, s);
				f->writeLine(string() + "date = " + formatDateTime(y, m, d, h, mm, s));
			}
			f->writeLine(string() + "name = " + txt.value);
			f->writeLine(string() + "score = " + scoreValue);
		}
#endif

		grid::gameStart(true);
		setScreenScores();
		return true;
	}
}

void setScreenGameover(uint32 score)
{
	scoreValue = score;
	eraseGui();
	generateLogo();
	entityManagerClass *ents = gui()->entities();
	guiEvent.bind<&buttonSave>();
	guiEvent.attach(gui()->genericEvent);

	entityClass *panel = ents->newEntity(ents->generateUniqueName());
	{
		GUI_GET_COMPONENT(control, control, panel);
		control.controlType = controlTypeEnum::Panel;
		GUI_GET_COMPONENT(position, position, panel);
		position.x = 0.3;
		position.y = 0.6;
		position.xUnit = unitsModeEnum::ScreenWidth;
		position.yUnit = unitsModeEnum::ScreenHeight;
		position.anchorX = position.anchorY = 0.5;
	}

	entityClass *column = ents->newEntity(ents->generateUniqueName());
	{
		GUI_GET_COMPONENT(parent, parent, column);
		parent.parent = panel->getName();
		GUI_GET_COMPONENT(layout, layout, column);
		layout.layoutMode = layoutModeEnum::Column;
	}

	{
		entityClass *empOver = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, parent, empOver);
		parent.parent = column->getName();
		parent.ordering = 1;
		GUI_GET_COMPONENT(control, control, empOver);
		control.controlType = controlTypeEnum::Empty;
		GUI_GET_COMPONENT(text, txt, empOver);
		txt.assetName = hashString("grid/languages/internationalized.textpack");
		txt.textName = hashString("gui/gameover/over");
		GUI_GET_COMPONENT(format, format, empOver);
		format.align = textAlignEnum::Center;
		format.color = vec3(1, 0, 0);
		format.fontName = hashString("cage/font/roboto.ttf?30");
		GUI_GET_COMPONENT(position, position, empOver);
		position.anchorX = 0.5;
	}

	{
		entityClass *empScore = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, parent, empScore);
		parent.parent = column->getName();
		parent.ordering = 2;
		GUI_GET_COMPONENT(control, control, empScore);
		control.controlType = controlTypeEnum::Empty;
		GUI_GET_COMPONENT(text, txt, empScore);
		txt.value = string(score);
		GUI_GET_COMPONENT(format, format, empScore);
		format.align = textAlignEnum::Center;
		format.color = vec3(0, 1, 0);
		format.fontName = hashString("cage/font/roboto.ttf?50");
		GUI_GET_COMPONENT(position, position, empScore);
		position.anchorX = 0.5;
	}

	{
		entityClass *txtName = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, parent, txtName);
		parent.parent = column->getName();
		parent.ordering = 3;
		GUI_GET_COMPONENT(control, control, txtName);
		control.controlType = controlTypeEnum::Textbox;
		GUI_GET_COMPONENT(text, txt, txtName);
		txt.value = grid::confPlayerName;
		GUI_GET_COMPONENT(position, position, txtName);
		position.anchorX = 0.5;
		nameEntity = txtName->getName();
	}

	{
		entityClass *butSave = ents->newEntity(12);
		GUI_GET_COMPONENT(control, control, butSave);
		control.controlType = controlTypeEnum::Button;
		GUI_GET_COMPONENT(text, txt, butSave);
		txt.assetName = hashString("grid/languages/internationalized.textpack");
		txt.textName = hashString("gui/gameover/save");
		GUI_GET_COMPONENT(position, position, butSave);
		position.x = 1.0;
		position.y = 1.0;
		position.xUnit = unitsModeEnum::ScreenWidth;
		position.yUnit = unitsModeEnum::ScreenHeight;
		position.anchorX = 1.0;
		position.anchorY = 1.0;
		GUI_GET_COMPONENT(format, format, butSave);
		format.align = textAlignEnum::Center;
	}
}