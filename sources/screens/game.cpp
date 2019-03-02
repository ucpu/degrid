#include "screens.h"
#include "../game.h"

#include <vector>
#include <algorithm>

namespace
{
	struct announcementStruct
	{
		uint32 headingName;
		uint32 descriptionName;
		uint32 duration;

		announcementStruct() : headingName(0), descriptionName(0), duration(30 * 30)
		{}
	};

	std::vector<announcementStruct> announcements;
	bool needToRemakeGui;

	eventListener<bool(uint32)> guiEvent;

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

		for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
		{
			if (powerupMode[i] == 2)
			{
				if (en == 1000 + i * 4 + 2) // sell
				{
					CAGE_ASSERT_RUNTIME(game.powerups[i] > 0);
					game.powerups[i]--;
					game.money += basePermanentPowerupSellPrice;
					gui()->skipAllEventsUntilNextUpdate();
					makeTheGui(3);
					return true;
				}
				if (en == 1000 + i * 4 + 3) // buy
				{
					CAGE_ASSERT_RUNTIME(canAddPermanentPowerup());
					CAGE_ASSERT_RUNTIME(game.money >= basePermanentPowerupBuyPrice * game.buyPriceMultiplier);
					game.powerups[i]++;
					game.money -= basePermanentPowerupBuyPrice * game.buyPriceMultiplier;
					game.buyPriceMultiplier++;
					gui()->skipAllEventsUntilNextUpdate();
					makeTheGui(3);
					return true;
				}
			}
		}

