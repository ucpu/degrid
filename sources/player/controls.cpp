#include "../game.h"
#include "../screens/screens.h"

#include <cage-core/config.h>

extern configUint32 confControlMovement;
extern configUint32 confControlFiring;
extern configUint32 confControlBomb;
extern configUint32 confControlTurret;
extern configUint32 confControlDecoy;
extern configString confPlayerName;

void eventBomb();
void eventTurret();
void eventDecoy();

entityClass *getPrimaryCameraEntity();

globalGameStruct game;

globalGameStruct::globalGameStruct()
{
	detail::memset(this, 0, sizeof(*this));
	cinematic = true;
	buyPriceMultiplier = 1;
}

namespace
{
	bool keyMap[512];
	mouseButtonsFlags buttonMap;
	windowEventListeners windowListeners;

	// input (options independent)
	vec3 arrowsDirection;
	vec3 mouseCurrentPosition;
	vec3 mouseLeftPosition;
	vec3 mouseRightPosition;

	void setMousePosition()
	{
		if (!window()->isFocused())
			return;
		pointStruct point = window()->mousePosition();
		pointStruct res = window()->resolution();
		vec2 p = vec2(point.x, point.y);
		p /= vec2(res.x, res.y);
		p = p * 2 - 1;
		real px = p[0], py = -p[1];
		ENGINE_GET_COMPONENT(transform, ts, getPrimaryCameraEntity());
		ENGINE_GET_COMPONENT(camera, cs, getPrimaryCameraEntity());
		mat4 view = mat4(ts.position, ts.orientation, vec3(ts.scale, ts.scale, ts.scale)).inverse();
		mat4 proj = perspectiveProjection(cs.perspectiveFov, real(res.x) / real(res.y), cs.near, cs.far);
		mat4 inv = (proj * view).inverse();
		vec4 pn = inv * vec4(px, py, -1, 1);
		vec4 pf = inv * vec4(px, py, 1, 1);
		vec3 near = vec3(pn) / pn[3];
		vec3 far = vec3(pf) / pf[3];
		mouseCurrentPosition = (near + far) * 0.5;
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

	void eventLetter(uint32 key)
	{
		for (uint32 o = 0; o < sizeof(letters); o++)
			if (letters[o] == key)
				eventAction(o + 4);
	}

	bool mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		if (game.paused)
			return false;

		statistics.buttonPressed++;
		buttonMap |= buttons;

		return false;
	}

	bool mouseRelease(mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		buttonMap &= ~buttons;

		if (game.paused)
			return false;

		if ((buttons & mouseButtonsFlags::Left) == mouseButtonsFlags::Left)
			eventAction(0);
		if ((buttons & mouseButtonsFlags::Right) == mouseButtonsFlags::Right)
			eventAction(1);
		if ((buttons & mouseButtonsFlags::Middle) == mouseButtonsFlags::Middle)
			eventAction(2);

		return false;
	}

	bool keyPress(uint32 key, uint32, modifiersFlags modifiers)
	{
		if (game.paused)
			return false;

		statistics.keyPressed++;
		if (key < sizeof(keyMap))
			keyMap[key] = true;

		return false;
	}

	bool keyRelease(uint32 key, uint32, modifiersFlags modifiers)
	{
		if (key < sizeof(keyMap))
			keyMap[key] = false;

		if (game.cinematic || game.gameOver)
			return false;

		if (key == 256) // esc
			game.paused = !game.paused;

		if (game.paused)
			return false;

		switch (key)
		{
		case ' ':
			eventAction(3);
			break;
		default:
			eventLetter(key);
			break;
		}

		return false;
	}

	void engineInit()
	{
		windowListeners.attachAll(window());
		windowListeners.mousePress.bind<&mousePress>();
		windowListeners.mouseRelease.bind<&mouseRelease>();
		windowListeners.keyPress.bind<&keyPress>();
		windowListeners.keyRelease.bind<&keyRelease>();

#ifdef GRID_TESTING
		CAGE_LOG(severityEnum::Info, "grid", string() + "TESTING GAME BUILD");
#endif // GRID_TESTING

		game.cinematic = true;
		gameStartEvent().dispatch();
	}

