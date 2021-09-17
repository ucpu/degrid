#include <cage-core/files.h>
#include <cage-core/ini.h>
#include <cage-core/enumerate.h>

#include "screens.h"

#include <vector>
#include <algorithm>

namespace
{
	struct Score
	{
		uint32 score;
		String date;
	};

	struct ScoreComparator
	{
		int mode = -1;
		explicit ScoreComparator(int mode) : mode(mode) {}
		bool operator () (const Score &a, const Score &b) const
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

	InputListener<InputClassEnum::GuiWidget, InputGuiWidget, bool> guiEvent;

	bool guiFunction(InputGuiWidget in)
	{
		switch (in.widget)
		{
		case 51:
		case 52:
			buildGui(in.widget - 51);
			return true;
		}
		return false;
	}

	void buildGui(int mode)
	{
		regenerateGui(GuiConfig());
		EntityManager *ents = engineGuiEntities();
		guiEvent.bind<&guiFunction>();
		guiEvent.attach(engineGuiManager()->widgetEvent);

		Entity *panel = ents->createUnique();
		{
			GuiPanelComponent &panel2 = panel->value<GuiPanelComponent>();
			GuiParentComponent &parent = panel->value<GuiParentComponent>();
			parent.parent = 12;
			GuiLayoutTableComponent &layout = panel->value<GuiLayoutTableComponent>();
			layout.sections = 2;
		}

		{ // header
			{
				Entity *butScore = ents->create(51);
				GuiParentComponent &parent = butScore->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = 1;
				GuiButtonComponent &control = butScore->value<GuiButtonComponent>();
				GuiTextComponent &txt = butScore->value<GuiTextComponent>();
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/scores/date");
			}

			{
				Entity *butDate = ents->create(52);
				GuiParentComponent &parent = butDate->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = 2;
				GuiButtonComponent &control = butDate->value<GuiButtonComponent>();
				GuiTextComponent &txt = butDate->value<GuiTextComponent>();
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/scores/score");
			}
		}

		std::sort(scores.begin(), scores.end(), ScoreComparator(mode));
		for (auto it : enumerate(scores))
		{
			{
				Entity *txtScore = ents->createUnique();
				GuiParentComponent &parent = txtScore->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = numeric_cast<sint32>(it.index) * 2 + 100;
				GuiLabelComponent &control = txtScore->value<GuiLabelComponent>();
				GuiTextComponent &txt = txtScore->value<GuiTextComponent>();
				txt.value = it->date;
				GuiTextFormatComponent &format = txtScore->value<GuiTextFormatComponent>();
				format.align = TextAlignEnum::Center;
			}

			{
				Entity *txtDate = ents->createUnique();
				GuiParentComponent &parent = txtDate->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = numeric_cast<sint32>(it.index * 2 + 101);
				GuiLabelComponent &control = txtDate->value<GuiLabelComponent>();
				GuiTextComponent &txt = txtDate->value<GuiTextComponent>();
				txt.value = Stringizer() + it->score;
				GuiTextFormatComponent &format = txtDate->value<GuiTextFormatComponent>();
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
			s.score = ini->getUint32(Stringizer() + i, "score");
			s.date = ini->getString(Stringizer() + i, "date");
			scores.push_back(s);
		}
	}

	guiFunction(InputGuiWidget{ nullptr, 52 });
}
