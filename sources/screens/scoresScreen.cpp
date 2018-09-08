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
		guiEvent.attach(gui()->widgetEvent);

		entityClass *panel = ents->newUniqueEntity();
		{
			GUI_GET_COMPONENT(groupBox, groupBox, panel);
			groupBox.type = groupBoxTypeEnum::Panel;
			GUI_GET_COMPONENT(position, position, panel);
			position.size.values[1] = 1;
			position.size.units[1] = unitEnum::ScreenHeight;
			GUI_GET_COMPONENT(layoutTable, layout, panel);
			layout.sections = 3;
		}

		{ // header
			{
				entityClass *butScore = ents->newEntity(51);
				GUI_GET_COMPONENT(parent, parent, butScore);
				parent.parent = panel->getName();
				parent.order = 1;
				GUI_GET_COMPONENT(button, control, butScore);
				GUI_GET_COMPONENT(text, txt, butScore);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/scores/score");
			}

			{
				entityClass *butDate = ents->newEntity(52);
				GUI_GET_COMPONENT(parent, parent, butDate);
				parent.parent = panel->getName();
				parent.order = 2;
				GUI_GET_COMPONENT(button, control, butDate);
				GUI_GET_COMPONENT(text, txt, butDate);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/scores/date");
			}

			{
				entityClass *butName = ents->newEntity(53);
				GUI_GET_COMPONENT(parent, parent, butName);
				parent.parent = panel->getName();
				parent.order = 3;
				GUI_GET_COMPONENT(button, control, butName);
				GUI_GET_COMPONENT(text, txt, butName);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/scores/name");
			}
		}

		uint32 index = 100;
		std::sort(scores.begin(), scores.end(), scoreComparatorStruct(mode));
		for (auto it = scores.begin(), et = scores.end(); it != et; it++)
		{
			{
				entityClass *txtScore = ents->newUniqueEntity();
				GUI_GET_COMPONENT(parent, parent, txtScore);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, txtScore);
				GUI_GET_COMPONENT(text, txt, txtScore);
				txt.value = it->score;
				GUI_GET_COMPONENT(textFormat, format, txtScore);
				format.align = textAlignEnum::Right;
			}

			{
				entityClass *txtDate = ents->newUniqueEntity();
				GUI_GET_COMPONENT(parent, parent, txtDate);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, txtDate);
				GUI_GET_COMPONENT(text, txt, txtDate);
				txt.value = it->date;
				GUI_GET_COMPONENT(textFormat, format, txtDate);
				format.align = textAlignEnum::Center;
			}

			{
				entityClass *txtName = ents->newUniqueEntity();
				GUI_GET_COMPONENT(parent, parent, txtName);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, txtName);
				GUI_GET_COMPONENT(text, txt, txtName);
				txt.value = it->name;
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
	gui()->skipAllEventsUntilNextUpdate();
}
