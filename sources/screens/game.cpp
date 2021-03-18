#include <cage-core/enumerate.h>
#include <cage-core/macros.h>

#include "screens.h"
#include "../game.h"

#include <vector>
#include <algorithm>

namespace
{
	struct Announcement
	{
		uint32 headingName;
		uint32 descriptionName;
		uint32 duration;

		Announcement() : headingName(0), descriptionName(0), duration(30 * 30)
		{}
	};

	std::vector<Announcement> announcements;
	bool needToRemakeGui;

	EventListener<bool(uint32)> guiEvent;

	void makeTheGui(uint32 mode = 0);

	bool guiFunction(uint32 en)
	{
		switch (en)
		{
		case 500:
			game.life = 0;
			game.paused = false;
			return true;
		case 501:
			game.paused = false;
			return true;
		}

		for (uint32 i = 0; i < (uint32)PowerupTypeEnum::Total; i++)
		{
			if (PowerupMode[i] == 2)
			{
				if (en == 1000 + i * 4 + 2) // buy
				{
					CAGE_ASSERT(canAddPermanentPowerup());
					CAGE_ASSERT(game.money >= PowerupBuyPriceBase * game.buyPriceMultiplier);
					game.powerups[i]++;
					game.money -= PowerupBuyPriceBase * game.buyPriceMultiplier;
					game.buyPriceMultiplier++;
					engineGui()->skipAllEventsUntilNextUpdate();
					makeTheGui(3);
					return true;
				}
				if (en == 1000 + i * 4 + 3) // sell
				{
					CAGE_ASSERT(game.powerups[i] > 0);
					game.powerups[i]--;
					game.money += PowerupSellPriceBase * (game.defeatedBosses + 1);
					engineGui()->skipAllEventsUntilNextUpdate();
					makeTheGui(3);
					return true;
				}
			}
		}

		return false;
	}

	constexpr const uint32 TextNames[(uint32)PowerupTypeEnum::Total] = {
		HashString("gui/game/puBomb"),
		HashString("gui/game/puTurret"),
		HashString("gui/game/puDecoy"),
		HashString("gui/game/puHomingShots"),
		HashString("gui/game/puSuperDamage"),
		HashString("gui/game/puShield"),
		HashString("gui/game/puMaxSpeed"),
		HashString("gui/game/puAcceleration"),
		HashString("gui/game/puShotsDamage"),
		HashString("gui/game/puShotsSpeed"),
		HashString("gui/game/puShooting"),
		HashString("gui/game/puMultishot"),
		HashString("gui/game/puArmor"),
		HashString("gui/game/puDuration")
	};

