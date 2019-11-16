#include <vector>
#include <algorithm>

#include "screens.h"

#include <cage-core/files.h>
#include <cage-core/configIni.h>
#include <cage-core/enumerate.h>

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
		entityManager *ents = gui()->entities();
		guiEvent.bind<&guiFunction>();
		guiEvent.attach(gui()->widgetEvent);

		entity *panel = ents->createUnique();
		{
			CAGE_COMPONENT_GUI(panel, panel2, panel);
			CAGE_COMPONENT_GUI(parent, parent, panel);
			parent.parent = 12;
			CAGE_COMPONENT_GUI(layoutTable, layout, panel);
			layout.sections = 2;
		}

		{ // header
			{
				entity *butScore = ents->create(51);
				CAGE_COMPONENT_GUI(parent, parent, butScore);
				parent.parent = panel->name();
				parent.order = 1;
				CAGE_COMPONENT_GUI(button, control, butScore);
				CAGE_COMPONENT_GUI(text, txt, butScore);
				txt.assetName = hashString("degrid/languages/internationalized.textpack");
				txt.textName = hashString("gui/scores/date");
			}

			{
				entity *butDate = ents->create(52);
				CAGE_COMPONENT_GUI(parent, parent, butDate);
				parent.parent = panel->name();
				parent.order = 2;
				CAGE_COMPONENT_GUI(button, control, butDate);
				CAGE_COMPONENT_GUI(text, txt, butDate);
				txt.assetName = hashString("degrid/languages/internationalized.textpack");
				txt.textName = hashString("gui/scores/score");
			}
		}

		std::sort(scores.begin(), scores.end(), scoreComparatorStruct(mode));
		for (auto it : enumerate(scores))
		{
			{
				entity *txtScore = ents->createUnique();
				CAGE_COMPONENT_GUI(parent, parent, txtScore);
				parent.parent = panel->name();
				parent.order = numeric_cast<sint32>(it.cnt) * 2 + 100;
				CAGE_COMPONENT_GUI(label, control, txtScore);
				CAGE_COMPONENT_GUI(text, txt, txtScore);
				txt.value = it->date;
				CAGE_COMPONENT_GUI(textFormat, format, txtScore);
				format.align = textAlignEnum::Center;
			}

			{
				entity *txtDate = ents->createUnique();
				CAGE_COMPONENT_GUI(parent, parent, txtDate);
				parent.parent = panel->name();
				parent.order = numeric_cast<sint32>(it.cnt * 2 + 101);
				CAGE_COMPONENT_GUI(label, control, txtDate);
				CAGE_COMPONENT_GUI(text, txt, txtDate);
				txt.value = stringizer() + it->score;
				CAGE_COMPONENT_GUI(textFormat, format, txtDate);
				format.align = textAlignEnum::Right;
			}
		}
	}
}

void setScreenScores()
{
	scores.clear();
	scores.reserve(100);

	if ((pathType("score.ini") & pathTypeFlags::File) == pathTypeFlags::File)
	{
		holder<configIni> ini = newConfigIni();
		ini->load("score.ini");
		for (uint32 i = 0, e = ini->sectionsCount(); i < e; i++)
		{
			scoreStruct s;
			s.score = ini->getUint32(stringizer() + i, "score");
			s.date = ini->getString(stringizer() + i, "date");
			scores.push_back(s);
		}
	}

	guiFunction(52);
}