	void engineUpdate()
	{
		CAGE_ASSERT_RUNTIME(!game.gameOver || game.paused, game.gameOver, game.paused, game.cinematic);

		if (game.paused)
			return;

		arrowsDirection = vec3();
		game.moveDirection = vec3();
		game.fireDirection = vec3();

		if (game.cinematic)
		{
			game.fireDirection = randomDirection3();
			game.fireDirection[1] = 0;
			return;
		}

		{
			setMousePosition();
			if ((buttonMap & mouseButtonsFlags::Left) == mouseButtonsFlags::Left)
				mouseLeftPosition = mouseCurrentPosition;
			if ((buttonMap & mouseButtonsFlags::Right) == mouseButtonsFlags::Right)
				mouseRightPosition = mouseCurrentPosition;
		}

		{
			if (keyMap[87] || keyMap[265]) // w, up
				arrowsDirection += vec3(0, 0, -1);
			if (keyMap[83] || keyMap[264]) // s, down
				arrowsDirection += vec3(0, 0, 1);
			if (keyMap[65] || keyMap[263]) // a, left
				arrowsDirection += vec3(-1, 0, 0);
			if (keyMap[68] || keyMap[262]) // d, right
				arrowsDirection += vec3(1, 0, 0);
		}

		ENGINE_GET_COMPONENT(transform, playerTransform, game.playerEntity);

		switch (confControlMovement)
		{
		case 0: // arrows (absolute)
			game.moveDirection = arrowsDirection;
			break;
		case 1: // arrows (relative)
		{
			ENGINE_GET_COMPONENT(transform, tr, game.playerEntity);
			game.moveDirection = tr.orientation * arrowsDirection;
		} break;
		case 2: // lmb
			game.moveDirection = mouseLeftPosition - playerTransform.position;
			if (game.moveDirection.squaredLength() < 100)
				game.moveDirection = vec3();
			break;
		case 3: // rmb
			game.moveDirection = mouseRightPosition - playerTransform.position;
			if (game.moveDirection.squaredLength() < 100)
				game.moveDirection = vec3();
			break;
		case 4: // cursor position
			game.moveDirection = mouseCurrentPosition - playerTransform.position;
			if (game.moveDirection.squaredLength() < 100)
				game.moveDirection = vec3();
			break;
		}

		switch (confControlFiring)
		{
		case 0: // arrows (absolute)
			game.fireDirection = arrowsDirection;
			break;
		case 1: // arrows (relative)
		{
			ENGINE_GET_COMPONENT(transform, tr, game.playerEntity);
			game.fireDirection = tr.orientation * arrowsDirection;
		} break;
		case 2: // lmb
			game.fireDirection = mouseLeftPosition - playerTransform.position;
			break;
		case 3: // rmb
			game.fireDirection = mouseRightPosition - playerTransform.position;
			break;
		case 4: // cursor position
			game.fireDirection = mouseCurrentPosition - playerTransform.position;
			break;
		}
	}

	void gameStart()
	{
		CAGE_LOG(severityEnum::Info, "grid", string() + "new game, cinematic: " + game.cinematic);

		for (uint32 i = 0; i < sizeof(keyMap); i++)
			keyMap[i] = false;
		buttonMap = (mouseButtonsFlags)0;

		for (entityClass *e : entities()->group()->entities())
			e->add(entitiesToDestroy);

		{
			bool cinematic = game.cinematic;
			game = globalGameStruct();
			game.cinematic = cinematic;
		}

		game.life = 100;
		game.money = achievements.acquired * 100;
		game.powerupSpawnChance = 0.3;

#ifdef GRID_TESTING
		if (!game.cinematic)
		{
			game.life = 1000000;
			game.money = 1000000;
			for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
			{
				switch (powerupMode[i])
				{
				case 0: game.powerups[i] = 10; break;
				case 2: game.powerups[i] = 2; break;
				}
			}
		}
#endif
	}

	void gameStop()
	{
		if (!game.cinematic)
		{
			CAGE_LOG(severityEnum::Info, "grid", string() + "game over");
			CAGE_LOG(severityEnum::Info, "grid", string() + "score: " + game.score);
			CAGE_LOG(severityEnum::Info, "grid", string() + "money: " + game.money);
		}
		game.paused = game.gameOver = true;
		setScreenGameover();
	}

	class callbacksClass
	{
		eventListener<void()> engineInitListener;
		eventListener<void()> engineUpdateListener;
		eventListener<void()> gameStartListener;
		eventListener<void()> gameStopListener;
	public:
		callbacksClass()
		{
			engineInitListener.attach(controlThread().initialize, -30);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener.attach(controlThread().update, -30);
			engineUpdateListener.bind<&engineUpdate>();
			gameStartListener.attach(gameStartEvent(), -30);
			gameStartListener.bind<&gameStart>();
			gameStopListener.attach(gameStopEvent(), -30);
			gameStopListener.bind<&gameStop>();
		}
	} callbacksInstance;
}