	void makeTheGuiPaused(uint32 openPanel)
	{
		EntityManager *ents = engineGui()->entities();

		{
			CAGE_COMPONENT_GUI(LayoutLine, ll, ents->get(15));
			ll.vertical = true;
		}

		{ // continue button
			Entity *but = ents->create(501);
			CAGE_COMPONENT_GUI(Button, control, but);
			CAGE_COMPONENT_GUI(Text, txt, but);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/paused/continue");
			CAGE_COMPONENT_GUI(Parent, parent, but);
			parent.parent = 15;
			parent.order = 1;
			CAGE_COMPONENT_GUI(TextFormat, format, but);
			format.color = RedPillColor;
		}

		{ // end game button
			Entity *but = ents->create(500);
			CAGE_COMPONENT_GUI(Button, control, but);
			CAGE_COMPONENT_GUI(Text, txt, but);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/paused/giveup");
			CAGE_COMPONENT_GUI(Parent, parent, but);
			parent.parent = 15;
			parent.order = 2;
			CAGE_COMPONENT_GUI(TextFormat, format, but);
			format.color = BluePillColor;
		}

		uint32 layoutName;
		{ // layout
			Entity *e = ents->createUnique();
			layoutName = e->name();
			CAGE_COMPONENT_GUI(Parent, parent, e);
			parent.parent = 12;
			CAGE_COMPONENT_GUI(LayoutLine, ll, e);
			ll.vertical = true;
		}

		{ // story
			uint32 panelName;
			{ // panel
				Entity *e = ents->createUnique();
				panelName = e->name();
				CAGE_COMPONENT_GUI(Spoiler, spoiler, e);
				spoiler.collapsed = openPanel != 1;
				spoiler.collapsesSiblings = true;
				CAGE_COMPONENT_GUI(Parent, parent, e);
				parent.parent = layoutName;
				parent.order = 1;
				CAGE_COMPONENT_GUI(Text, text, e);
				text.assetName = HashString("degrid/languages/internationalized.textpack");
				text.textName = HashString("gui/paused/story");
				CAGE_COMPONENT_GUI(Scrollbars, sc, e);
				sc.alignment = vec2(0.5, 0.5);
				CAGE_COMPONENT_GUI(LayoutLine, ll, e);
				ll.vertical = true;
			}

			constexpr const uint32 TextNames[] = {
				#define GCHL_GENERATE(N) HashString("gui/story/" CAGE_STRINGIZE(N)),
						GCHL_GENERATE(0)
						CAGE_EVAL_MEDIUM(CAGE_REPEAT(20, GCHL_GENERATE))
				#undef GCHL_GENERATE
			};

			for (uint32 idx = 0; idx < game.defeatedBosses + 1; idx++)
			{
				Entity *label = engineGui()->entities()->createUnique();
				CAGE_COMPONENT_GUI(Parent, parent, label);
				parent.parent = panelName;
				parent.order = idx;
				CAGE_COMPONENT_GUI(Label, lab, label);
				CAGE_COMPONENT_GUI(Text, txt, label);
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = TextNames[idx];
				idx++;
			}
		}

		{ // bosses
			uint32 panelName;
			{ // panel
				Entity *e = ents->createUnique();
				panelName = e->name();
				CAGE_COMPONENT_GUI(Spoiler, spoiler, e);
				spoiler.collapsed = openPanel != 2;
				spoiler.collapsesSiblings = true;
				CAGE_COMPONENT_GUI(Parent, parent, e);
				parent.parent = layoutName;
				parent.order = 2;
				CAGE_COMPONENT_GUI(Text, text, e);
				text.assetName = HashString("degrid/languages/internationalized.textpack");
				text.textName = HashString("gui/paused/bosses");
				CAGE_COMPONENT_GUI(Scrollbars, sc, e);
				sc.alignment = vec2(0.5, 0.5);
				CAGE_COMPONENT_GUI(LayoutLine, ll, e);
				ll.vertical = false;
			}

			for (uint32 i = 0; i < BossesTotalCount; i++)
			{
				uint32 pn;
				{
					Entity *e = ents->createUnique();
					pn = e->name();
					CAGE_COMPONENT_GUI(Panel, panel, e);
					CAGE_COMPONENT_GUI(Parent, parent, e);
					parent.parent = panelName;
					parent.order = i;
					CAGE_COMPONENT_GUI(Text, text, e);
					text.assetName = HashString("degrid/languages/internationalized.textpack");
					text.textName = i < achievements.bosses ? HashString(string(stringizer() + "achievement/boss-" + i)) : HashString("achievement/boss-unknown");
				}
				{
					Entity *e = ents->createUnique();
					CAGE_COMPONENT_GUI(Label, label, e);
					CAGE_COMPONENT_GUI(Parent, parent, e);
					parent.parent = pn;
					parent.order = 1;
					CAGE_COMPONENT_GUI(Image, img, e);
					img.textureName = i < achievements.bosses ? HashString(string(stringizer() + "degrid/boss/icon/" + i + ".png")) : HashString("degrid/boss/icon/unknown.png");
				}
				if (game.defeatedBosses > i)
				{
					Entity *e = ents->createUnique();
					CAGE_COMPONENT_GUI(Label, label, e);
					CAGE_COMPONENT_GUI(Parent, parent, e);
					parent.parent = pn;
					parent.order = 2;
					CAGE_COMPONENT_GUI(Image, img, e);
					img.textureName = HashString("degrid/boss/icon/defeated.png");
				}
			}
		}

		{ // market
			uint32 marketName;
			{ // panel
				Entity *e = ents->createUnique();
				marketName = e->name();
				CAGE_COMPONENT_GUI(Spoiler, spoiler, e);
				spoiler.collapsed = openPanel != 3;
				spoiler.collapsesSiblings = true;
				CAGE_COMPONENT_GUI(Parent, parent, e);
				parent.parent = layoutName;
				parent.order = 3;
				CAGE_COMPONENT_GUI(Text, text, e);
				text.assetName = HashString("degrid/languages/internationalized.textpack");
				text.textName = HashString("gui/paused/market");
				CAGE_COMPONENT_GUI(Scrollbars, sc, e);
				sc.alignment = vec2(0.5, 0.5);
				CAGE_COMPONENT_GUI(LayoutLine, ll, e);
				ll.vertical = true;
			}

			{ // permanent powerup limit
				Entity *e = ents->createUnique();
				CAGE_COMPONENT_GUI(Parent, parent, e);
				parent.parent = marketName;
				parent.order = 1;
				CAGE_COMPONENT_GUI(Text, text, e);
				text.assetName = HashString("degrid/languages/internationalized.textpack");
				text.textName = HashString("gui/paused/permanentLimit");
				text.value = stringizer() + currentPermanentPowerups() + "|" + permanentPowerupLimit();
				CAGE_COMPONENT_GUI(Label, but, e);
				CAGE_COMPONENT_GUI(TextFormat, tf, e);
				tf.align = TextAlignEnum::Center;
			}

			uint32 panelName;
			{ // permanent powerup market

				Entity *e = ents->createUnique();
				panelName = e->name();
				CAGE_COMPONENT_GUI(Parent, parent, e);
				parent.parent = marketName;
				parent.order = 2;
				CAGE_COMPONENT_GUI(LayoutTable, lt, e);
				lt.sections = 4;
			}

			{ // header
				{ // label
					Entity *e = ents->createUnique();
					CAGE_COMPONENT_GUI(Parent, parent, e);
					parent.parent = panelName;
					parent.order = -10;
				}
				{ // count
					Entity *e = ents->createUnique();
					CAGE_COMPONENT_GUI(Parent, parent, e);
					parent.parent = panelName;
					parent.order = -9;
					CAGE_COMPONENT_GUI(Label, but, e);
					CAGE_COMPONENT_GUI(Text, text, e);
					text.assetName = HashString("degrid/languages/internationalized.textpack");
					text.textName = HashString("gui/paused/count");
				}
				{ // buy
					Entity *e = ents->createUnique();
					CAGE_COMPONENT_GUI(Parent, parent, e);
					parent.parent = panelName;
					parent.order = -8;
				}
				{ // sell
					Entity *e = ents->createUnique();
					CAGE_COMPONENT_GUI(Parent, parent, e);
					parent.parent = panelName;
					parent.order = -7;
				}
			}

			{ // footer
				{ // label
					Entity *e = ents->createUnique();
					CAGE_COMPONENT_GUI(Parent, parent, e);
					parent.parent = panelName;
					parent.order = 1000;
					CAGE_COMPONENT_GUI(Label, but, e);
					CAGE_COMPONENT_GUI(Text, text, e);
					text.assetName = HashString("degrid/languages/internationalized.textpack");
					text.textName = HashString("gui/paused/prices");
				}
				{ // empty
					Entity *e = ents->createUnique();
					CAGE_COMPONENT_GUI(Parent, parent, e);
					parent.parent = panelName;
					parent.order = 1001;
				}
				{ // buy
					Entity *e = ents->createUnique();
					CAGE_COMPONENT_GUI(Parent, parent, e);
					parent.parent = panelName;
					parent.order = 1002;
					CAGE_COMPONENT_GUI(Text, text, e);
					text.value = stringizer() + (PowerupBuyPriceBase * game.buyPriceMultiplier);
					CAGE_COMPONENT_GUI(Label, but, e);
					CAGE_COMPONENT_GUI(TextFormat, tf, e);
					tf.align = TextAlignEnum::Center;
				}
				{ // sell
					Entity *e = ents->createUnique();
					CAGE_COMPONENT_GUI(Parent, parent, e);
					parent.parent = panelName;
					parent.order = 1003;
					CAGE_COMPONENT_GUI(Text, text, e);
					text.value = stringizer() + (PowerupSellPriceBase * (game.defeatedBosses + 1));
					CAGE_COMPONENT_GUI(Label, but, e);
					CAGE_COMPONENT_GUI(TextFormat, tf, e);
					tf.align = TextAlignEnum::Center;
				}
			}

			for (uint32 i = 0; i < (uint32)PowerupTypeEnum::Total; i++)
			{
				bool anyBuy = canAddPermanentPowerup();
				if (PowerupMode[i] == 2)
				{
					{ // label
						Entity *e = ents->create(1000 + i * 4 + 0);
						CAGE_COMPONENT_GUI(Parent, parent, e);
						parent.parent = panelName;
						parent.order = i * 4 + 0;
						CAGE_COMPONENT_GUI(Text, text, e);
						text.assetName = HashString("degrid/languages/internationalized.textpack");
						text.textName = TextNames[i];
						CAGE_COMPONENT_GUI(Label, but, e);
					}
					{ // count
						Entity *e = ents->create(1000 + i * 4 + 1);
						CAGE_COMPONENT_GUI(Parent, parent, e);
						parent.parent = panelName;
						parent.order = i * 4 + 1;
						CAGE_COMPONENT_GUI(Text, text, e);
						text.value = stringizer() + game.powerups[i];
						CAGE_COMPONENT_GUI(Label, but, e);
						CAGE_COMPONENT_GUI(TextFormat, format, e);
						format.align = TextAlignEnum::Center;
					}
					{ // buy
						Entity *e = ents->create(1000 + i * 4 + 2);
						CAGE_COMPONENT_GUI(Parent, parent, e);
						parent.parent = panelName;
						parent.order = i * 4 + 2;
						CAGE_COMPONENT_GUI(Text, text, e);
						text.assetName = HashString("degrid/languages/internationalized.textpack");
						text.textName = HashString("gui/paused/buy");
						CAGE_COMPONENT_GUI(Button, but, e);
						if (!anyBuy || game.money < PowerupBuyPriceBase * game.buyPriceMultiplier)
						{
							CAGE_COMPONENT_GUI(WidgetState, ws, e);
							ws.disabled = true;
						}
					}
					{ // sell
						Entity *e = ents->create(1000 + i * 4 + 3);
						CAGE_COMPONENT_GUI(Parent, parent, e);
						parent.parent = panelName;
						parent.order = i * 4 + 3;
						CAGE_COMPONENT_GUI(Text, text, e);
						text.assetName = HashString("degrid/languages/internationalized.textpack");
						text.textName = HashString("gui/paused/sell");
						CAGE_COMPONENT_GUI(Button, but, e);
						if (game.powerups[i] == 0)
						{
							CAGE_COMPONENT_GUI(WidgetState, ws, e);
							ws.disabled = true;
						}
					}
				}
			}
		}
	}

