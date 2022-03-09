#include <cage-core/config.h>
#include <cage-core/timer.h>
#include <cage-core/ini.h>
#include <cage-core/hashString.h>
#include <cage-core/debug.h>

#include "game.h"
#include "screens/screens.h"

#include <unordered_map>
#include <string>
#include <chrono>
#include <ctime>

Achievements achievements;

EventDispatcher<bool(const String&)> &achievementAcquiredEvent()
{
	static EventDispatcher<bool(const String&)> inst;
	return inst;
}

namespace std
{
	template <>
	struct hash<cage::String>
	{
		std::size_t operator()(const cage::String &k) const
		{
			return std::hash<std::string>()(std::string(k.c_str()));
		}
	};
}

namespace
{
	struct Achievement
	{
		String date;
		bool boss = false;
	};

	std::unordered_map<String, Achievement> data;
}

bool achievementFullfilled(const String &name, bool bossKill)
{
	if (game.cinematic)
		return false;

	CAGE_LOG_DEBUG(SeverityEnum::Info, "achievements", Stringizer() + "fulfilled achievement: '" + name + "', boss: " + bossKill);

	{
		auto it = data.find(name);
		if (it != data.end())
		{
			CAGE_ASSERT(it->second.boss == bossKill);
			return false;
		}
	}

	Achievement &a = data[name];
	{ // date
		const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		char buffer[50];
		std::strftime(buffer, 50, "%Y-%m-%d %H:%M:%S", std::localtime(&now));
		a.date = buffer;
	}
	a.boss = bossKill;

	if (bossKill)
		achievements.bosses++;
	achievements.acquired++;

	CAGE_LOG(SeverityEnum::Info, "achievements", String() + "acquired achievement: '" + name + "'");

	makeAnnouncement(
		HashString((String() + "achievement/" + name).c_str()),
		HashString((String() + "achievement-desc/" + name).c_str())
	);

	return true;
}

namespace
{
	void summaries(uint32 &achievements, uint32 &bosses)
	{
		achievements = numeric_cast<uint32>(data.size());
		bosses = 0;
		for (const auto &a : data)
			if (a.second.boss)
				bosses++;
	}

	void engineInit()
	{
		// load achievements
		try
		{
			Holder<Ini> ini = newIni();
			{
				detail::OverrideBreakpoint ob;
				ini->importFile("achievements.ini");
			}
			for (const String &section : ini->sections())
			{
				Achievement &a = data[section];
				a.date = ini->getString(section, "date");
				a.boss = ini->getBool(section, "boss");
			}
		}
		catch (...)
		{
			CAGE_LOG(SeverityEnum::Warning, "achievements", "failed to load achievements");
			data.clear();
		}
		summaries(achievements.acquired, achievements.bosses);
		CAGE_LOG(SeverityEnum::Info, "achievements", Stringizer() + "acquired achievements: " + achievements.acquired + ", bosses: " + achievements.bosses);
		for (const auto &a : data)
			CAGE_LOG_CONTINUE(SeverityEnum::Note, "achievements", Stringizer() + a.first + ": " + a.second.date + " (" + a.second.boss + ")");
		if (achievements.bosses > BossesTotalCount)
		{
			CAGE_LOG(SeverityEnum::Warning, "achievements", "are you cheating? there is not that many bosses in the game");
		}
	}

	void engineFinish()
	{
		{ // validation
			uint32 a, b;
			summaries(a, b);
			CAGE_ASSERT(a == achievements.acquired);
			CAGE_ASSERT(b == achievements.bosses);
		}

		{ // save achievements
#ifndef DEGRID_TESTING
			Holder<Ini> ini = newIni();
			for (const auto &a : data)
			{
				ini->setString(a.first, "date", a.second.date);
				ini->setBool(a.first, "boss", a.second.boss);
			}
			try
			{
				ini->exportFile("achievements.ini");
			}
			catch (...)
			{
				CAGE_LOG(SeverityEnum::Warning, "achievements", "failed to save achievements");
			}
#endif // !DEGRID_TESTING
		}
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineFinishListener;
	public:
		Callbacks()
		{
			engineInitListener.attach(controlThread().initialize, -60);
			engineInitListener.bind<&engineInit>();
			engineFinishListener.attach(controlThread().finalize, 60);
			engineFinishListener.bind<&engineFinish>();
		}
	} callbacksInstance;
}

void setScreenAchievements()
{
	regenerateGui(GuiConfig());
	EntityManager *ents = engineGuiEntities();

	{
		GuiLayoutLineComponent &layout = ents->get(12)->value<GuiLayoutLineComponent>();
		layout.vertical = true;
	}

	uint32 index = 0;
	for (const auto &it : data)
	{
		uint32 panelName;
		{ // panel
			Entity *e = ents->createUnique();
			panelName = e->name();
			GuiParentComponent &parent = e->value<GuiParentComponent>();
			parent.parent = 12;
			parent.order = index++;
			GuiPanelComponent &panel = e->value<GuiPanelComponent>();
			GuiTextComponent &txt = e->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString((String() + "achievement/" + it.first).c_str());
			GuiLayoutLineComponent &layout = e->value<GuiLayoutLineComponent>();
			layout.vertical = true;
		}

		{ // description
			Entity *e = ents->createUnique();
			GuiParentComponent &parent = e->value<GuiParentComponent>();
			parent.parent = panelName;
			parent.order = 1;
			GuiLabelComponent &label = e->value<GuiLabelComponent>();
			GuiTextComponent &txt = e->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString((String() + "achievement-desc/" + it.first).c_str());
		}

		{ // date
			Entity *e = ents->createUnique();
			GuiParentComponent &parent = e->value<GuiParentComponent>();
			parent.parent = panelName;
			parent.order = 2;
			GuiLabelComponent &label = e->value<GuiLabelComponent>();
			GuiTextComponent &txt = e->value<GuiTextComponent>();
			txt.value = it.second.date;
			GuiTextFormatComponent &format = e->value<GuiTextFormatComponent>();
			format.align = TextAlignEnum::Right;
		}
	}
}