		return false;
	}

	static const uint32 textNames[(uint32)powerupTypeEnum::Total] = {
		hashString("gui/game/puBomb"),
		hashString("gui/game/puTurret"),
		hashString("gui/game/puDecoy"),
		hashString("gui/game/puHomingShots"),
		hashString("gui/game/puSuperDamage"),
		hashString("gui/game/puShield"),
		hashString("gui/game/puMaxSpeed"),
		hashString("gui/game/puAcceleration"),
		hashString("gui/game/puShotsDamage"),
		hashString("gui/game/puShotsSpeed"),
		hashString("gui/game/puShooting"),
		hashString("gui/game/puMultishot"),
		hashString("gui/game/puArmor"),
		hashString("gui/game/puDuration")
	};

	void makeTheGuiPaused(uint32 openPanel)
	{
		entityManagerClass *ents = gui()->entities();

		{
			GUI_GET_COMPONENT(layoutLine, ll, ents->get(15));
			ll.vertical = true;
		}

		{ // continue button
			entityClass *but = ents->create(501);
			GUI_GET_COMPONENT(button, control, but);
			GUI_GET_COMPONENT(text, txt, but);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/paused/continue");
			GUI_GET_COMPONENT(parent, parent, but);
			parent.parent = 15;
			parent.order = 1;
			GUI_GET_COMPONENT(textFormat, format, but);
			format.color = redPillColor;
		}

		{ // end game button
			entityClass *but = ents->create(500);
			GUI_GET_COMPONENT(button, control, but);
			GUI_GET_COMPONENT(text, txt, but);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/paused/giveup");
			GUI_GET_COMPONENT(parent, parent, but);
			parent.parent = 15;
			parent.order = 2;
			GUI_GET_COMPONENT(textFormat, format, but);
			format.color = bluePillColor;
		}

		uint32 layoutName;
		{ // layout
			entityClass *e = ents->createUnique();
			layoutName = e->name();
			GUI_GET_COMPONENT(parent, parent, e);
			parent.parent = 12;
			GUI_GET_COMPONENT(layoutLine, ll, e);
			ll.vertical = true;
		}

		{ // story
			uint32 panelName;
			{ // panel
				entityClass *e = ents->createUnique();
				panelName = e->name();
				GUI_GET_COMPONENT(spoiler, spoiler, e);
				spoiler.collapsed = openPanel != 1;
				spoiler.collapsesSiblings = true;
				GUI_GET_COMPONENT(parent, parent, e);
				parent.parent = layoutName;
				parent.order = 1;
				GUI_GET_COMPONENT(text, text, e);
				text.assetName = hashString("grid/languages/internationalized.textpack");
				text.textName = hashString("gui/paused/story");
				GUI_GET_COMPONENT(scrollbars, sc, e);
				sc.alignment = vec2(0.5, 0.5);
				GUI_GET_COMPONENT(layoutLine, ll, e);
				ll.vertical = true;
			}

			static const uint32 textNames[] = {
				#define GCHL_GENERATE(N) hashString("gui/story/" CAGE_STRINGIZE(N)),
						GCHL_GENERATE(0)
						CAGE_EVAL_MEDIUM(CAGE_REPEAT(20, GCHL_GENERATE))
				#undef GCHL_GENERATE
			};

			for (uint32 idx = 0; idx < game.defeatedBosses + 1; idx++)
			{
				entityClass *label = gui()->entities()->createUnique();
				GUI_GET_COMPONENT(parent, parent, label);
				parent.parent = panelName;
				parent.order = idx;
				GUI_GET_COMPONENT(label, lab, label);
				GUI_GET_COMPONENT(text, txt, label);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = textNames[idx];
				idx++;
			}
		}

		{ // bosses
			uint32 panelName;
			{ // panel
				entityClass *e = ents->createUnique();
				panelName = e->name();
				GUI_GET_COMPONENT(spoiler, spoiler, e);
				spoiler.collapsed = openPanel != 2;
				spoiler.collapsesSiblings = true;
				GUI_GET_COMPONENT(parent, parent, e);
				parent.parent = layoutName;
				parent.order = 2;
				GUI_GET_COMPONENT(text, text, e);
				text.assetName = hashString("grid/languages/internationalized.textpack");
				text.textName = hashString("gui/paused/bosses");
				GUI_GET_COMPONENT(scrollbars, sc, e);
				sc.alignment = vec2(0.5, 0.5);
				GUI_GET_COMPONENT(layoutLine, ll, e);
				ll.vertical = false;
			}

			for (uint32 i = 0; i < bossesTotalCount; i++)
			{
				uint32 pn;
				{
					entityClass *e = ents->createUnique();
					pn = e->name();
					GUI_GET_COMPONENT(panel, panel, e);
					GUI_GET_COMPONENT(parent, parent, e);
					parent.parent = panelName;
					parent.order = i;
					GUI_GET_COMPONENT(text, text, e);
					text.assetName = hashString("grid/languages/internationalized.textpack");
					text.textName = i < achievements.bosses ? hashString((string() + "achievement/boss-" + i).c_str()) : hashString("achievement/boss-unknown");
				}
				{
					entityClass *e = ents->createUnique();
					GUI_GET_COMPONENT(label, label, e);
					GUI_GET_COMPONENT(parent, parent, e);
					parent.parent = pn;
					parent.order = 1;
					GUI_GET_COMPONENT(image, img, e);
					img.textureName = i < achievements.bosses ? hashString((string() + "grid/boss/icon/" + i + ".png").c_str()) : hashString("grid/boss/icon/unknown.png");
				}
				if (game.defeatedBosses > i)
				{
					entityClass *e = ents->createUnique();
					GUI_GET_COMPONENT(label, label, e);
					GUI_GET_COMPONENT(parent, parent, e);
					parent.parent = pn;
					parent.order = 2;
					GUI_GET_COMPONENT(image, img, e);
					img.textureName = hashString("grid/boss/icon/defeated.png");
				}
			}
		}

		{ // market
			uint32 marketName;
			{ // panel
				entityClass *e = ents->createUnique();
				marketName = e->name();
				GUI_GET_COMPONENT(spoiler, spoiler, e);
				spoiler.collapsed = openPanel != 3;
				spoiler.collapsesSiblings = true;
				GUI_GET_COMPONENT(parent, parent, e);
				parent.parent = layoutName;
				parent.order = 3;
				GUI_GET_COMPONENT(text, text, e);
				text.assetName = hashString("grid/languages/internationalized.textpack");
				text.textName = hashString("gui/paused/market");
				GUI_GET_COMPONENT(scrollbars, sc, e);
				sc.alignment = vec2(0.5, 0.5);
				GUI_GET_COMPONENT(layoutLine, ll, e);
				ll.vertical = true;
			}

			{ // permanent powerup limit
				entityClass *e = ents->createUnique();
				GUI_GET_COMPONENT(parent, parent, e);
				parent.parent = marketName;
				parent.order = 1;
				GUI_GET_COMPONENT(text, text, e);
				text.assetName = hashString("grid/languages/internationalized.textpack");
				text.textName = hashString("gui/paused/permanentLimit");
				text.value = string() + currentPermanentPowerups() + "|" + permanentPowerupLimit();
				GUI_GET_COMPONENT(label, but, e);
				GUI_GET_COMPONENT(textFormat, tf, e);
				tf.align = textAlignEnum::Center;
			}

			uint32 panelName;
			{ // permanent powerup market

				entityClass *e = ents->createUnique();
				panelName = e->name();
				GUI_GET_COMPONENT(parent, parent, e);
				parent.parent = marketName;
				parent.order = 2;
				GUI_GET_COMPONENT(layoutTable, lt, e);
				lt.sections = 4;
			}

			{ // header
				{ // label
					entityClass *e = ents->createUnique();
					GUI_GET_COMPONENT(parent, parent, e);
					parent.parent = panelName;
					parent.order = -10;
				}
				{ // count
					entityClass *e = ents->createUnique();
					GUI_GET_COMPONENT(parent, parent, e);
					parent.parent = panelName;
					parent.order = -9;
					GUI_GET_COMPONENT(label, but, e);
					GUI_GET_COMPONENT(text, text, e);
					text.assetName = hashString("grid/languages/internationalized.textpack");
					text.textName = hashString("gui/paused/count");
				}
				{ // sell
					entityClass *e = ents->createUnique();
					GUI_GET_COMPONENT(parent, parent, e);
					parent.parent = panelName;
					parent.order = -8;
					GUI_GET_COMPONENT(text, text, e);
					text.assetName = hashString("grid/languages/internationalized.textpack");
					text.textName = hashString("gui/paused/sell");
					GUI_GET_COMPONENT(label, but, e);
					GUI_GET_COMPONENT(textFormat, tf, e);
					tf.align = textAlignEnum::Center;
				}
				{ // buy
					entityClass *e = ents->createUnique();
					GUI_GET_COMPONENT(parent, parent, e);
					parent.parent = panelName;
					parent.order = -7;
					GUI_GET_COMPONENT(text, text, e);
					text.assetName = hashString("grid/languages/internationalized.textpack");
					text.textName = hashString("gui/paused/buy");
					GUI_GET_COMPONENT(label, but, e);
					GUI_GET_COMPONENT(textFormat, tf, e);
					tf.align = textAlignEnum::Center;
				}
			}

			for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
			{
				bool anyBuy = canAddPermanentPowerup();
				if (powerupMode[i] == 2)
				{
					{ // label
						entityClass *e = ents->create(1000 + i * 4 + 0);
						GUI_GET_COMPONENT(parent, parent, e);
						parent.parent = panelName;
						parent.order = i * 4 + 0;
						GUI_GET_COMPONENT(text, text, e);
						text.assetName = hashString("grid/languages/internationalized.textpack");
						text.textName = textNames[i];
						GUI_GET_COMPONENT(label, but, e);
					}
					{ // count
						entityClass *e = ents->create(1000 + i * 4 + 1);
						GUI_GET_COMPONENT(parent, parent, e);
						parent.parent = panelName;
						parent.order = i * 4 + 1;
						GUI_GET_COMPONENT(text, text, e);
						text.value = game.powerups[i];
						GUI_GET_COMPONENT(label, but, e);
						GUI_GET_COMPONENT(textFormat, format, e);
						format.align = textAlignEnum::Center;
					}
					{ // sell
						entityClass *e = ents->create(1000 + i * 4 + 2);
						GUI_GET_COMPONENT(parent, parent, e);
						parent.parent = panelName;
						parent.order = i * 4 + 2;
						GUI_GET_COMPONENT(text, text, e);
						text.value = basePermanentPowerupSellPrice;
						GUI_GET_COMPONENT(button, but, e);
						if (game.powerups[i] == 0)
						{
							GUI_GET_COMPONENT(widgetState, ws, e);
							ws.disabled = true;
						}
					}
					{ // buy
						entityClass *e = ents->create(1000 + i * 4 + 3);
						GUI_GET_COMPONENT(parent, parent, e);
						parent.parent = panelName;
						parent.order = i * 4 + 3;
						GUI_GET_COMPONENT(text, text, e);
						text.value = basePermanentPowerupBuyPrice * game.buyPriceMultiplier;
						GUI_GET_COMPONENT(button, but, e);
						if (!anyBuy || game.money < basePermanentPowerupBuyPrice * game.buyPriceMultiplier)
						{
							GUI_GET_COMPONENT(widgetState, ws, e);
							ws.disabled = true;
						}
					}
				}
			}
		}
	}

	void makeTheGuiPlaying()
	{
		entityManagerClass *ents = gui()->entities();

		{
			GUI_GET_COMPONENT(scrollbars, sc, ents->get(12));
			sc.alignment = vec2(0.5, 0);
		}

		uint32 layoutName;
		{ // layout
			entityClass *e = ents->createUnique();
			layoutName = e->name();
			GUI_GET_COMPONENT(parent, parent, e);
			parent.parent = 12;
			GUI_GET_COMPONENT(layoutLine, ll, e);
			ll.vertical = true;
		}

		{ // boss healthbar
			// todo
		}

		{ // announcements
			uint32 order = 1;
			for (const auto &a : announcements)
			{
				entityClass *panel = ents->createUnique();
				{
					GUI_GET_COMPONENT(panel, panel2, panel);
					GUI_GET_COMPONENT(parent, parent, panel);
					parent.parent = layoutName;
					parent.order = order++;
					GUI_GET_COMPONENT(text, txt, panel);
					txt.assetName = hashString("grid/languages/internationalized.textpack");
					txt.textName = a.headingName;
					GUI_GET_COMPONENT(textFormat, format, panel);
					format.size = 20;
					GUI_GET_COMPONENT(layoutLine, ll, panel);
					ll.vertical = true;
				}

				{ // description
					entityClass *label = ents->createUnique();
					GUI_GET_COMPONENT(parent, parent, label);
					parent.parent = panel->name();
					GUI_GET_COMPONENT(label, control, label);
					GUI_GET_COMPONENT(text, txt, label);
					txt.assetName = hashString("grid/languages/internationalized.textpack");
					txt.textName = a.descriptionName;
					GUI_GET_COMPONENT(textFormat, format, label);
					format.size = 20;
				}
			}
		}
	}

	void makeTheGui(uint32 openPanel)
	{
		{
			guiConfig c;
			c.logo = false;
			c.backButton = false;
			regenerateGui(c);
		}
		entityManagerClass *ents = gui()->entities();
		guiEvent.bind<&guiFunction>();
		guiEvent.attach(gui()->widgetEvent);

		{ // base stats
			entityClass *table = ents->createUnique();
			{
				GUI_GET_COMPONENT(parent, parent, table);
				parent.parent = 10;
				GUI_GET_COMPONENT(layoutTable, layout, table);
				GUI_GET_COMPONENT(panel, gb, table);
			}
			uint32 index = 1;

			{ // life label
				entityClass *label = gui()->entities()->createUnique();
				GUI_GET_COMPONENT(parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, label);
				GUI_GET_COMPONENT(text, text, label);
				text.assetName = hashString("grid/languages/internationalized.textpack");
				text.textName = hashString("gui/game/life");
				GUI_GET_COMPONENT(textFormat, format, label);
				format.color = vec3(1, 0, 0);
				format.size = 20;
			}

			{ // life value
				entityClass *label = gui()->entities()->create(100);
				GUI_GET_COMPONENT(parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, label);
				GUI_GET_COMPONENT(text, text, label);
				GUI_GET_COMPONENT(textFormat, format, label);
				format.align = textAlignEnum::Right;
				format.color = vec3(1, 0, 0);
				format.size = 20;
			}

			{ // money label
				entityClass *label = gui()->entities()->createUnique();
				GUI_GET_COMPONENT(parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, label);
				GUI_GET_COMPONENT(text, text, label);
				text.assetName = hashString("grid/languages/internationalized.textpack");
				text.textName = hashString("gui/game/money");
				GUI_GET_COMPONENT(textFormat, format, label);
				format.color = vec3(1, 1, 0);
				format.size = 20;
			}

			{ // money value
				entityClass *label = gui()->entities()->create(101);
				GUI_GET_COMPONENT(parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, label);
				GUI_GET_COMPONENT(text, text, label);
				GUI_GET_COMPONENT(textFormat, format, label);
				format.align = textAlignEnum::Right;
				format.color = vec3(1, 1, 0);
				format.size = 20;
			}

			{ // score label
				entityClass *label = gui()->entities()->createUnique();
				GUI_GET_COMPONENT(parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, label);
				GUI_GET_COMPONENT(text, text, label);
				text.assetName = hashString("grid/languages/internationalized.textpack");
				text.textName = hashString("gui/game/score");
				GUI_GET_COMPONENT(textFormat, format, label);
				format.color = vec3(0, 1, 0);
				format.size = 20;
			}

			{ // score value
				entityClass *label = gui()->entities()->create(102);
				GUI_GET_COMPONENT(parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, label);
				GUI_GET_COMPONENT(text, text, label);
				GUI_GET_COMPONENT(textFormat, format, label);
				format.align = textAlignEnum::Right;
				format.color = vec3(0, 1, 0);
				format.size = 20;
			}
		}

		{ // collectible powerups
			entityClass *table = ents->createUnique();
			{
				GUI_GET_COMPONENT(parent, parent, table);
				parent.parent = 14;
				GUI_GET_COMPONENT(layoutTable, layout, table);
				GUI_GET_COMPONENT(panel, gb, table);
			}
			uint32 index = 1;

			for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
			{
				if (powerupMode[i] == 0)
				{
					{ // label
						entityClass *label = gui()->entities()->createUnique();
						GUI_GET_COMPONENT(parent, parent, label);
						parent.parent = table->name();
						parent.order = index++;
						GUI_GET_COMPONENT(label, control, label);
						GUI_GET_COMPONENT(text, text, label);
						text.assetName = hashString("grid/languages/internationalized.textpack");
						text.textName = textNames[i];
					}

					{ // value
						entityClass *label = gui()->entities()->create(200 + i);
						GUI_GET_COMPONENT(parent, parent, label);
						parent.parent = table->name();
						parent.order = index++;
						GUI_GET_COMPONENT(label, control, label);
						GUI_GET_COMPONENT(text, text, label);
						GUI_GET_COMPONENT(textFormat, format, label);
						format.align = textAlignEnum::Right;
					}
				}
			}
		}

		{ // timed
			entityClass *table = ents->createUnique();
			{
				GUI_GET_COMPONENT(parent, parent, table);
				parent.parent = 11;
				GUI_GET_COMPONENT(layoutTable, layout, table);
				GUI_GET_COMPONENT(panel, gb, table);
			}
			uint32 index = 1;

			for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
			{
				if (powerupMode[i] == 1)
				{
					{ // label
						entityClass *label = gui()->entities()->createUnique();
						GUI_GET_COMPONENT(parent, parent, label);
						parent.parent = table->name();
						parent.order = index++;
						GUI_GET_COMPONENT(label, control, label);
						GUI_GET_COMPONENT(text, text, label);
						text.assetName = hashString("grid/languages/internationalized.textpack");
						text.textName = textNames[i];
					}

					{ // value
						entityClass *label = gui()->entities()->create(200 + i);
						GUI_GET_COMPONENT(parent, parent, label);
						parent.parent = table->name();
						parent.order = index++;
						GUI_GET_COMPONENT(label, control, label);
						GUI_GET_COMPONENT(text, text, label);
						GUI_GET_COMPONENT(textFormat, format, label);
						format.align = textAlignEnum::Right;
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
		announcements.erase(std::remove_if(announcements.begin(), announcements.end(), [&](announcementStruct &a)
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
			GUI_GET_COMPONENT(text, txt, gui()->entities()->get(100));
			txt.value = numeric_cast<uint32>(max(0, game.life));
		}
		{ // money
			GUI_GET_COMPONENT(text, txt, gui()->entities()->get(101));
			txt.value = game.money;
		}
		{ // score
			GUI_GET_COMPONENT(text, txt, gui()->entities()->get(102));
			txt.value = game.score;
		}

		for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
		{
			if (!gui()->entities()->has(200 + i))
				continue;
			GUI_GET_COMPONENT(text, txt, gui()->entities()->get(200 + i));
			switch (powerupMode[i])
			{
			case 0: // collectibles
				txt.value = game.powerups[i];
				break;
			case 1: // timed
				txt.value = game.powerups[i] / 30;
				break;
			case 2: // permanent
				txt.value = game.powerups[i];
				break;
			}
		}
	}

	class callbacksClass
	{
		eventListener<void()> engineUpdateListener;
	public:
		callbacksClass()
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
	announcementStruct a;
	a.headingName = headline;
	a.descriptionName = description;
	a.duration = duration;
	announcements.push_back(a);
	needToRemakeGui = true;
}
