#include <cage-core/config.h>
#include <cage-core/camera.h>
#include <cage-core/geometry.h>
#include <cage-engine/gamepad.h>

#include "../game.h"
#include "../screens/screens.h"

extern ConfigUint32 confControlMovement;
extern ConfigUint32 confControlFiring;
extern ConfigUint32 confControlBomb;
extern ConfigUint32 confControlTurret;
extern ConfigUint32 confControlDecoy;
extern ConfigString confPlayerName;

void eventBomb();
void eventTurret();
void eventDecoy();

Entity *getPrimaryCameraEntity();

GlobalGame game;

namespace
{
	Holder<Gamepad> gamepad;

	bool keyMap[512];
	MouseButtonsFlags buttonMap;

	EventListener<bool(const GenericInput &)> mousePressListener;
	EventListener<bool(const GenericInput &)> mouseReleaseListener;
	EventListener<bool(const GenericInput &)> keyPressListener;
	EventListener<bool(const GenericInput &)> keyReleaseListener;
	EventListener<bool(const GenericInput &)> gamepadKeyListener;

	// input (options independent)
	Vec3 arrowsDirection;
	Vec3 mouseCurrentPosition;
	Vec3 mouseLeftPosition;
	Vec3 mouseRightPosition;
	Vec3 leftStick, rightStick;

	void setMousePosition()
	{
		if (!engineWindow()->isFocused())
			return;

		Vec2 p = engineWindow()->mousePosition();
		Vec2i res = engineWindow()->resolution();
		p /= Vec2(res[0], res[1]);
		p = p * 2 - 1;
		Real px = p[0], py = -p[1];
		const TransformComponent &ts = getPrimaryCameraEntity()->value<TransformComponent>();
		const CameraComponent &cs = getPrimaryCameraEntity()->value<CameraComponent>();
		Mat4 view = inverse(Mat4(ts.position, ts.orientation, Vec3(ts.scale, ts.scale, ts.scale)));
		Mat4 proj = perspectiveProjection(cs.camera.perspectiveFov, Real(res[0]) / Real(res[1]), cs.near, cs.far);
		Mat4 inv = inverse(proj * view);
		Vec4 pn = inv * Vec4(px, py, -1, 1);
		Vec4 pf = inv * Vec4(px, py, 1, 1);
		Vec3 near = Vec3(pn) / pn[3];
		Vec3 far = Vec3(pf) / pf[3];
		mouseCurrentPosition = intersection(makeLine(near, far), Plane(Vec3(), Vec3(0, 1, 0)));
	}

	void eventAction(uint32 option)
	{
		if (confControlBomb == option)
			eventBomb();
		if (confControlTurret == option)
			eventTurret();
		if (confControlDecoy == option)
			eventDecoy();
	}

	void mousePress(InputMouse in)
	{
		if (game.paused)
			return;

		statistics.buttonPressed++;
		buttonMap |= in.buttons;
	}

	void mouseRelease(InputMouse in)
	{
		buttonMap &= ~in.buttons;

		if (game.paused)
			return;

		if (any(in.buttons & MouseButtonsFlags::Left))
			eventAction(0);
		if (any(in.buttons & MouseButtonsFlags::Middle))
			eventAction(1);
		if (any(in.buttons & MouseButtonsFlags::Right))
			eventAction(2);
	}

	void keyPress(InputKey in)
	{
		if (game.paused)
			return;

		statistics.keyPressed++;
		if (in.key < sizeof(keyMap))
			keyMap[in.key] = true;
	}

	void keyRelease(InputKey in)
	{
		if (in.key < sizeof(keyMap))
			keyMap[in.key] = false;

		if (game.cinematic || game.gameOver)
			return;

		if (in.key == 256) // esc
			game.paused = !game.paused;

		if (game.paused)
			return;

		if (in.key == ' ')
			eventAction(3);
		for (uint32 o = 0; o < sizeof(Letters); o++)
			if (Letters[o] == in.key)
				eventAction(o + 4);
	}

