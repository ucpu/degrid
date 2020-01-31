#include "game.h"
#include "screens/screens.h"

#include <cage-core/config.h>
#include <cage-core/timer.h>
#include <cage-core/ini.h>
#include <cage-core/hashString.h>
#include <cage-core/debug.h>

#include <unordered_map>

Achievements achievements;

Achievements::Achievements()
{
	detail::memset(this, 0, sizeof(*this));
}

EventDispatcher<bool(const string&)> &achievementAcquiredEvent()
{
	static EventDispatcher<bool(const string&)> inst;
	return inst;
}

namespace std
{
	template <>
	struct hash<cage::string>
	{
		std::size_t operator()(const cage::string &k) const
		{
			return std::hash<std::string>()(std::string(k.c_str()));
		}
	};
}

namespace
{
	struct Achievement
	{
		string date;
		bool boss;

		Achievement() : boss(false)
		{}
	};

	std::unordered_map<string, Achievement> data;
}

bool achievementFullfilled(const string &name, bool bossKill)
{
	if (game.cinematic)
		return false;

	CAGE_LOG_DEBUG(SeverityEnum::Info, "achievements", stringizer() + "fulfilled achievement: '" + name + "', boss: " + bossKill);

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
		uint32 y, M, d, h, m, s;
		detail::getSystemDateTime(y, M, d, h, m, s);
		a.date = detail::formatDateTime(y, M, d, h, m, s);
	}
	a.boss = bossKill;

	if (bossKill)
		achievements.bosses++;
	achievements.acquired++;

	CAGE_LOG(SeverityEnum::Info, "achievements", string() + "acquired achievement: '" + name + "'");

	makeAnnouncement(
		HashString((string() + "achievement/" + name).c_str()),
		HashString((string() + "achievement-desc/" + name).c_str())
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
				ini->load("achievements.ini");
			}
			for (const string &section : ini->sections())
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
		CAGE_LOG(SeverityEnum::Info, "achievements", stringizer() + "acquired achievements: " + achievements.acquired + ", bosses: " + achievements.bosses);
		for (const auto &a : data)
			CAGE_LOG_CONTINUE(SeverityEnum::Note, "achievements", stringizer() + a.first + ": " + a.second.date + " (" + a.second.boss + ")");
		if (achievements.bosses > bossesTotalCount)
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
				ini->save("achievements.ini");
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
		Callbacks() : engineInitListener("achievements"), engineFinishListener("achievements")
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
	EntityManager *ents = engineGui()->entities();

	{
		CAGE_COMPONENT_GUI(LayoutLine, layout, ents->get(12));
		layout.vertical = true;
	}

	uint32 index = 0;
	for (const auto &it : data)
	{
		uint32 panelName;
		{ // panel
			Entity *e = ents->createUnique();
			panelName = e->name();
			CAGE_COMPONENT_GUI(Parent, parent, e);
			parent.parent = 12;
			parent.order = index++;
			CAGE_COMPONENT_GUI(Panel, panel, e);
			CAGE_COMPONENT_GUI(Text, txt, e);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString((string() + "achievement/" + it.first).c_str());
			CAGE_COMPONENT_GUI(LayoutLine, layout, e);
			layout.vertical = true;
		}

		{ // description
			Entity *e = ents->createUnique();
			CAGE_COMPONENT_GUI(Parent, parent, e);
			parent.parent = panelName;
			parent.order = 1;
			CAGE_COMPONENT_GUI(Label, label, e);
			CAGE_COMPONENT_GUI(Text, txt, e);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString((string() + "achievement-desc/" + it.first).c_str());
		}

		{ // date
			Entity *e = ents->createUnique();
			CAGE_COMPONENT_GUI(Parent, parent, e);
			parent.parent = panelName;
			parent.order = 2;
			CAGE_COMPONENT_GUI(Label, label, e);
			CAGE_COMPONENT_GUI(Text, txt, e);
			txt.value = it.second.date;
			CAGE_COMPONENT_GUI(TextFormat, format, e);
			format.align = TextAlignEnum::Right;
		}
	}
}
