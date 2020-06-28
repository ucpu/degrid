#include "screens.h"

#include <cage-core/files.h>
#include <cage-core/ini.h>
#include <cage-core/enumerate.h>

#include <vector>
#include <algorithm>

namespace
{
	struct Score
	{
		uint32 score;
		string date;
	};

	struct ScoreComparator
	{
		int mode;
		ScoreComparator(int mode) : mode(mode) {}
		const bool operator () (const Score &a, const Score &b) const
		{
			switch (mode)
			{
			case 0: return a.date > b.date;
			case 1: return a.score > b.score;
			default: CAGE_THROW_CRITICAL(Exception, "invalid comparison mode");
			}
		}
	};

	std::vector<Score> scores;

	void buildGui(int mode);

	EventListener<bool(uint32)> guiEvent;

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
		regenerateGui(GuiConfig());
		EntityManager *ents = engineGui()->entities();
		guiEvent.bind<&guiFunction>();
		guiEvent.attach(engineGui()->widgetEvent);

		Entity *panel = ents->createUnique();
		{
			CAGE_COMPONENT_GUI(Panel, panel2, panel);
			CAGE_COMPONENT_GUI(Parent, parent, panel);
			parent.parent = 12;
			CAGE_COMPONENT_GUI(LayoutTable, layout, panel);
			layout.sections = 2;
		}

		{ // header
			{
				Entity *butScore = ents->create(51);
				CAGE_COMPONENT_GUI(Parent, parent, butScore);
				parent.parent = panel->name();
				parent.order = 1;
				CAGE_COMPONENT_GUI(Button, control, butScore);
				CAGE_COMPONENT_GUI(Text, txt, butScore);
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/scores/date");
			}

			{
				Entity *butDate = ents->create(52);
				CAGE_COMPONENT_GUI(Parent, parent, butDate);
				parent.parent = panel->name();
				parent.order = 2;
				CAGE_COMPONENT_GUI(Button, control, butDate);
				CAGE_COMPONENT_GUI(Text, txt, butDate);
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/scores/score");
			}
		}

		std::sort(scores.begin(), scores.end(), ScoreComparator(mode));
		for (auto it : enumerate(scores))
		{
			{
				Entity *txtScore = ents->createUnique();
				CAGE_COMPONENT_GUI(Parent, parent, txtScore);
				parent.parent = panel->name();
				parent.order = numeric_cast<sint32>(it.cnt) * 2 + 100;
				CAGE_COMPONENT_GUI(Label, control, txtScore);
				CAGE_COMPONENT_GUI(Text, txt, txtScore);
				txt.value = it->date;
				CAGE_COMPONENT_GUI(TextFormat, format, txtScore);
				format.align = TextAlignEnum::Center;
			}

			{
				Entity *txtDate = ents->createUnique();
				CAGE_COMPONENT_GUI(Parent, parent, txtDate);
				parent.parent = panel->name();
				parent.order = numeric_cast<sint32>(it.cnt * 2 + 101);
				CAGE_COMPONENT_GUI(Label, control, txtDate);
				CAGE_COMPONENT_GUI(Text, txt, txtDate);
				txt.value = stringizer() + it->score;
				CAGE_COMPONENT_GUI(TextFormat, format, txtDate);
				format.align = TextAlignEnum::Right;
			}
		}
	}
}

void setScreenScores()
{
	scores.clear();
	scores.reserve(100);

	if ((pathType("score.ini") & PathTypeFlags::File) == PathTypeFlags::File)
	{
		Holder<Ini> ini = newIni();
		ini->importFile("score.ini");
		for (uint32 i = 0, e = ini->sectionsCount(); i < e; i++)
		{
			Score s;
			s.score = ini->getUint32(stringizer() + i, "score");
			s.date = ini->getString(stringizer() + i, "date");
			scores.push_back(s);
		}
	}

	guiFunction(52);
}