	void gamepadKey(InputGamepadKey in)
	{
		if (in.key < 4)
			eventAction(in.key + 4 + sizeof(Letters));
	}

	const auto engineInitListener = controlThread().initialize.listen([]() {
		mousePressListener.attach(engineWindow()->events);
		mousePressListener.bind(inputListener<InputClassEnum::MousePress, InputMouse>(&mousePress));
		keyPressListener.attach(engineWindow()->events);
		keyPressListener.bind(inputListener<InputClassEnum::KeyPress, InputKey>(&keyPress));
		// process some events before gui
		mouseReleaseListener.attach(engineWindow()->events, -2);
		mouseReleaseListener.bind(inputListener<InputClassEnum::MouseRelease, InputMouse>(&mouseRelease));
		keyReleaseListener.attach(engineWindow()->events, -1);
		keyReleaseListener.bind(inputListener<InputClassEnum::KeyRelease, InputKey>(&keyRelease));

#ifdef DEGRID_TESTING
		CAGE_LOG(SeverityEnum::Info, "degrid", String() + "TESTING GAME BUILD");
#endif // DEGRID_TESTING

		game.cinematic = true;
		gameStartEvent().dispatch();
	}, -30);

	const auto engineUpdateListener = controlThread().update.listen([]() {
		CAGE_ASSERT(!game.gameOver || game.paused);

		if (!gamepad && gamepadsAvailable() > 0)
		{
			gamepad = newGamepad();
			gamepadKeyListener.attach(gamepad->events);
			gamepadKeyListener.bind(inputListener<InputClassEnum::GamepadRelease, InputGamepadKey>(&gamepadKey));
		}
		if (gamepad)
		{
			gamepad->processEvents();
			if (!gamepad->connected())
				gamepad.clear();
		}

		if (game.paused)
			return;

		arrowsDirection = Vec3();
		leftStick = rightStick = Vec3();
		game.moveDirection = Vec3();
		game.fireDirection = Vec3();

		if (game.cinematic)
		{
			uint32 cnt = engineEntities()->component<MonsterComponent>()->count();
			if (cnt == 0)
				game.fireDirection = randomDirection3();
			else
			{
				cnt = randomRange(0u, cnt);
				for (Entity *e : engineEntities()->component<MonsterComponent>()->entities())
				{
					if (cnt-- == 0)
					{
						TransformComponent &p = game.playerEntity->value<TransformComponent>();
						TransformComponent &t = e->value<TransformComponent>();
						game.fireDirection = t.position - p.position;
						break;
					}
				}
			}
			game.fireDirection[1] = 0;
			game.fireDirection = normalize(game.fireDirection);
			return;
		}

		{
			setMousePosition();
			if (any(buttonMap & MouseButtonsFlags::Left))
				mouseLeftPosition = mouseCurrentPosition;
			if (any(buttonMap & MouseButtonsFlags::Right))
				mouseRightPosition = mouseCurrentPosition;
		}

		{
			if (keyMap[87] || keyMap[265]) // w, up
				arrowsDirection += Vec3(0, 0, -1);
			if (keyMap[83] || keyMap[264]) // s, down
				arrowsDirection += Vec3(0, 0, 1);
			if (keyMap[65] || keyMap[263]) // a, left
				arrowsDirection += Vec3(-1, 0, 0);
			if (keyMap[68] || keyMap[262]) // d, right
				arrowsDirection += Vec3(1, 0, 0);
			if (arrowsDirection != Vec3())
				arrowsDirection = normalize(arrowsDirection);
		}

		if (gamepad)
		{
			leftStick[0] = gamepad->axes()[0];
			leftStick[2] = gamepad->axes()[1];
			rightStick[0] = gamepad->axes()[2];
			rightStick[2] = gamepad->axes()[3];
		}

		static constexpr Real MouseMultiplier = 0.05;
		const TransformComponent &playerTransform = game.playerEntity->value<TransformComponent>();

		switch (confControlMovement)
		{
		case 0: // arrows (absolute)
			game.moveDirection = arrowsDirection;
			break;
		case 1: // arrows (relative)
			game.moveDirection = playerTransform.orientation * arrowsDirection;
			break;
		case 2: // lmb
			game.moveDirection = (mouseLeftPosition - playerTransform.position) * MouseMultiplier;
			break;
		case 3: // rmb
			game.moveDirection = (mouseRightPosition - playerTransform.position) * MouseMultiplier;
			break;
		case 4: // cursor position
			game.moveDirection = (mouseCurrentPosition - playerTransform.position) * MouseMultiplier;
			break;
		case 5: // left stick (absolute)
			game.moveDirection = leftStick;
			break;
		case 6: // right stick (absolute)
			game.moveDirection = rightStick;
			break;
		}
		game.moveDirection[1] = 0;
		if (lengthSquared(game.moveDirection) > sqr(0.9))
			game.moveDirection = normalize(game.moveDirection);
		else
			game.moveDirection = Vec3();

		switch (confControlFiring)
		{
		case 0: // arrows (absolute)
			game.fireDirection = arrowsDirection;
			break;
		case 1: // arrows (relative)
			game.fireDirection = playerTransform.orientation * arrowsDirection;
			break;
		case 2: // lmb
			game.fireDirection = (mouseLeftPosition - playerTransform.position) * MouseMultiplier;
			break;
		case 3: // rmb
			game.fireDirection = (mouseRightPosition - playerTransform.position) * MouseMultiplier;
			break;
		case 4: // cursor position
			game.fireDirection = (mouseCurrentPosition - playerTransform.position) * MouseMultiplier;
			break;
		case 5: // left stick (absolute)
			game.fireDirection = leftStick;
			break;
		case 6: // right stick (absolute)
			game.fireDirection = rightStick;
			break;
		}
		game.fireDirection[1] = 0;
		if (lengthSquared(game.fireDirection) > sqr(0.9))
			game.fireDirection = normalize(game.fireDirection);
		else
			game.fireDirection = Vec3();
	}, -30);

