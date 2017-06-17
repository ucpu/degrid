#include "../includes.h"
#include "../screens.h"
#include "../game.h"

namespace
{
	void makeTheGui();

	const bool mousePress(windowClass *, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		if (!grid::player.paused)
			grid::mousePress(buttons, modifiers, point);
		return true;
	}

	const bool mouseRelease(windowClass *, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		if (!grid::player.paused)
			grid::mouseRelease(buttons, modifiers, point);
		return true;
	}

	const bool mouseMove(windowClass *, mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		if (!grid::player.paused)
			grid::mouseMove(buttons, modifiers, point);
		return true;
	}

	const bool keyPress(windowClass *, uint32 key, uint32, modifiersFlags modifiers)
	{
		if (!grid::player.paused)
			grid::keyPress(key, modifiers);
		return true;
	}

	const bool keyRelease(windowClass *, uint32 key, uint32, modifiersFlags modifiers)
	{
		if (!grid::player.paused)
			grid::keyRelease(key, modifiers);

		if (key == 256) // esc
			grid::player.paused = !grid::player.paused;

		return true;
	}

	eventListener<void(uint32)> guiEvent;

	const bool guiFunction(uint32 en)
	{
		switch (en)
		{
		case 500:
			grid::player.life = 0;
			grid::player.paused = false;
			return true;
		}
		if (en <= grid::puTotal)
		{
			grid::player.powerups[en - 1]--;
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
		guiEvent.attach(gui()->genericEvent);

		{
			winEvtLists.mousePress.bind<&mousePress>();
			winEvtLists.mouseRelease.bind<&mouseRelease>();
			winEvtLists.mouseMove.bind<&mouseMove>();
			winEvtLists.keyPress.bind<&keyPress>();
			winEvtLists.keyRelease.bind<&keyRelease>();
		}

		{ // life
			entityClass *lifeLabel = gui()->entities()->newEntity(100);
			GUI_GET_COMPONENT(control, control, lifeLabel);
			control.controlType = controlTypeEnum::Empty;
			GUI_GET_COMPONENT(text, text, lifeLabel);
			text.value = "life";
			GUI_GET_COMPONENT(format, format, lifeLabel);
			format.align = textAlignEnum::Center;
			format.color = vec3(1, 0, 0);
			format.fontName = hashString("cage/font/roboto.ttf?20");
			GUI_GET_COMPONENT(position, pos, lifeLabel);
			pos.x = 1.0 / 6.0;
			pos.xUnit = unitsModeEnum::ScreenWidth;
			pos.y = 0;
			pos.yUnit = unitsModeEnum::ScreenHeight;
			pos.anchorX = 0.5;
			pos.anchorY = 0;
		}

		{ // bombs
			entityClass *lifeLabel = gui()->entities()->newEntity(1);
			GUI_GET_COMPONENT(control, control, lifeLabel);
			control.controlType = controlTypeEnum::Empty;
			GUI_GET_COMPONENT(text, text, lifeLabel);
			GUI_GET_COMPONENT(format, format, lifeLabel);
			format.align = textAlignEnum::Center;
			format.color = vec3(1, 1, 1);
			format.fontName = hashString("cage/font/roboto.ttf?20");
			GUI_GET_COMPONENT(position, pos, lifeLabel);
			pos.x = 2.0 / 6.0;
			pos.xUnit = unitsModeEnum::ScreenWidth;
			pos.y = 0;
			pos.yUnit = unitsModeEnum::ScreenHeight;
			pos.anchorX = 0.5;
			pos.anchorY = 0;
		}

		{ // turrets
			entityClass *lifeLabel = gui()->entities()->newEntity(2);
			GUI_GET_COMPONENT(control, control, lifeLabel);
			control.controlType = controlTypeEnum::Empty;
			GUI_GET_COMPONENT(text, text, lifeLabel);
			GUI_GET_COMPONENT(format, format, lifeLabel);
			format.align = textAlignEnum::Center;
			format.color = vec3(1, 1, 1);
			format.fontName = hashString("cage/font/roboto.ttf?20");
			GUI_GET_COMPONENT(position, pos, lifeLabel);
			pos.x = 3.0 / 6.0;
			pos.xUnit = unitsModeEnum::ScreenWidth;
			pos.y = 0;
			pos.yUnit = unitsModeEnum::ScreenHeight;
			pos.anchorX = 0.5;
			pos.anchorY = 0;
		}

		{ // decoys
			entityClass *lifeLabel = gui()->entities()->newEntity(3);
			GUI_GET_COMPONENT(control, control, lifeLabel);
			control.controlType = controlTypeEnum::Empty;
			GUI_GET_COMPONENT(text, text, lifeLabel);
			GUI_GET_COMPONENT(format, format, lifeLabel);
			format.align = textAlignEnum::Center;
			format.color = vec3(1, 1, 1);
			format.fontName = hashString("cage/font/roboto.ttf?20");
			GUI_GET_COMPONENT(position, pos, lifeLabel);
			pos.x = 4.0 / 6.0;
			pos.xUnit = unitsModeEnum::ScreenWidth;
			pos.y = 0;
			pos.yUnit = unitsModeEnum::ScreenHeight;
			pos.anchorX = 0.5;
			pos.anchorY = 0;
		}

		{ // score
			entityClass *scoreLabel = gui()->entities()->newEntity(101);
			GUI_GET_COMPONENT(control, control, scoreLabel);
			control.controlType = controlTypeEnum::Empty;
			GUI_GET_COMPONENT(text, text, scoreLabel);
			text.value = "score";
			GUI_GET_COMPONENT(format, format, scoreLabel);
			format.align = textAlignEnum::Center;
			format.color = vec3(0, 1, 0);
			format.fontName = hashString("cage/font/roboto.ttf?20");
			GUI_GET_COMPONENT(position, pos, scoreLabel);
			pos.x = 5.0 / 6.0;
			pos.xUnit = unitsModeEnum::ScreenWidth;
			pos.y = 0;
			pos.yUnit = unitsModeEnum::ScreenHeight;
			pos.anchorX = 0.5;
			pos.anchorY = 0;
		}

		entityClass *column = ents->newEntity(ents->generateUniqueName());
		{
			GUI_GET_COMPONENT(layout, layout, column);
			layout.layoutMode = layoutModeEnum::Column;
			GUI_GET_COMPONENT(position, position, column);
			position.anchorY = 1;
			position.y = 1;
			position.yUnit = unitsModeEnum::ScreenHeight;
		}

		for (uint32 i = 0; i < grid::puTotal; i++)
		{
			switch (grid::powerupMode[i])
			{
			case 0: // collectibles
				continue;
			case 1: // timed
				if (grid::player.paused)
					continue;
				break;
			case 2: // permanent
				if (!grid::player.paused || grid::player.powerups[i] == 0)
					continue;
				break;
			}

			{
				entityClass *label = gui()->entities()->newEntity(i + 1);
				GUI_GET_COMPONENT(parent, parent, label);
				parent.parent = column->getName();
				parent.ordering = -(sint32)i;
				GUI_GET_COMPONENT(control, control, label);
				if (grid::player.paused)
					control.controlType = controlTypeEnum::Button;
				else
				{
					control.controlType = controlTypeEnum::Empty;
					GUI_GET_COMPONENT(format, format, label);
					format.color = vec3(1, 1, 1);
				}
				GUI_GET_COMPONENT(text, text, label);
			}
		}

		if (!grid::player.paused)
			return;

		{ // end game button
			entityClass *but = ents->newEntity(500);
			GUI_GET_COMPONENT(control, control, but);
			control.controlType = controlTypeEnum::Button;
			GUI_GET_COMPONENT(text, txt, but);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/game/end");
			GUI_GET_COMPONENT(position, position, but);
			position.anchorX = position.anchorY = 1.0;
			position.x = position.y = 1.0;
			position.xUnit = unitsModeEnum::ScreenWidth;
			position.yUnit = unitsModeEnum::ScreenHeight;
			GUI_GET_COMPONENT(format, format, but);
			format.align = textAlignEnum::Center;
		}

		{ // paused
			entityClass *panel = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(control, controlP, panel);
			controlP.controlType = controlTypeEnum::Panel;
			GUI_GET_COMPONENT(position, position, panel);
			position.anchorX = position.anchorY = 0.5;
			position.x = 0.5;
			position.y = 0.2;
			position.xUnit = unitsModeEnum::ScreenWidth;
			position.yUnit = unitsModeEnum::ScreenHeight;

			entityClass *label = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, parent, label);
			parent.parent = panel->getName();
			GUI_GET_COMPONENT(control, controlL, label);
			controlL.controlType = controlTypeEnum::Empty;
			GUI_GET_COMPONENT(text, txt, label);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/game/pause");
			GUI_GET_COMPONENT(format, format, label);
			format.fontName = hashString("cage/font/roboto.ttf?50");
		}
	}
}

namespace grid
{
	bool previousPaused = false;

	void gameGuiUpdate()
	{
		if (assets()->state(hashString("grid/languages/internationalized.textpack")) != assetStateEnum::Ready)
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

		static const uint32 textNames[puTotal] = {
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

		for (uint32 i = 0; i < puTotal; i++)
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
}

void setScreenGame()
{
	grid::previousPaused = true;
	grid::gameStart(false);
}