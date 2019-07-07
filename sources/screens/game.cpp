#include "screens.h"
#include "../game.h"
#include <cage-core/enumerate.h>

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
		entityManager *ents = gui()->entities();

		{
			CAGE_COMPONENT_GUI(layoutLine, ll, ents->get(15));
			ll.vertical = true;
		}

		{ // continue button
			entity *but = ents->create(501);
			CAGE_COMPONENT_GUI(button, control, but);
			CAGE_COMPONENT_GUI(text, txt, but);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/paused/continue");
			CAGE_COMPONENT_GUI(parent, parent, but);
			parent.parent = 15;
			parent.order = 1;
			CAGE_COMPONENT_GUI(textFormat, format, but);
			format.color = redPillColor;
		}

		{ // end game button
			entity *but = ents->create(500);
			CAGE_COMPONENT_GUI(button, control, but);
			CAGE_COMPONENT_GUI(text, txt, but);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/paused/giveup");
			CAGE_COMPONENT_GUI(parent, parent, but);
			parent.parent = 15;
			parent.order = 2;
			CAGE_COMPONENT_GUI(textFormat, format, but);
			format.color = bluePillColor;
		}

		uint32 layoutName;
		{ // layout
			entity *e = ents->createUnique();
			layoutName = e->name();
			CAGE_COMPONENT_GUI(parent, parent, e);
			parent.parent = 12;
			CAGE_COMPONENT_GUI(layoutLine, ll, e);
			ll.vertical = true;
		}

		{ // story
			uint32 panelName;
			{ // panel
				entity *e = ents->createUnique();
				panelName = e->name();
				CAGE_COMPONENT_GUI(spoiler, spoiler, e);
				spoiler.collapsed = openPanel != 1;
				spoiler.collapsesSiblings = true;
				CAGE_COMPONENT_GUI(parent, parent, e);
				parent.parent = layoutName;
				parent.order = 1;
				CAGE_COMPONENT_GUI(text, text, e);
				text.assetName = hashString("degrid/languages/internationalized.textpack");
				text.textName = hashString("gui/paused/story");
				CAGE_COMPONENT_GUI(scrollbars, sc, e);
				sc.alignment = vec2(0.5, 0.5);
				CAGE_COMPONENT_GUI(layoutLine, ll, e);
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
				entity *label = gui()->entities()->createUnique();
				CAGE_COMPONENT_GUI(parent, parent, label);
				parent.parent = panelName;
				parent.order = idx;
				CAGE_COMPONENT_GUI(label, lab, label);
				CAGE_COMPONENT_GUI(text, txt, label);
				txt.assetName = hashString("degrid/languages/internationalized.textpack");
				txt.textName = textNames[idx];
				idx++;
			}
		}

		{ // bosses
			uint32 panelName;
			{ // panel
				entity *e = ents->createUnique();
				panelName = e->name();
				CAGE_COMPONENT_GUI(spoiler, spoiler, e);
				spoiler.collapsed = openPanel != 2;
				spoiler.collapsesSiblings = true;
				CAGE_COMPONENT_GUI(parent, parent, e);
				parent.parent = layoutName;
				parent.order = 2;
				CAGE_COMPONENT_GUI(text, text, e);
				text.assetName = hashString("degrid/languages/internationalized.textpack");
				text.textName = hashString("gui/paused/bosses");
				CAGE_COMPONENT_GUI(scrollbars, sc, e);
				sc.alignment = vec2(0.5, 0.5);
				CAGE_COMPONENT_GUI(layoutLine, ll, e);
				ll.vertical = false;
			}

			for (uint32 i = 0; i < bossesTotalCount; i++)
			{
				uint32 pn;
				{
					entity *e = ents->createUnique();
					pn = e->name();
					CAGE_COMPONENT_GUI(panel, panel, e);
					CAGE_COMPONENT_GUI(parent, parent, e);
					parent.parent = panelName;
					parent.order = i;
					CAGE_COMPONENT_GUI(text, text, e);
					text.assetName = hashString("degrid/languages/internationalized.textpack");
					text.textName = i < achievements.bosses ? hashString((string() + "achievement/boss-" + i).c_str()) : hashString("achievement/boss-unknown");
				}
				{
					entity *e = ents->createUnique();
					CAGE_COMPONENT_GUI(label, label, e);
					CAGE_COMPONENT_GUI(parent, parent, e);
					parent.parent = pn;
					parent.order = 1;
					CAGE_COMPONENT_GUI(image, img, e);
					img.textureName = i < achievements.bosses ? hashString((string() + "degrid/boss/icon/" + i + ".png").c_str()) : hashString("degrid/boss/icon/unknown.png");
				}
				if (game.defeatedBosses > i)
				{
					entity *e = ents->createUnique();
					CAGE_COMPONENT_GUI(label, label, e);
					CAGE_COMPONENT_GUI(parent, parent, e);
					parent.parent = pn;
					parent.order = 2;
					CAGE_COMPONENT_GUI(image, img, e);
					img.textureName = hashString("degrid/boss/icon/defeated.png");
				}
			}
		}

		{ // market
			uint32 marketName;
			{ // panel
				entity *e = ents->createUnique();
				marketName = e->name();
				CAGE_COMPONENT_GUI(spoiler, spoiler, e);
				spoiler.collapsed = openPanel != 3;
				spoiler.collapsesSiblings = true;
				CAGE_COMPONENT_GUI(parent, parent, e);
				parent.parent = layoutName;
				parent.order = 3;
				CAGE_COMPONENT_GUI(text, text, e);
				text.assetName = hashString("degrid/languages/internationalized.textpack");
				text.textName = hashString("gui/paused/market");
				CAGE_COMPONENT_GUI(scrollbars, sc, e);
				sc.alignment = vec2(0.5, 0.5);
				CAGE_COMPONENT_GUI(layoutLine, ll, e);
				ll.vertical = true;
			}

			{ // permanent powerup limit
				entity *e = ents->createUnique();
				CAGE_COMPONENT_GUI(parent, parent, e);
				parent.parent = marketName;
				parent.order = 1;
				CAGE_COMPONENT_GUI(text, text, e);
				text.assetName = hashString("degrid/languages/internationalized.textpack");
				text.textName = hashString("gui/paused/permanentLimit");
				text.value = string() + currentPermanentPowerups() + "|" + permanentPowerupLimit();
				CAGE_COMPONENT_GUI(label, but, e);
				CAGE_COMPONENT_GUI(textFormat, tf, e);
				tf.align = textAlignEnum::Center;
			}

			uint32 panelName;
			{ // permanent powerup market

				entity *e = ents->createUnique();
				panelName = e->name();
				CAGE_COMPONENT_GUI(parent, parent, e);
				parent.parent = marketName;
				parent.order = 2;
				CAGE_COMPONENT_GUI(layoutTable, lt, e);
				lt.sections = 4;
			}

			{ // header
				{ // label
					entity *e = ents->createUnique();
					CAGE_COMPONENT_GUI(parent, parent, e);
					parent.parent = panelName;
					parent.order = -10;
				}
				{ // count
					entity *e = ents->createUnique();
					CAGE_COMPONENT_GUI(parent, parent, e);
					parent.parent = panelName;
					parent.order = -9;
					CAGE_COMPONENT_GUI(label, but, e);
					CAGE_COMPONENT_GUI(text, text, e);
					text.assetName = hashString("degrid/languages/internationalized.textpack");
					text.textName = hashString("gui/paused/count");
				}
				{ // sell
					entity *e = ents->createUnique();
					CAGE_COMPONENT_GUI(parent, parent, e);
					parent.parent = panelName;
					parent.order = -8;
					CAGE_COMPONENT_GUI(text, text, e);
					text.assetName = hashString("degrid/languages/internationalized.textpack");
					text.textName = hashString("gui/paused/sell");
					CAGE_COMPONENT_GUI(label, but, e);
					CAGE_COMPONENT_GUI(textFormat, tf, e);
					tf.align = textAlignEnum::Center;
				}
				{ // buy
					entity *e = ents->createUnique();
					CAGE_COMPONENT_GUI(parent, parent, e);
					parent.parent = panelName;
					parent.order = -7;
					CAGE_COMPONENT_GUI(text, text, e);
					text.assetName = hashString("degrid/languages/internationalized.textpack");
					text.textName = hashString("gui/paused/buy");
					CAGE_COMPONENT_GUI(label, but, e);
					CAGE_COMPONENT_GUI(textFormat, tf, e);
					tf.align = textAlignEnum::Center;
				}
			}

			for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
			{
				bool anyBuy = canAddPermanentPowerup();
				if (powerupMode[i] == 2)
				{
					{ // label
						entity *e = ents->create(1000 + i * 4 + 0);
						CAGE_COMPONENT_GUI(parent, parent, e);
						parent.parent = panelName;
						parent.order = i * 4 + 0;
						CAGE_COMPONENT_GUI(text, text, e);
						text.assetName = hashString("degrid/languages/internationalized.textpack");
						text.textName = textNames[i];
						CAGE_COMPONENT_GUI(label, but, e);
					}
					{ // count
						entity *e = ents->create(1000 + i * 4 + 1);
						CAGE_COMPONENT_GUI(parent, parent, e);
						parent.parent = panelName;
						parent.order = i * 4 + 1;
						CAGE_COMPONENT_GUI(text, text, e);
						text.value = game.powerups[i];
						CAGE_COMPONENT_GUI(label, but, e);
						CAGE_COMPONENT_GUI(textFormat, format, e);
						format.align = textAlignEnum::Center;
					}
					{ // sell
						entity *e = ents->create(1000 + i * 4 + 2);
						CAGE_COMPONENT_GUI(parent, parent, e);
						parent.parent = panelName;
						parent.order = i * 4 + 2;
						CAGE_COMPONENT_GUI(text, text, e);
						text.value = basePermanentPowerupSellPrice;
						CAGE_COMPONENT_GUI(button, but, e);
						if (game.powerups[i] == 0)
						{
							CAGE_COMPONENT_GUI(widgetState, ws, e);
							ws.disabled = true;
						}
					}
					{ // buy
						entity *e = ents->create(1000 + i * 4 + 3);
						CAGE_COMPONENT_GUI(parent, parent, e);
						parent.parent = panelName;
						parent.order = i * 4 + 3;
						CAGE_COMPONENT_GUI(text, text, e);
						text.value = basePermanentPowerupBuyPrice * game.buyPriceMultiplier;
						CAGE_COMPONENT_GUI(button, but, e);
						if (!anyBuy || game.money < basePermanentPowerupBuyPrice * game.buyPriceMultiplier)
						{
							CAGE_COMPONENT_GUI(widgetState, ws, e);
							ws.disabled = true;
						}
					}
				}
			}
		}
	}

	void makeTheGuiPlaying()
	{
		entityManager *ents = gui()->entities();

		{
			CAGE_COMPONENT_GUI(scrollbars, sc, ents->get(12));
			sc.alignment = vec2(0.5, 0);
		}

		uint32 layoutName;
		{ // layout
			entity *e = ents->createUnique();
			layoutName = e->name();
			CAGE_COMPONENT_GUI(parent, parent, e);
			parent.parent = 12;
			CAGE_COMPONENT_GUI(layoutLine, ll, e);
			ll.vertical = true;
		}

		{ // announcements
			for (auto a : enumerate(announcements))
			{
				entity *panel = ents->createUnique();
				{
					CAGE_COMPONENT_GUI(panel, panel2, panel);
					CAGE_COMPONENT_GUI(parent, parent, panel);
					parent.parent = layoutName;
					parent.order = numeric_cast<sint32>(a.cnt) + 1;
					CAGE_COMPONENT_GUI(text, txt, panel);
					txt.assetName = hashString("degrid/languages/internationalized.textpack");
					txt.textName = a->headingName;
					CAGE_COMPONENT_GUI(textFormat, format, panel);
					format.size = 20;
					CAGE_COMPONENT_GUI(layoutLine, ll, panel);
					ll.vertical = true;
				}

				{ // description
					entity *label = ents->createUnique();
					CAGE_COMPONENT_GUI(parent, parent, label);
					parent.parent = panel->name();
					CAGE_COMPONENT_GUI(label, control, label);
					CAGE_COMPONENT_GUI(text, txt, label);
					txt.assetName = hashString("degrid/languages/internationalized.textpack");
					txt.textName = a->descriptionName;
					CAGE_COMPONENT_GUI(textFormat, format, label);
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
		entityManager *ents = gui()->entities();
		guiEvent.bind<&guiFunction>();
		guiEvent.attach(gui()->widgetEvent);

		{ // base stats
			entity *table = ents->createUnique();
			{
				CAGE_COMPONENT_GUI(parent, parent, table);
				parent.parent = 10;
				CAGE_COMPONENT_GUI(layoutTable, layout, table);
				CAGE_COMPONENT_GUI(panel, gb, table);
			}
			uint32 index = 1;

			{ // life label
				entity *label = gui()->entities()->createUnique();
				CAGE_COMPONENT_GUI(parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(label, control, label);
				CAGE_COMPONENT_GUI(text, text, label);
				text.assetName = hashString("degrid/languages/internationalized.textpack");
				text.textName = hashString("gui/game/life");
				CAGE_COMPONENT_GUI(textFormat, format, label);
				format.color = vec3(1, 0, 0);
				format.size = 20;
			}

			{ // life value
				entity *label = gui()->entities()->create(100);
				CAGE_COMPONENT_GUI(parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(label, control, label);
				CAGE_COMPONENT_GUI(text, text, label);
				CAGE_COMPONENT_GUI(textFormat, format, label);
				format.align = textAlignEnum::Right;
				format.color = vec3(1, 0, 0);
				format.size = 20;
			}

			{ // money label
				entity *label = gui()->entities()->createUnique();
				CAGE_COMPONENT_GUI(parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(label, control, label);
				CAGE_COMPONENT_GUI(text, text, label);
				text.assetName = hashString("degrid/languages/internationalized.textpack");
				text.textName = hashString("gui/game/money");
				CAGE_COMPONENT_GUI(textFormat, format, label);
				format.color = vec3(1, 1, 0);
				format.size = 20;
			}

			{ // money value
				entity *label = gui()->entities()->create(101);
				CAGE_COMPONENT_GUI(parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(label, control, label);
				CAGE_COMPONENT_GUI(text, text, label);
				CAGE_COMPONENT_GUI(textFormat, format, label);
				format.align = textAlignEnum::Right;
				format.color = vec3(1, 1, 0);
				format.size = 20;
			}

			{ // score label
				entity *label = gui()->entities()->createUnique();
				CAGE_COMPONENT_GUI(parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(label, control, label);
				CAGE_COMPONENT_GUI(text, text, label);
				text.assetName = hashString("degrid/languages/internationalized.textpack");
				text.textName = hashString("gui/game/score");
				CAGE_COMPONENT_GUI(textFormat, format, label);
				format.color = vec3(0, 1, 0);
				format.size = 20;
			}

			{ // score value
				entity *label = gui()->entities()->create(102);
				CAGE_COMPONENT_GUI(parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(label, control, label);
				CAGE_COMPONENT_GUI(text, text, label);
				CAGE_COMPONENT_GUI(textFormat, format, label);
				format.align = textAlignEnum::Right;
				format.color = vec3(0, 1, 0);
				format.size = 20;
			}
		}

		{ // collectible powerups
			entity *table = ents->createUnique();
			{
				CAGE_COMPONENT_GUI(parent, parent, table);
				parent.parent = 14;
				CAGE_COMPONENT_GUI(layoutTable, layout, table);
				CAGE_COMPONENT_GUI(panel, gb, table);
			}
			uint32 index = 1;

			for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
			{
				if (powerupMode[i] == 0)
				{
					{ // label
						entity *label = gui()->entities()->createUnique();
						CAGE_COMPONENT_GUI(parent, parent, label);
						parent.parent = table->name();
						parent.order = index++;
						CAGE_COMPONENT_GUI(label, control, label);
						CAGE_COMPONENT_GUI(text, text, label);
						text.assetName = hashString("degrid/languages/internationalized.textpack");
						text.textName = textNames[i];
					}

					{ // value
						entity *label = gui()->entities()->create(200 + i);
						CAGE_COMPONENT_GUI(parent, parent, label);
						parent.parent = table->name();
						parent.order = index++;
						CAGE_COMPONENT_GUI(label, control, label);
						CAGE_COMPONENT_GUI(text, text, label);
						CAGE_COMPONENT_GUI(textFormat, format, label);
						format.align = textAlignEnum::Right;
					}
				}
			}
		}

		{ // timed
			entity *table = ents->createUnique();
			{
				CAGE_COMPONENT_GUI(parent, parent, table);
				parent.parent = 11;
				CAGE_COMPONENT_GUI(layoutTable, layout, table);
				CAGE_COMPONENT_GUI(panel, gb, table);
			}
			uint32 index = 1;

			for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
			{
				if (powerupMode[i] == 1)
				{
					{ // label
						entity *label = gui()->entities()->createUnique();
						CAGE_COMPONENT_GUI(parent, parent, label);
						parent.parent = table->name();
						parent.order = index++;
						CAGE_COMPONENT_GUI(label, control, label);
						CAGE_COMPONENT_GUI(text, text, label);
						text.assetName = hashString("degrid/languages/internationalized.textpack");
						text.textName = textNames[i];
					}

					{ // value
						entity *label = gui()->entities()->create(200 + i);
						CAGE_COMPONENT_GUI(parent, parent, label);
						parent.parent = table->name();
						parent.order = index++;
						CAGE_COMPONENT_GUI(label, control, label);
						CAGE_COMPONENT_GUI(text, text, label);
						CAGE_COMPONENT_GUI(textFormat, format, label);
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
			CAGE_COMPONENT_GUI(text, txt, gui()->entities()->get(100));
			txt.value = numeric_cast<uint32>(max(0, game.life));
		}
		{ // money
			CAGE_COMPONENT_GUI(text, txt, gui()->entities()->get(101));
			txt.value = game.money;
		}
		{ // score
			CAGE_COMPONENT_GUI(text, txt, gui()->entities()->get(102));
			txt.value = game.score;
		}

		for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
		{
			if (!gui()->entities()->has(200 + i))
				continue;
			CAGE_COMPONENT_GUI(text, txt, gui()->entities()->get(200 + i));
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
