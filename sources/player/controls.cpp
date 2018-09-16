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

globalPlayerStruct player;

globalPlayerStruct::globalPlayerStruct()
{
	detail::memset(this, 0, sizeof(*this));
	cinematic = true;
}

namespace
{
	bool keyMap[512];
	mouseButtonsFlags buttonMap;
	windowEventListeners windowListeners;

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
		player.mouseCurrentPosition = (near + far) * 0.5;
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
		if (player.paused)
			return false;

		statistics.buttonPressed++;
		buttonMap |= buttons;

		return false;
	}

	bool mouseRelease(mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		buttonMap &= ~buttons;

		if (player.paused)
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
		if (player.paused)
			return false;

		statistics.keyPressed++;
		if (key < sizeof(keyMap))
			keyMap[key] = true;

		return false;
	}

	bool keyRelease(uint32 key, uint32, modifiersFlags modifiers)
	{
		if (key == 256) // esc
			player.paused = !player.paused;

		if (key < sizeof(keyMap))
			keyMap[key] = false;

		if (player.paused)
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

		player.cinematic = true;
		gameStartEvent().dispatch();
	}

	void engineUpdate()
	{
		if (player.paused)
			return;

		player.arrowsDirection = vec3();
		player.moveDirection = vec3();
		player.fireDirection = vec3();

		if (player.cinematic)
		{
			player.fireDirection = randomDirection3();
			player.fireDirection[1] = 0;
			return;
		}

		{
			setMousePosition();
			if ((buttonMap & mouseButtonsFlags::Left) == mouseButtonsFlags::Left)
				player.mouseLeftPosition = player.mouseCurrentPosition;
			if ((buttonMap & mouseButtonsFlags::Right) == mouseButtonsFlags::Right)
				player.mouseRightPosition = player.mouseCurrentPosition;
		}

		{
			if (keyMap[87] || keyMap[265]) // w, up
				player.arrowsDirection += vec3(0, 0, -1);
			if (keyMap[83] || keyMap[264]) // s, down
				player.arrowsDirection += vec3(0, 0, 1);
			if (keyMap[65] || keyMap[263]) // a, left
				player.arrowsDirection += vec3(-1, 0, 0);
			if (keyMap[68] || keyMap[262]) // d, right
				player.arrowsDirection += vec3(1, 0, 0);
		}

		ENGINE_GET_COMPONENT(transform, playerTransform, player.playerEntity);

		switch (confControlMovement)
		{
		case 0: // arrows (absolute)
			player.moveDirection = player.arrowsDirection;
			break;
		case 1: // arrows (relative)
		{
			ENGINE_GET_COMPONENT(transform, tr, player.playerEntity);
			player.moveDirection = tr.orientation * player.arrowsDirection;
		} break;
		case 2: // lmb
			player.moveDirection = player.mouseLeftPosition - playerTransform.position;
			if (player.moveDirection.squaredLength() < 100)
				player.moveDirection = vec3();
			break;
		case 3: // rmb
			player.moveDirection = player.mouseRightPosition - playerTransform.position;
			if (player.moveDirection.squaredLength() < 100)
				player.moveDirection = vec3();
			break;
		case 4: // cursor position
			player.moveDirection = player.mouseCurrentPosition - playerTransform.position;
			if (player.moveDirection.squaredLength() < 100)
				player.moveDirection = vec3();
			break;
		}

		switch (confControlFiring)
		{
		case 0: // arrows (absolute)
			player.fireDirection = player.arrowsDirection;
			break;
		case 1: // arrows (relative)
		{
			ENGINE_GET_COMPONENT(transform, tr, player.playerEntity);
			player.fireDirection = tr.orientation * player.arrowsDirection;
		} break;
		case 2: // lmb
			player.fireDirection = player.mouseLeftPosition - playerTransform.position;
			break;
		case 3: // rmb
			player.fireDirection = player.mouseRightPosition - playerTransform.position;
			break;
		case 4: // cursor position
			player.fireDirection = player.mouseCurrentPosition - playerTransform.position;
			break;
		}
	}

	void gameStart()
	{
		CAGE_LOG(severityEnum::Info, "grid", string() + "new game, cinematic: " + player.cinematic);

		for (uint32 i = 0; i < sizeof(keyMap); i++)
			keyMap[i] = false;
		buttonMap = (mouseButtonsFlags)0;

		entities()->getAllEntities()->destroyAllEntities();
		player = globalPlayerStruct();
		player.life = 100;
		player.powerupSpawnChance = 0.3;

#ifdef GRID_TESTING
		if (!player.cinematic)
		{
			player.life = 1000000;
			for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
			{
				switch (powerupMode[i])
				{
				case 0: player.powerups[i] = 10; break;
				case 2: player.powerups[i] = 2; break;
				}
			}
		}
#endif
	}

	void gameStop()
	{
		CAGE_LOG(severityEnum::Info, "grid", string() + "game over, score: " + player.score);
		player.paused = player.gameOver = true;
		setScreenGameover(player.score);
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
			engineInitListener.attach(controlThread().initialize);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener.attach(controlThread().update);
			engineUpdateListener.bind<&engineUpdate>();
			gameStartListener.attach(gameStartEvent());
			gameStartListener.bind<&gameStart>();
			gameStopListener.attach(gameStopEvent());
			gameStopListener.bind<&gameStop>();
		}
	} callbacksInstance;
}