	const auto gameStartListener = gameStartEvent().listen([]() {
		CAGE_LOG(SeverityEnum::Info, "degrid", Stringizer() + "new game, cinematic: " + game.cinematic);

		for (uint32 i = 0; i < sizeof(keyMap); i++)
			keyMap[i] = false;
		buttonMap = MouseButtonsFlags::None;

		for (Entity *e : engineEntities()->group()->entities())
			e->add(entitiesToDestroy);

		{
			const bool cinematic = game.cinematic;
			game = GlobalGame();
			game.cinematic = cinematic;
		}

		game.life = 100;
		game.money = achievements.acquired * PowerupBuyPriceBase * 2;
		game.powerupSpawnChance = 0.3;

#ifdef DEGRID_TESTING
		if (!game.cinematic)
		{
			game.life = 1000000;
			game.money = 1000000;
			for (uint32 i = 0; i < (uint32)PowerupTypeEnum::Total; i++)
			{
				switch (PowerupMode[i])
				{
				case 0: game.powerups[i] = 10; break;
				case 2: game.powerups[i] = 2; break;
				}
			}
		}
#endif
	}, -30);

	const auto gameStopListener = gameStopEvent().listen([]() {
		if (!game.cinematic)
		{
			CAGE_LOG(SeverityEnum::Info, "degrid", Stringizer() + "game over");
			CAGE_LOG(SeverityEnum::Info, "degrid", Stringizer() + "score: " + game.score);
			CAGE_LOG(SeverityEnum::Info, "degrid", Stringizer() + "money: " + game.money);
		}
		game.paused = game.gameOver = true;
		setScreenGameover();
	}, -30);
}
