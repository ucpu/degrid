#include <vector>
#include <algorithm>

#include "screens.h"

#include <cage-core/filesystem.h>
#include <cage-core/ini.h>

namespace
{
	struct scoreStruct
	{
		uint32 score;
		string date;
	};

	struct scoreComparatorStruct
	{
		int mode;
		scoreComparatorStruct(int mode) : mode(mode) {}
		const bool operator () (const scoreStruct &a, const scoreStruct &b) const
		{
			switch (mode)
			{
			case 0: return a.date > b.date;
			case 1: return a.score > b.score;
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
			buildGui(en - 51);
			return true;
		}
		return false;
	}

	void buildGui(int mode)
	{
		regenerateGui(guiConfig());
		entityManagerClass *ents = gui()->entities();
		guiEvent.bind<&guiFunction>();
		guiEvent.attach(gui()->widgetEvent);

		entityClass *panel = ents->createUnique();
		{
			GUI_GET_COMPONENT(panel, panel2, panel);
			GUI_GET_COMPONENT(parent, parent, panel);
			parent.parent = 12;
			GUI_GET_COMPONENT(layoutTable, layout, panel);
			layout.sections = 2;
		}

		{ // header
			{
				entityClass *butScore = ents->create(51);
				GUI_GET_COMPONENT(parent, parent, butScore);
				parent.parent = panel->name();
				parent.order = 1;
				GUI_GET_COMPONENT(button, control, butScore);
				GUI_GET_COMPONENT(text, txt, butScore);
				txt.assetName = hashString("degrid/languages/internationalized.textpack");
				txt.textName = hashString("gui/scores/date");
			}

			{
				entityClass *butDate = ents->create(52);
				GUI_GET_COMPONENT(parent, parent, butDate);
				parent.parent = panel->name();
				parent.order = 2;
				GUI_GET_COMPONENT(button, control, butDate);
				GUI_GET_COMPONENT(text, txt, butDate);
				txt.assetName = hashString("degrid/languages/internationalized.textpack");
				txt.textName = hashString("gui/scores/score");
			}
		}

		uint32 index = 100;
		std::sort(scores.begin(), scores.end(), scoreComparatorStruct(mode));
		for (const auto &it : scores)
		{
			{
				entityClass *txtScore = ents->createUnique();
				GUI_GET_COMPONENT(parent, parent, txtScore);
				parent.parent = panel->name();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, txtScore);
				GUI_GET_COMPONENT(text, txt, txtScore);
				txt.value = it.date;
				GUI_GET_COMPONENT(textFormat, format, txtScore);
				format.align = textAlignEnum::Center;
			}

			{
				entityClass *txtDate = ents->createUnique();
				GUI_GET_COMPONENT(parent, parent, txtDate);
				parent.parent = panel->name();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, txtDate);
				GUI_GET_COMPONENT(text, txt, txtDate);
				txt.value = it.score;
				GUI_GET_COMPONENT(textFormat, format, txtDate);
				format.align = textAlignEnum::Right;
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
			scores.push_back(s);
		}
	}

	guiFunction(52);
}