	void makeTheGuiPlaying()
	{
		EntityManager *ents = engineGui()->entities();

		{
			CAGE_COMPONENT_GUI(Scrollbars, sc, ents->get(12));
			sc.alignment = vec2(0.5, 0);
		}

		uint32 layoutName;
		{ // layout
			Entity *e = ents->createUnique();
			layoutName = e->name();
			CAGE_COMPONENT_GUI(Parent, parent, e);
			parent.parent = 12;
			CAGE_COMPONENT_GUI(LayoutLine, ll, e);
			ll.vertical = true;
		}

		{ // announcements
			for (auto a : enumerate(announcements))
			{
				Entity *panel = ents->createUnique();
				{
					CAGE_COMPONENT_GUI(Panel, panel2, panel);
					CAGE_COMPONENT_GUI(Parent, parent, panel);
					parent.parent = layoutName;
					parent.order = numeric_cast<sint32>(a.index) + 1;
					CAGE_COMPONENT_GUI(Text, txt, panel);
					txt.assetName = HashString("degrid/languages/internationalized.textpack");
					txt.textName = a->headingName;
					CAGE_COMPONENT_GUI(TextFormat, format, panel);
					format.size = 20;
					CAGE_COMPONENT_GUI(LayoutLine, ll, panel);
					ll.vertical = true;
				}

				{ // description
					Entity *label = ents->createUnique();
					CAGE_COMPONENT_GUI(Parent, parent, label);
					parent.parent = panel->name();
					CAGE_COMPONENT_GUI(Label, control, label);
					CAGE_COMPONENT_GUI(Text, txt, label);
					txt.assetName = HashString("degrid/languages/internationalized.textpack");
					txt.textName = a->descriptionName;
					CAGE_COMPONENT_GUI(TextFormat, format, label);
					format.size = 20;
				}
			}
		}
	}

