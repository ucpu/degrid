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

	InputListener<InputClassEnum::GuiWidget, InputGuiWidget, bool> guiEvent;

	void makeTheGui(uint32 mode = 0);

	bool guiFunction(InputGuiWidget in)
	{
		switch (in.widget)
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
				if (in.widget == 1000 + i * 4 + 2) // buy
				{
					CAGE_ASSERT(canAddPermanentPowerup());
					CAGE_ASSERT(game.money >= PowerupBuyPriceBase * game.buyPriceMultiplier);
					game.powerups[i]++;
					game.money -= PowerupBuyPriceBase * game.buyPriceMultiplier;
					game.buyPriceMultiplier++;
					engineGuiManager()->invalidateInputs();
					makeTheGui(3);
					return true;
				}
				if (in.widget == 1000 + i * 4 + 3) // sell
				{
					CAGE_ASSERT(game.powerups[i] > 0);
					game.powerups[i]--;
					game.money += PowerupSellPriceBase * (game.defeatedBosses + 1);
					engineGuiManager()->invalidateInputs();
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
		EntityManager *ents = engineGuiEntities();

		{
			GuiLayoutLineComponent &ll = ents->get(15)->value<GuiLayoutLineComponent>();
			ll.vertical = true;
		}

		{ // continue button
			Entity *but = ents->create(501);
			GuiButtonComponent &control = but->value<GuiButtonComponent>();
			GuiTextComponent &txt = but->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/paused/continue");
			GuiParentComponent &parent = but->value<GuiParentComponent>();
			parent.parent = 15;
			parent.order = 1;
			GuiTextFormatComponent &format = but->value<GuiTextFormatComponent>();
			format.color = RedPillColor;
		}

		{ // end game button
			Entity *but = ents->create(500);
			GuiButtonComponent &control = but->value<GuiButtonComponent>();
			GuiTextComponent &txt = but->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/paused/giveup");
			GuiParentComponent &parent = but->value<GuiParentComponent>();
			parent.parent = 15;
			parent.order = 2;
			GuiTextFormatComponent &format = but->value<GuiTextFormatComponent>();
			format.color = BluePillColor;
		}

		uint32 layoutName;
		{ // layout
			Entity *e = ents->createUnique();
			layoutName = e->name();
			GuiParentComponent &parent = e->value<GuiParentComponent>();
			parent.parent = 12;
			GuiLayoutLineComponent &ll = e->value<GuiLayoutLineComponent>();
			ll.vertical = true;
		}

		{ // story
			uint32 panelName;
			{ // panel
				Entity *e = ents->createUnique();
				panelName = e->name();
				GuiSpoilerComponent &spoiler = e->value<GuiSpoilerComponent>();
				spoiler.collapsed = openPanel != 1;
				spoiler.collapsesSiblings = true;
				GuiParentComponent &parent = e->value<GuiParentComponent>();
				parent.parent = layoutName;
				parent.order = 1;
				GuiTextComponent &text = e->value<GuiTextComponent>();
				text.assetName = HashString("degrid/languages/internationalized.textpack");
				text.textName = HashString("gui/paused/story");
				GuiScrollbarsComponent &sc = e->value<GuiScrollbarsComponent>();
				sc.alignment = Vec2(0.5, 0.5);
				GuiLayoutLineComponent &ll = e->value<GuiLayoutLineComponent>();
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
				Entity *label = engineGuiEntities()->createUnique();
				GuiParentComponent &parent = label->value<GuiParentComponent>();
				parent.parent = panelName;
				parent.order = idx;
				GuiLabelComponent &lab = label->value<GuiLabelComponent>();
				GuiTextComponent &txt = label->value<GuiTextComponent>();
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
				GuiSpoilerComponent &spoiler = e->value<GuiSpoilerComponent>();
				spoiler.collapsed = openPanel != 2;
				spoiler.collapsesSiblings = true;
				GuiParentComponent &parent = e->value<GuiParentComponent>();
				parent.parent = layoutName;
				parent.order = 2;
				GuiTextComponent &text = e->value<GuiTextComponent>();
				text.assetName = HashString("degrid/languages/internationalized.textpack");
				text.textName = HashString("gui/paused/bosses");
				GuiScrollbarsComponent &sc = e->value<GuiScrollbarsComponent>();
				sc.alignment = Vec2(0.5, 0.5);
				GuiLayoutLineComponent &ll = e->value<GuiLayoutLineComponent>();
				ll.vertical = false;
			}

			for (uint32 i = 0; i < BossesTotalCount; i++)
			{
				uint32 pn;
				{
					Entity *e = ents->createUnique();
					pn = e->name();
					GuiPanelComponent &panel = e->value<GuiPanelComponent>();
					GuiParentComponent &parent = e->value<GuiParentComponent>();
					parent.parent = panelName;
					parent.order = i;
					GuiTextComponent &text = e->value<GuiTextComponent>();
					text.assetName = HashString("degrid/languages/internationalized.textpack");
					text.textName = i < achievements.bosses ? HashString(String(Stringizer() + "achievement/boss-" + i)) : HashString("achievement/boss-unknown");
				}
				{
					Entity *e = ents->createUnique();
					GuiLabelComponent &label = e->value<GuiLabelComponent>();
					GuiParentComponent &parent = e->value<GuiParentComponent>();
					parent.parent = pn;
					parent.order = 1;
					GuiImageComponent &img = e->value<GuiImageComponent>();
					img.textureName = i < achievements.bosses ? HashString(String(Stringizer() + "degrid/boss/icon/" + i + ".png")) : HashString("degrid/boss/icon/unknown.png");
				}
				if (game.defeatedBosses > i)
				{
					Entity *e = ents->createUnique();
					GuiLabelComponent &label = e->value<GuiLabelComponent>();
					GuiParentComponent &parent = e->value<GuiParentComponent>();
					parent.parent = pn;
					parent.order = 2;
					GuiImageComponent &img = e->value<GuiImageComponent>();
					img.textureName = HashString("degrid/boss/icon/defeated.png");
				}
			}
		}

		{ // market
			uint32 marketName;
			{ // panel
				Entity *e = ents->createUnique();
				marketName = e->name();
				GuiSpoilerComponent &spoiler = e->value<GuiSpoilerComponent>();
				spoiler.collapsed = openPanel != 3;
				spoiler.collapsesSiblings = true;
				GuiParentComponent &parent = e->value<GuiParentComponent>();
				parent.parent = layoutName;
				parent.order = 3;
				GuiTextComponent &text = e->value<GuiTextComponent>();
				text.assetName = HashString("degrid/languages/internationalized.textpack");
				text.textName = HashString("gui/paused/market");
				GuiScrollbarsComponent &sc = e->value<GuiScrollbarsComponent>();
				sc.alignment = Vec2(0.5, 0.5);
				GuiLayoutLineComponent &ll = e->value<GuiLayoutLineComponent>();
				ll.vertical = true;
			}

			{ // permanent powerup limit
				Entity *e = ents->createUnique();
				GuiParentComponent &parent = e->value<GuiParentComponent>();
				parent.parent = marketName;
				parent.order = 1;
				GuiTextComponent &text = e->value<GuiTextComponent>();
				text.assetName = HashString("degrid/languages/internationalized.textpack");
				text.textName = HashString("gui/paused/permanentLimit");
				text.value = Stringizer() + currentPermanentPowerups() + "|" + permanentPowerupLimit();
				GuiLabelComponent &but = e->value<GuiLabelComponent>();
				GuiTextFormatComponent &tf = e->value<GuiTextFormatComponent>();
				tf.align = TextAlignEnum::Center;
			}

			uint32 panelName;
			{ // permanent powerup market

				Entity *e = ents->createUnique();
				panelName = e->name();
				GuiParentComponent &parent = e->value<GuiParentComponent>();
				parent.parent = marketName;
				parent.order = 2;
				GuiLayoutTableComponent &lt = e->value<GuiLayoutTableComponent>();
				lt.sections = 4;
			}

			{ // header
				{ // label
					Entity *e = ents->createUnique();
					GuiParentComponent &parent = e->value<GuiParentComponent>();
					parent.parent = panelName;
					parent.order = -10;
				}
				{ // count
					Entity *e = ents->createUnique();
					GuiParentComponent &parent = e->value<GuiParentComponent>();
					parent.parent = panelName;
					parent.order = -9;
					GuiLabelComponent &but = e->value<GuiLabelComponent>();
					GuiTextComponent &text = e->value<GuiTextComponent>();
					text.assetName = HashString("degrid/languages/internationalized.textpack");
					text.textName = HashString("gui/paused/count");
				}
				{ // buy
					Entity *e = ents->createUnique();
					GuiParentComponent &parent = e->value<GuiParentComponent>();
					parent.parent = panelName;
					parent.order = -8;
				}
				{ // sell
					Entity *e = ents->createUnique();
					GuiParentComponent &parent = e->value<GuiParentComponent>();
					parent.parent = panelName;
					parent.order = -7;
				}
			}

			{ // footer
				{ // label
					Entity *e = ents->createUnique();
					GuiParentComponent &parent = e->value<GuiParentComponent>();
					parent.parent = panelName;
					parent.order = 1000;
					GuiLabelComponent &but = e->value<GuiLabelComponent>();
					GuiTextComponent &text = e->value<GuiTextComponent>();
					text.assetName = HashString("degrid/languages/internationalized.textpack");
					text.textName = HashString("gui/paused/prices");
				}
				{ // empty
					Entity *e = ents->createUnique();
					GuiParentComponent &parent = e->value<GuiParentComponent>();
					parent.parent = panelName;
					parent.order = 1001;
				}
				{ // buy
					Entity *e = ents->createUnique();
					GuiParentComponent &parent = e->value<GuiParentComponent>();
					parent.parent = panelName;
					parent.order = 1002;
					GuiTextComponent &text = e->value<GuiTextComponent>();
					text.value = Stringizer() + (PowerupBuyPriceBase * game.buyPriceMultiplier);
					GuiLabelComponent &but = e->value<GuiLabelComponent>();
					GuiTextFormatComponent &tf = e->value<GuiTextFormatComponent>();
					tf.align = TextAlignEnum::Center;
				}
				{ // sell
					Entity *e = ents->createUnique();
					GuiParentComponent &parent = e->value<GuiParentComponent>();
					parent.parent = panelName;
					parent.order = 1003;
					GuiTextComponent &text = e->value<GuiTextComponent>();
					text.value = Stringizer() + (PowerupSellPriceBase * (game.defeatedBosses + 1));
					GuiLabelComponent &but = e->value<GuiLabelComponent>();
					GuiTextFormatComponent &tf = e->value<GuiTextFormatComponent>();
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
						GuiParentComponent &parent = e->value<GuiParentComponent>();
						parent.parent = panelName;
						parent.order = i * 4 + 0;
						GuiTextComponent &text = e->value<GuiTextComponent>();
						text.assetName = HashString("degrid/languages/internationalized.textpack");
						text.textName = TextNames[i];
						GuiLabelComponent &but = e->value<GuiLabelComponent>();
					}
					{ // count
						Entity *e = ents->create(1000 + i * 4 + 1);
						GuiParentComponent &parent = e->value<GuiParentComponent>();
						parent.parent = panelName;
						parent.order = i * 4 + 1;
						GuiTextComponent &text = e->value<GuiTextComponent>();
						text.value = Stringizer() + game.powerups[i];
						GuiLabelComponent &but = e->value<GuiLabelComponent>();
						GuiTextFormatComponent &format = e->value<GuiTextFormatComponent>();
						format.align = TextAlignEnum::Center;
					}
					{ // buy
						Entity *e = ents->create(1000 + i * 4 + 2);
						GuiParentComponent &parent = e->value<GuiParentComponent>();
						parent.parent = panelName;
						parent.order = i * 4 + 2;
						GuiTextComponent &text = e->value<GuiTextComponent>();
						text.assetName = HashString("degrid/languages/internationalized.textpack");
						text.textName = HashString("gui/paused/buy");
						GuiButtonComponent &but = e->value<GuiButtonComponent>();
						if (!anyBuy || game.money < PowerupBuyPriceBase * game.buyPriceMultiplier)
						{
							GuiWidgetStateComponent &ws = e->value<GuiWidgetStateComponent>();
							ws.disabled = true;
						}
					}
					{ // sell
						Entity *e = ents->create(1000 + i * 4 + 3);
						GuiParentComponent &parent = e->value<GuiParentComponent>();
						parent.parent = panelName;
						parent.order = i * 4 + 3;
						GuiTextComponent &text = e->value<GuiTextComponent>();
						text.assetName = HashString("degrid/languages/internationalized.textpack");
						text.textName = HashString("gui/paused/sell");
						GuiButtonComponent &but = e->value<GuiButtonComponent>();
						if (game.powerups[i] == 0)
						{
							GuiWidgetStateComponent &ws = e->value<GuiWidgetStateComponent>();
							ws.disabled = true;
						}
					}
				}
			}
		}
	}

	void makeTheGuiPlaying()
	{
		EntityManager *ents = engineGuiEntities();

		{
			GuiScrollbarsComponent &sc = ents->get(12)->value<GuiScrollbarsComponent>();
			sc.alignment = Vec2(0.5, 0);
		}

		uint32 layoutName;
		{ // layout
			Entity *e = ents->createUnique();
			layoutName = e->name();
			GuiParentComponent &parent = e->value<GuiParentComponent>();
			parent.parent = 12;
			GuiLayoutLineComponent &ll = e->value<GuiLayoutLineComponent>();
			ll.vertical = true;
		}

		{ // announcements
			for (auto a : enumerate(announcements))
			{
				Entity *panel = ents->createUnique();
				{
					GuiPanelComponent &panel2 = panel->value<GuiPanelComponent>();
					GuiParentComponent &parent = panel->value<GuiParentComponent>();
					parent.parent = layoutName;
					parent.order = numeric_cast<sint32>(a.index) + 1;
					GuiTextComponent &txt = panel->value<GuiTextComponent>();
					txt.assetName = HashString("degrid/languages/internationalized.textpack");
					txt.textName = a->headingName;
					GuiTextFormatComponent &format = panel->value<GuiTextFormatComponent>();
					format.size = 20;
					GuiLayoutLineComponent &ll = panel->value<GuiLayoutLineComponent>();
					ll.vertical = true;
				}

				{ // description
					Entity *label = ents->createUnique();
					GuiParentComponent &parent = label->value<GuiParentComponent>();
					parent.parent = panel->name();
					GuiLabelComponent &control = label->value<GuiLabelComponent>();
					GuiTextComponent &txt = label->value<GuiTextComponent>();
					txt.assetName = HashString("degrid/languages/internationalized.textpack");
					txt.textName = a->descriptionName;
					GuiTextFormatComponent &format = label->value<GuiTextFormatComponent>();
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
		EntityManager *ents = engineGuiEntities();
		guiEvent.bind<&guiFunction>();
		guiEvent.attach(engineGuiManager()->widgetEvent);

		{ // base stats
			Entity *table = ents->createUnique();
			{
				GuiParentComponent &parent = table->value<GuiParentComponent>();
				parent.parent = 10;
				GuiLayoutTableComponent &layout = table->value<GuiLayoutTableComponent>();
				GuiPanelComponent &gb = table->value<GuiPanelComponent>();
			}
			uint32 index = 1;

			{ // life label
				Entity *label = engineGuiEntities()->createUnique();
				GuiParentComponent &parent = label->value<GuiParentComponent>();
				parent.parent = table->name();
				parent.order = index++;
				GuiLabelComponent &control = label->value<GuiLabelComponent>();
				GuiTextComponent &text = label->value<GuiTextComponent>();
				text.assetName = HashString("degrid/languages/internationalized.textpack");
				text.textName = HashString("gui/game/life");
				GuiTextFormatComponent &format = label->value<GuiTextFormatComponent>();
				format.color = Vec3(1, 0, 0);
				format.size = 20;
			}

			{ // life value
				Entity *label = engineGuiEntities()->create(100);
				GuiParentComponent &parent = label->value<GuiParentComponent>();
				parent.parent = table->name();
				parent.order = index++;
				GuiLabelComponent &control = label->value<GuiLabelComponent>();
				GuiTextComponent &text = label->value<GuiTextComponent>();
				GuiTextFormatComponent &format = label->value<GuiTextFormatComponent>();
				format.align = TextAlignEnum::Right;
				format.color = Vec3(1, 0, 0);
				format.size = 20;
			}

			{ // money label
				Entity *label = engineGuiEntities()->createUnique();
				GuiParentComponent &parent = label->value<GuiParentComponent>();
				parent.parent = table->name();
				parent.order = index++;
				GuiLabelComponent &control = label->value<GuiLabelComponent>();
				GuiTextComponent &text = label->value<GuiTextComponent>();
				text.assetName = HashString("degrid/languages/internationalized.textpack");
				text.textName = HashString("gui/game/money");
				GuiTextFormatComponent &format = label->value<GuiTextFormatComponent>();
				format.color = Vec3(1, 1, 0);
				format.size = 20;
			}

			{ // money value
				Entity *label = engineGuiEntities()->create(101);
				GuiParentComponent &parent = label->value<GuiParentComponent>();
				parent.parent = table->name();
				parent.order = index++;
				GuiLabelComponent &control = label->value<GuiLabelComponent>();
				GuiTextComponent &text = label->value<GuiTextComponent>();
				GuiTextFormatComponent &format = label->value<GuiTextFormatComponent>();
				format.align = TextAlignEnum::Right;
				format.color = Vec3(1, 1, 0);
				format.size = 20;
			}

			{ // score label
				Entity *label = engineGuiEntities()->createUnique();
				GuiParentComponent &parent = label->value<GuiParentComponent>();
				parent.parent = table->name();
				parent.order = index++;
				GuiLabelComponent &control = label->value<GuiLabelComponent>();
				GuiTextComponent &text = label->value<GuiTextComponent>();
				text.assetName = HashString("degrid/languages/internationalized.textpack");
				text.textName = HashString("gui/game/score");
				GuiTextFormatComponent &format = label->value<GuiTextFormatComponent>();
				format.color = Vec3(0, 1, 0);
				format.size = 20;
			}

			{ // score value
				Entity *label = engineGuiEntities()->create(102);
				GuiParentComponent &parent = label->value<GuiParentComponent>();
				parent.parent = table->name();
				parent.order = index++;
				GuiLabelComponent &control = label->value<GuiLabelComponent>();
				GuiTextComponent &text = label->value<GuiTextComponent>();
				GuiTextFormatComponent &format = label->value<GuiTextFormatComponent>();
				format.align = TextAlignEnum::Right;
				format.color = Vec3(0, 1, 0);
				format.size = 20;
			}
		}

		{ // collectible powerups
			Entity *table = ents->createUnique();
			{
				GuiParentComponent &parent = table->value<GuiParentComponent>();
				parent.parent = 14;
				GuiLayoutTableComponent &layout = table->value<GuiLayoutTableComponent>();
				GuiPanelComponent &gb = table->value<GuiPanelComponent>();
			}
			uint32 index = 1;

			for (uint32 i = 0; i < (uint32)PowerupTypeEnum::Total; i++)
			{
				if (PowerupMode[i] == 0)
				{
					{ // label
						Entity *label = engineGuiEntities()->createUnique();
						GuiParentComponent &parent = label->value<GuiParentComponent>();
						parent.parent = table->name();
						parent.order = index++;
						GuiLabelComponent &control = label->value<GuiLabelComponent>();
						GuiTextComponent &text = label->value<GuiTextComponent>();
						text.assetName = HashString("degrid/languages/internationalized.textpack");
						text.textName = TextNames[i];
					}

					{ // value
						Entity *label = engineGuiEntities()->create(200 + i);
						GuiParentComponent &parent = label->value<GuiParentComponent>();
						parent.parent = table->name();
						parent.order = index++;
						GuiLabelComponent &control = label->value<GuiLabelComponent>();
						GuiTextComponent &text = label->value<GuiTextComponent>();
						GuiTextFormatComponent &format = label->value<GuiTextFormatComponent>();
						format.align = TextAlignEnum::Right;
					}
				}
			}
		}

		{ // timed
			Entity *table = ents->createUnique();
			{
				GuiParentComponent &parent = table->value<GuiParentComponent>();
				parent.parent = 11;
				GuiLayoutTableComponent &layout = table->value<GuiLayoutTableComponent>();
				GuiPanelComponent &gb = table->value<GuiPanelComponent>();
			}
			uint32 index = 1;

			for (uint32 i = 0; i < (uint32)PowerupTypeEnum::Total; i++)
			{
				if (PowerupMode[i] == 1)
				{
					{ // label
						Entity *label = engineGuiEntities()->createUnique();
						GuiParentComponent &parent = label->value<GuiParentComponent>();
						parent.parent = table->name();
						parent.order = index++;
						GuiLabelComponent &control = label->value<GuiLabelComponent>();
						GuiTextComponent &text = label->value<GuiTextComponent>();
						text.assetName = HashString("degrid/languages/internationalized.textpack");
						text.textName = TextNames[i];
					}

					{ // value
						Entity *label = engineGuiEntities()->create(200 + i);
						GuiParentComponent &parent = label->value<GuiParentComponent>();
						parent.parent = table->name();
						parent.order = index++;
						GuiLabelComponent &control = label->value<GuiLabelComponent>();
						GuiTextComponent &text = label->value<GuiTextComponent>();
						GuiTextFormatComponent &format = label->value<GuiTextFormatComponent>();
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
			GuiTextComponent &txt = engineGuiEntities()->get(100)->value<GuiTextComponent>();
			txt.value = Stringizer() + numeric_cast<uint32>(max(0, game.life));
		}
		{ // money
			GuiTextComponent &txt = engineGuiEntities()->get(101)->value<GuiTextComponent>();
			txt.value = Stringizer() + game.money;
		}
		{ // score
			GuiTextComponent &txt = engineGuiEntities()->get(102)->value<GuiTextComponent>();
			txt.value = Stringizer() + game.score;
		}

		for (uint32 i = 0; i < (uint32)PowerupTypeEnum::Total; i++)
		{
			if (!engineGuiEntities()->has(200 + i))
				continue;
			GuiTextComponent &txt = engineGuiEntities()->get(200 + i)->value<GuiTextComponent>();
			switch (PowerupMode[i])
			{
			case 0: // collectibles
				txt.value = Stringizer() + game.powerups[i];
				break;
			case 1: // timed
				txt.value = Stringizer() + game.powerups[i] / 30;
				break;
			case 2: // permanent
				txt.value = Stringizer() + game.powerups[i];
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
