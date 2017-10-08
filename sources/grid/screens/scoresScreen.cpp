#include <vector>
#include <algorithm>

#include "../includes.h"
#include "../screens.h"

namespace
{
	struct scoreStruct
	{
		uint32 score;
		string date;
		string name;
	};

	struct scoreComparatorStruct
	{
		int mode;
		scoreComparatorStruct(int mode) : mode(mode) {}
		const bool operator () (const scoreStruct &a, const scoreStruct &b) const
		{
			switch (mode)
			{
			case 0: return a.score > b.score;
			case 1: return a.date > b.date;
			case 2: return a.name < b.name;
			default: CAGE_THROW_CRITICAL(exception, "invalid comparison mode");
			}
		}
	};

	std::vector<scoreStruct> scores;

	void buildGui(int mode);

	eventListener<bool(uint32)> guiEvent;

	bool guiFunction(uint32 en)
	{
		switch (en)
		{
		case 51:
		case 52:
		case 53:
			buildGui(en - 51);
			return true;
		}
		return false;
	}

	void buildGui(int mode)
	{
		eraseGui();
		generateLogo();
		generateButtonBack();
		entityManagerClass *ents = gui()->entities();
		guiEvent.bind<&guiFunction>();
		guiEvent.attach(gui()->genericEvent);

		entityClass *panel = ents->newEntity(ents->generateUniqueName());
		{
			GUI_GET_COMPONENT(control, control, panel);
			control.controlType = controlTypeEnum::Panel;
			GUI_GET_COMPONENT(position, position, panel);
			position.x = 0.005;
			position.y = 0.005;
			position.h = 0.99;
			position.xUnit = unitsModeEnum::ScreenHeight;
			position.yUnit = unitsModeEnum::ScreenHeight;
			position.hUnit = unitsModeEnum::ScreenHeight;
		}

		entityClass *column = ents->newEntity(ents->generateUniqueName());
		{
			GUI_GET_COMPONENT(parent, parent, column);
			parent.parent = panel->getName();
			GUI_GET_COMPONENT(layout, layout, column);
			layout.layoutMode = layoutModeEnum::Column;
		}

		{
			entityClass *row = ents->newEntity(ents->generateUniqueName());
			{
				GUI_GET_COMPONENT(parent, parent, row);
				parent.parent = column->getName();
				parent.ordering = -1;
				GUI_GET_COMPONENT(layout, layout, row);
				layout.layoutMode = layoutModeEnum::Row;
			}

			{
				entityClass *butScore = ents->newEntity(51);
				GUI_GET_COMPONENT(parent, parent, butScore);
				parent.parent = row->getName();
				parent.ordering = 1;
				GUI_GET_COMPONENT(control, control, butScore);
				control.controlType = controlTypeEnum::Button;
				GUI_GET_COMPONENT(text, txt, butScore);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/scores/score");
				GUI_GET_COMPONENT(format, format, butScore);
				format.align = textAlignEnum::Center;
			}

			{
				entityClass *butDate = ents->newEntity(52);
				GUI_GET_COMPONENT(parent, parent, butDate);
				parent.parent = row->getName();
				parent.ordering = 2;
				GUI_GET_COMPONENT(control, control, butDate);
				control.controlType = controlTypeEnum::Button;
				GUI_GET_COMPONENT(text, txt, butDate);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/scores/date");
				GUI_GET_COMPONENT(format, format, butDate);
				format.align = textAlignEnum::Center;
			}

			{
				entityClass *butName = ents->newEntity(53);
				GUI_GET_COMPONENT(parent, parent, butName);
				parent.parent = row->getName();
				parent.ordering = 3;
				GUI_GET_COMPONENT(control, control, butName);
				control.controlType = controlTypeEnum::Button;
				GUI_GET_COMPONENT(text, txt, butName);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/scores/name");
				GUI_GET_COMPONENT(format, format, butName);
				format.align = textAlignEnum::Center;
			}
		}

		real txtSize = gui()->elements[(int)elementTypeEnum::ButtonNormal].defaultSize[0];
		std::sort(scores.begin(), scores.end(), scoreComparatorStruct(mode));
		for (auto it = scores.begin(), et = scores.end(); it != et; it++)
		{
			entityClass *row = ents->newEntity(ents->generateUniqueName());
			{
				GUI_GET_COMPONENT(parent, parent, row);
				parent.parent = column->getName();
				parent.ordering = numeric_cast<sint32>(it - scores.begin());
				GUI_GET_COMPONENT(layout, layout, row);
				layout.layoutMode = layoutModeEnum::Row;
			}

			{
				entityClass *txtScore = ents->newEntity(ents->generateUniqueName());
				GUI_GET_COMPONENT(parent, parent, txtScore);
				parent.parent = row->getName();
				parent.ordering = 1;
				GUI_GET_COMPONENT(control, control, txtScore);
				control.controlType = controlTypeEnum::Empty;
				GUI_GET_COMPONENT(text, txt, txtScore);
				txt.value = it->score;
				GUI_GET_COMPONENT(position, pos, txtScore);
				pos.w = txtSize;
				pos.wUnit = unitsModeEnum::Pixels;
				GUI_GET_COMPONENT(format, format, txtScore);
				format.align = textAlignEnum::Right;
			}

			{
				entityClass *txtDate = ents->newEntity(ents->generateUniqueName());
				GUI_GET_COMPONENT(parent, parent, txtDate);
				parent.parent = row->getName();
				parent.ordering = 2;
				GUI_GET_COMPONENT(control, control, txtDate);
				control.controlType = controlTypeEnum::Empty;
				GUI_GET_COMPONENT(text, txt, txtDate);
				txt.value = it->date;
				GUI_GET_COMPONENT(position, pos, txtDate);
				pos.w = txtSize;
				pos.wUnit = unitsModeEnum::Pixels;
				GUI_GET_COMPONENT(format, format, txtDate);
				format.align = textAlignEnum::Center;
			}

			{
				entityClass *txtName = ents->newEntity(ents->generateUniqueName());
				GUI_GET_COMPONENT(parent, parent, txtName);
				parent.parent = row->getName();
				parent.ordering = 3;
				GUI_GET_COMPONENT(control, control, txtName);
				control.controlType = controlTypeEnum::Empty;
				GUI_GET_COMPONENT(text, txt, txtName);
				txt.value = it->name;
				GUI_GET_COMPONENT(position, pos, txtName);
				pos.w = txtSize;
				pos.wUnit = unitsModeEnum::Pixels;
				GUI_GET_COMPONENT(format, format, txtName);
				format.align = textAlignEnum::Left;
			}
		}
	}
}

void setScreenScores()
{
	scores.clear();
	scores.reserve(100);

	if (newFilesystem()->exists("score.ini"))
	{
		holder<iniClass> ini = newIni();
		ini->load("score.ini");
		for (uint32 i = 0, e = ini->sectionCount(); i < e; i++)
		{
			scoreStruct s;
			s.score = ini->getUint32(i, "score");
			s.date = ini->getString(i, "date");
			s.name = ini->getString(i, "name");
			scores.push_back(s);
		}
	}

	guiFunction(51);
}