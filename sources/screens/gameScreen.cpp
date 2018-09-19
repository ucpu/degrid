#include "screens.h"
#include "../game.h"

#include <cage-core/entities.h>
#include <cage-core/assets.h>
#include <cage-core/textPack.h>

namespace
{
	void makeTheGui();

	eventListener<bool(uint32)> guiEvent;

	bool guiFunction(uint32 en)
	{
		switch (en)
		{
		case 500:
			player.life = 0;
			player.paused = false;
			return true;
		}
		if (en <= (uint32)powerupTypeEnum::Total)
		{
			player.powerups[en - 1]--;
			makeTheGui();
			return true;
		}
		return false;
	}

	void makeTheGui()
	{
		eraseGui();
		entityManagerClass *ents = gui()->entities();
		guiEvent.bind<&guiFunction>();
		guiEvent.attach(gui()->widgetEvent);

		{ // life
			entityClass *lifeLabel = gui()->entities()->newEntity(100);
			GUI_GET_COMPONENT(label, control, lifeLabel);
			GUI_GET_COMPONENT(text, text, lifeLabel);
			text.value = "life";
			GUI_GET_COMPONENT(textFormat, format, lifeLabel);
			format.align = textAlignEnum::Center;
			format.color = vec3(1, 0, 0);
			format.size = 20;
			GUI_GET_COMPONENT(position, pos, lifeLabel);
			pos.anchor = vec2(0.5, 0);
			pos.position.values[0] = 1.0 / 6.0;
			pos.position.units[0] = unitEnum::ScreenWidth;
			pos.position.values[1] = 0;
			pos.position.units[1] = unitEnum::ScreenHeight;
		}

		{ // bombs
			entityClass *lifeLabel = gui()->entities()->newEntity(1);
			GUI_GET_COMPONENT(label, control, lifeLabel);
			GUI_GET_COMPONENT(text, text, lifeLabel);
			(void)text;
			GUI_GET_COMPONENT(textFormat, format, lifeLabel);
			format.align = textAlignEnum::Center;
			format.color = vec3(1, 1, 1);
			format.size = 20;
			GUI_GET_COMPONENT(position, pos, lifeLabel);
			pos.anchor = vec2(0.5, 0);
			pos.position.values[0] = 2.0 / 6.0;
			pos.position.units[0] = unitEnum::ScreenWidth;
			pos.position.values[1] = 0;
			pos.position.units[1] = unitEnum::ScreenHeight;
		}

		{ // turrets
			entityClass *lifeLabel = gui()->entities()->newEntity(2);
			GUI_GET_COMPONENT(label, control, lifeLabel);
			GUI_GET_COMPONENT(text, text, lifeLabel);
			(void)text;
			GUI_GET_COMPONENT(textFormat, format, lifeLabel);
			format.align = textAlignEnum::Center;
			format.color = vec3(1, 1, 1);
			format.size = 20;
			GUI_GET_COMPONENT(position, pos, lifeLabel);
			pos.anchor = vec2(0.5, 0);
			pos.position.values[0] = 3.0 / 6.0;
			pos.position.units[0] = unitEnum::ScreenWidth;
			pos.position.values[1] = 0;
			pos.position.units[1] = unitEnum::ScreenHeight;
		}

		{ // decoys
			entityClass *lifeLabel = gui()->entities()->newEntity(3);
			GUI_GET_COMPONENT(label, control, lifeLabel);
			GUI_GET_COMPONENT(text, text, lifeLabel);
			(void)text;
			GUI_GET_COMPONENT(textFormat, format, lifeLabel);
			format.align = textAlignEnum::Center;
			format.color = vec3(1, 1, 1);
			format.size = 20;
			GUI_GET_COMPONENT(position, pos, lifeLabel);
			pos.anchor = vec2(0.5, 0);
			pos.position.values[0] = 4.0 / 6.0;
			pos.position.units[0] = unitEnum::ScreenWidth;
			pos.position.values[1] = 0;
			pos.position.units[1] = unitEnum::ScreenHeight;
		}

		{ // score
			entityClass *scoreLabel = gui()->entities()->newEntity(101);
			GUI_GET_COMPONENT(label, control, scoreLabel);
			GUI_GET_COMPONENT(text, text, scoreLabel);
			text.value = "score";
			GUI_GET_COMPONENT(textFormat, format, scoreLabel);
			format.align = textAlignEnum::Center;
			format.color = vec3(0, 1, 0);
			format.size = 20;
			GUI_GET_COMPONENT(position, pos, scoreLabel);
			pos.anchor = vec2(0.5, 0);
			pos.position.values[0] = 5.0 / 6.0;
			pos.position.units[0] = unitEnum::ScreenWidth;
			pos.position.values[1] = 0;
			pos.position.units[1] = unitEnum::ScreenHeight;
		}

		entityClass *column = ents->newUniqueEntity();
		{
			GUI_GET_COMPONENT(layoutLine, layout, column);
			layout.vertical = true;
			GUI_GET_COMPONENT(position, position, column);
			position.anchor = vec2(0, 1);
			position.position.values[0] = 0;
			position.position.units[0] = unitEnum::ScreenWidth;
			position.position.values[1] = 1;
			position.position.units[1] = unitEnum::ScreenHeight;
		}

		for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
		{
			switch (powerupMode[i])
			{
			case 0: // collectibles
				continue;
			case 1: // timed
				if (player.paused)
					continue;
				break;
			case 2: // permanent
				if (!player.paused || player.powerups[i] == 0)
					continue;
				break;
			}

			{
				entityClass *label = gui()->entities()->newEntity(i + 1);
				GUI_GET_COMPONENT(parent, parent, label);
				parent.parent = column->getName();
				parent.order = -(sint32)i;
				if (player.paused)
				{
					GUI_GET_COMPONENT(button, control, label);
				}
				else
				{
					GUI_GET_COMPONENT(label, control, label);
					GUI_GET_COMPONENT(textFormat, format, label);
					format.color = vec3(1, 1, 1);
				}
				GUI_GET_COMPONENT(text, text, label);
				(void)text;
			}
		}

		if (!player.paused)
			return;

		{ // end game button
			entityClass *but = ents->newEntity(500);
			GUI_GET_COMPONENT(button, control, but);
			GUI_GET_COMPONENT(text, txt, but);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/game/end");
			GUI_GET_COMPONENT(position, position, but);
			position.anchor = vec2(1, 1);
			position.position.values[0] = 1;
			position.position.units[0] = unitEnum::ScreenWidth;
			position.position.values[1] = 1;
			position.position.units[1] = unitEnum::ScreenHeight;
		}

		{ // paused
			entityClass *panel = ents->newUniqueEntity();
			GUI_GET_COMPONENT(groupBox, groupBox, panel);
			groupBox.type = groupBoxTypeEnum::Panel;
			GUI_GET_COMPONENT(position, position, panel);
			position.anchor = vec2(0.5, 0.5);
			position.position.values[0] = 0.5;
			position.position.units[0] = unitEnum::ScreenWidth;
			position.position.values[1] = 0.2;
			position.position.units[1] = unitEnum::ScreenHeight;

			entityClass *label = ents->newUniqueEntity();
			GUI_GET_COMPONENT(parent, parent, label);
			parent.parent = panel->getName();
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
		if (player.gameOver || player.cinematic)
			return;

		if (!assets()->ready(hashString("grid/languages/internationalized.textpack")))
			return;

		if (player.paused != previousPaused)
		{
			previousPaused = player.paused;
			makeTheGui();
		}

		textPackClass *texts = assets()->get<assetSchemeIndexTextPackage, textPackClass>(hashString("grid/languages/internationalized.textpack"));

		{ // life
			GUI_GET_COMPONENT(text, txt, gui()->entities()->getEntity(100));
			txt.value = texts->get(hashString("gui/game/life")) + "\n" + numeric_cast<uint32>(max(0, player.life));
		}

		{ // score
			GUI_GET_COMPONENT(text, txt, gui()->entities()->getEntity(101));
			txt.value = texts->get(hashString("gui/game/score")) + "\n" + player.score;
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

		for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
		{
			if (!gui()->entities()->hasEntity(i + 1))
				continue;
			GUI_GET_COMPONENT(text, txt, gui()->entities()->getEntity(i + 1));
			if (player.powerups[i] == 0)
			{
				txt.value = "";
				continue;
			}
			switch (powerupMode[i])
			{
			case 0: // collectibles
				txt.value = texts->get(textNames[i]) + "\n" + player.powerups[i];
				break;
			case 1: // timed
				txt.value = texts->get(textNames[i]) + " " + (player.powerups[i] / 30);
				break;
			case 2: // permanent
				txt.value = texts->get(textNames[i]) + " " + player.powerups[i];
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
	player.cinematic = false;
	gameStartEvent().dispatch();

	gui()->skipAllEventsUntilNextUpdate();
}
