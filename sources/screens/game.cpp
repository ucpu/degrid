#include "screens.h"
#include "../game.h"

namespace
{
	void makeTheGui();

	eventListener<bool(uint32)> guiEvent;

	bool guiFunction(uint32 en)
	{
		switch (en)
		{
		case 500:
			game.life = 0;
			game.paused = false;
			return true;
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
		hashString("gui/game/puMultishot")
	};

	void makeTheGui()
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
				GUI_GET_COMPONENT(groupBox, gb, table);
				gb.type = groupBoxTypeEnum::Panel;
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

			{ // currency label
				entityClass *label = gui()->entities()->createUnique();
				GUI_GET_COMPONENT(parent, parent, label);
				parent.parent = table->name();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, label);
				GUI_GET_COMPONENT(text, text, label);
				text.assetName = hashString("grid/languages/internationalized.textpack");
				text.textName = hashString("gui/game/currency");
				GUI_GET_COMPONENT(textFormat, format, label);
				format.color = vec3(1, 1, 0);
				format.size = 20;
			}

			{ // currency value
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
				GUI_GET_COMPONENT(groupBox, gb, table);
				gb.type = groupBoxTypeEnum::Panel;
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

		{ // timed/permanent
			entityClass *table = ents->createUnique();
			{
				GUI_GET_COMPONENT(parent, parent, table);
				parent.parent = 11;
				GUI_GET_COMPONENT(layoutTable, layout, table);
				GUI_GET_COMPONENT(groupBox, gb, table);
				gb.type = groupBoxTypeEnum::Panel;
			}
			uint32 index = 1;

			for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
			{
				if (powerupMode[i] == (game.paused ? 2 : 1))
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

		if (!game.paused)
			return;

		{ // end game button
			entityClass *but = ents->create(500);
			GUI_GET_COMPONENT(button, control, but);
			GUI_GET_COMPONENT(text, txt, but);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/game/end");
			GUI_GET_COMPONENT(position, position, but);
			GUI_GET_COMPONENT(parent, parent, but);
			parent.parent = 15;
		}

		{ // paused
			entityClass *panel = ents->createUnique();
			GUI_GET_COMPONENT(groupBox, groupBox, panel);
			groupBox.type = groupBoxTypeEnum::Panel;
			GUI_GET_COMPONENT(position, position, panel);
			GUI_GET_COMPONENT(parent, parentPanel, panel);
			parentPanel.parent = 12;

			entityClass *label = ents->createUnique();
			GUI_GET_COMPONENT(parent, parent, label);
			parent.parent = panel->name();
			GUI_GET_COMPONENT(label, controlL, label);
			GUI_GET_COMPONENT(text, txt, label);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/game/pause");
			GUI_GET_COMPONENT(textFormat, format, label);
			format.size = 50;
		}
	}

	bool previousPaused = false;

	void engineUpdate()
	{
		if (game.gameOver || game.cinematic)
			return;

		if (game.paused != previousPaused)
		{
			previousPaused = game.paused;
			makeTheGui();
		}

		{ // life
			GUI_GET_COMPONENT(text, txt, gui()->entities()->get(100));
			txt.value = numeric_cast<uint32>(max(0, game.life));
		}
		{ // currency
			GUI_GET_COMPONENT(text, txt, gui()->entities()->get(101));
			txt.value = game.currency;
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
	previousPaused = true;
	game.cinematic = false;
	gameStartEvent().dispatch();
}