	void makeTheGui(uint32 openPanel)
	{
		{
			GuiConfig c;
			c.logo = false;
			c.backButton = false;
			regenerateGui(c);
		}
		EntityManager *ents = engineGui()->entities();
		guiEvent.bind<&guiFunction>();
		guiEvent.attach(engineGui()->widgetEvent);

		{ // base stats
			Entity *table = ents->createUnique();
			{
				CAGE_COMPONENT_GUI(Parent, parent, table);
				parent.parent = 10;
				CAGE_COMPONENT_GUI(LayoutTable, layout, table);
				CAGE_COMPONENT_GUI(Panel, gb, table);
			}
			uint32 index = 1;

			{ // life label
				Entity *label = engineGui()->entities()->createUnique();
				CAGE_COMPONENT_GUI(Parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(Label, control, label);
				CAGE_COMPONENT_GUI(Text, text, label);
				text.assetName = HashString("degrid/languages/internationalized.textpack");
				text.textName = HashString("gui/game/life");
				CAGE_COMPONENT_GUI(TextFormat, format, label);
				format.color = vec3(1, 0, 0);
				format.size = 20;
			}

			{ // life value
				Entity *label = engineGui()->entities()->create(100);
				CAGE_COMPONENT_GUI(Parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(Label, control, label);
				CAGE_COMPONENT_GUI(Text, text, label);
				CAGE_COMPONENT_GUI(TextFormat, format, label);
				format.align = TextAlignEnum::Right;
				format.color = vec3(1, 0, 0);
				format.size = 20;
			}

			{ // money label
				Entity *label = engineGui()->entities()->createUnique();
				CAGE_COMPONENT_GUI(Parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(Label, control, label);
				CAGE_COMPONENT_GUI(Text, text, label);
				text.assetName = HashString("degrid/languages/internationalized.textpack");
				text.textName = HashString("gui/game/money");
				CAGE_COMPONENT_GUI(TextFormat, format, label);
				format.color = vec3(1, 1, 0);
				format.size = 20;
			}

			{ // money value
				Entity *label = engineGui()->entities()->create(101);
				CAGE_COMPONENT_GUI(Parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(Label, control, label);
				CAGE_COMPONENT_GUI(Text, text, label);
				CAGE_COMPONENT_GUI(TextFormat, format, label);
				format.align = TextAlignEnum::Right;
				format.color = vec3(1, 1, 0);
				format.size = 20;
			}

			{ // score label
				Entity *label = engineGui()->entities()->createUnique();
				CAGE_COMPONENT_GUI(Parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(Label, control, label);
				CAGE_COMPONENT_GUI(Text, text, label);
				text.assetName = HashString("degrid/languages/internationalized.textpack");
				text.textName = HashString("gui/game/score");
				CAGE_COMPONENT_GUI(TextFormat, format, label);
				format.color = vec3(0, 1, 0);
				format.size = 20;
			}

			{ // score value
				Entity *label = engineGui()->entities()->create(102);
				CAGE_COMPONENT_GUI(Parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(Label, control, label);
				CAGE_COMPONENT_GUI(Text, text, label);
				CAGE_COMPONENT_GUI(TextFormat, format, label);
				format.align = TextAlignEnum::Right;
				format.color = vec3(0, 1, 0);
				format.size = 20;
			}
		}

		{ // collectible powerups
			Entity *table = ents->createUnique();
			{
				CAGE_COMPONENT_GUI(Parent, parent, table);
				parent.parent = 14;
				CAGE_COMPONENT_GUI(LayoutTable, layout, table);
				CAGE_COMPONENT_GUI(Panel, gb, table);
			}
			uint32 index = 1;

			for (uint32 i = 0; i < (uint32)PowerupTypeEnum::Total; i++)
			{
				if (PowerupMode[i] == 0)
				{
					{ // label
						Entity *label = engineGui()->entities()->createUnique();
						CAGE_COMPONENT_GUI(Parent, parent, label);
						parent.parent = table->name();
						parent.order = index++;
						CAGE_COMPONENT_GUI(Label, control, label);
						CAGE_COMPONENT_GUI(Text, text, label);
						text.assetName = HashString("degrid/languages/internationalized.textpack");
						text.textName = TextNames[i];
					}

					{ // value
						Entity *label = engineGui()->entities()->create(200 + i);
						CAGE_COMPONENT_GUI(Parent, parent, label);
						parent.parent = table->name();
						parent.order = index++;
						CAGE_COMPONENT_GUI(Label, control, label);
						CAGE_COMPONENT_GUI(Text, text, label);
						CAGE_COMPONENT_GUI(TextFormat, format, label);
						format.align = TextAlignEnum::Right;
					}
				}
			}
		}

		{ // timed
			Entity *table = ents->createUnique();
			{
				CAGE_COMPONENT_GUI(Parent, parent, table);
				parent.parent = 11;
				CAGE_COMPONENT_GUI(LayoutTable, layout, table);
				CAGE_COMPONENT_GUI(Panel, gb, table);
			}
			uint32 index = 1;

			for (uint32 i = 0; i < (uint32)PowerupTypeEnum::Total; i++)
			{
				if (PowerupMode[i] == 1)
				{
					{ // label
						Entity *label = engineGui()->entities()->createUnique();
						CAGE_COMPONENT_GUI(Parent, parent, label);
						parent.parent = table->name();
						parent.order = index++;
						CAGE_COMPONENT_GUI(Label, control, label);
						CAGE_COMPONENT_GUI(Text, text, label);
						text.assetName = HashString("degrid/languages/internationalized.textpack");
						text.textName = TextNames[i];
					}

					{ // value
						Entity *label = engineGui()->entities()->create(200 + i);
						CAGE_COMPONENT_GUI(Parent, parent, label);
						parent.parent = table->name();
						parent.order = index++;
						CAGE_COMPONENT_GUI(Label, control, label);
						CAGE_COMPONENT_GUI(Text, text, label);
						CAGE_COMPONENT_GUI(TextFormat, format, label);
						format.align = TextAlignEnum::Right;
					}
				}
			}
		}

		if (game.paused)
			makeTheGuiPaused(openPanel);
		else
			makeTheGuiPlaying();
	}

	bool validateAnnouncements()
	{
		// delete elapsed announcements
		bool result = needToRemakeGui;
		needToRemakeGui = false;
		announcements.erase(std::remove_if(announcements.begin(), announcements.end(), [&](Announcement &a)
		{
			if (a.duration == 0)
			{
				result = true;
				return true;
			}
			a.duration--;
			return false;
		}
		), announcements.end());
		return result;
	}

	bool previousPaused = false;

	void engineUpdate()
	{
		OPTICK_EVENT("gui");

		if (game.gameOver || game.cinematic)
			return;

		if (!game.paused)
		{
			if (validateAnnouncements())
				makeTheGui();
		}

		if (game.paused != previousPaused)
		{
			previousPaused = game.paused;
			makeTheGui(currentPermanentPowerups() == 0 && game.money > 0 ? 3 : 1); // market or story
		}

		{ // life
			CAGE_COMPONENT_GUI(Text, txt, engineGui()->entities()->get(100));
			txt.value = stringizer() + numeric_cast<uint32>(max(0, game.life));
		}
		{ // money
			CAGE_COMPONENT_GUI(Text, txt, engineGui()->entities()->get(101));
			txt.value = stringizer() + game.money;
		}
		{ // score
			CAGE_COMPONENT_GUI(Text, txt, engineGui()->entities()->get(102));
			txt.value = stringizer() + game.score;
		}

		for (uint32 i = 0; i < (uint32)PowerupTypeEnum::Total; i++)
		{
			if (!engineGui()->entities()->has(200 + i))
				continue;
			CAGE_COMPONENT_GUI(Text, txt, engineGui()->entities()->get(200 + i));
			switch (PowerupMode[i])
			{
			case 0: // collectibles
				txt.value = stringizer() + game.powerups[i];
				break;
			case 1: // timed
				txt.value = stringizer() + game.powerups[i] / 30;
				break;
			case 2: // permanent
				txt.value = stringizer() + game.powerups[i];
				break;
			}
		}
	}

	class Callbacks
	{
		EventListener<void()> engineUpdateListener;
	public:
		Callbacks()
		{
			engineUpdateListener.attach(controlThread().update, 60);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInstance;
}

void setScreenGame()
{
	game.cinematic = false;
	announcements.clear();
	gameStartEvent().dispatch();
	if (game.money > 0)
		game.paused = true;
	previousPaused = !game.paused;
}

void makeAnnouncement(uint32 headline, uint32 description, uint32 duration)
{
	Announcement a;
	a.headingName = headline;
	a.descriptionName = description;
	a.duration = duration;
	announcements.push_back(a);
	needToRemakeGui = true;
}
