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
			GUI_GET_COMPONENT(inputBox, txt, gui()->entities()->getEntity(nameEntity));
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
	guiEvent.attach(gui()->widgetEvent);

	entityClass *panel = ents->newEntity(ents->generateUniqueName());
	{
		GUI_GET_COMPONENT(groupBox, groupBox, panel);
		groupBox.type = groupBoxTypeEnum::Panel;
		GUI_GET_COMPONENT(position, position, panel);
		position.anchor = vec2(0.5, 0.5);
		position.position.values[0] = 0.3;
		position.position.units[0] = unitEnum::ScreenWidth;
		position.position.values[1] = 0.6;
		position.position.units[1] = unitEnum::ScreenHeight;
		GUI_GET_COMPONENT(layoutLine, layout, panel);
		layout.vertical = true;
		layout.expandToSameWidth = true;
	}

	{
		entityClass *empOver = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, parent, empOver);
		parent.parent = panel->getName();
		parent.order = 1;
		GUI_GET_COMPONENT(label, control, empOver);
		GUI_GET_COMPONENT(text, txt, empOver);
		txt.assetName = hashString("grid/languages/internationalized.textpack");
		txt.textName = hashString("gui/gameover/over");
		GUI_GET_COMPONENT(textFormat, format, empOver);
		format.align = textAlignEnum::Center;
		format.color = vec3(1, 0, 0);
		format.fontName = hashString("cage/font/roboto.ttf?30");
	}

	{
		entityClass *empScore = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, parent, empScore);
		parent.parent = panel->getName();
		parent.order = 2;
		GUI_GET_COMPONENT(label, control, empScore);
		GUI_GET_COMPONENT(text, txt, empScore);
		txt.value = string(score);
		GUI_GET_COMPONENT(textFormat, format, empScore);
		format.align = textAlignEnum::Center;
		format.color = vec3(0, 1, 0);
		format.fontName = hashString("cage/font/roboto.ttf?50");
	}

	{
		entityClass *txtName = ents->newEntity(ents->generateUniqueName());
		GUI_GET_COMPONENT(parent, parent, txtName);
		parent.parent = panel->getName();
		parent.order = 3;
		GUI_GET_COMPONENT(inputBox, control, txtName);
		control.value = grid::confPlayerName;
		//GUI_GET_COMPONENT(text, txt, txtName);
		nameEntity = txtName->getName();
	}

	{
		entityClass *butSave = ents->newEntity(12);
		GUI_GET_COMPONENT(button, control, butSave);
		GUI_GET_COMPONENT(text, txt, butSave);
		txt.assetName = hashString("grid/languages/internationalized.textpack");
		txt.textName = hashString("gui/gameover/save");
		GUI_GET_COMPONENT(position, position, butSave);
		position.anchor = vec2(1, 1);
		position.position.values[0] = 1;
		position.position.units[0] = unitEnum::ScreenWidth;
		position.position.values[1] = 1;
		position.position.units[1] = unitEnum::ScreenHeight;
	}
}