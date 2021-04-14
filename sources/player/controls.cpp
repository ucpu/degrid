#include <cage-core/config.h>
#include <cage-core/camera.h>
#include <cage-core/geometry.h>

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
	bool keyMap[512];
	MouseButtonsFlags buttonMap;
	WindowEventListeners windowListeners;

	// input (options independent)
	vec3 arrowsDirection;
	vec3 mouseCurrentPosition;
	vec3 mouseLeftPosition;
	vec3 mouseRightPosition;

	void setMousePosition()
	{
		if (!engineWindow()->isFocused())
			return;

		ivec2 point = engineWindow()->mousePosition();
		ivec2 res = engineWindow()->resolution();
		vec2 p = vec2(point[0], point[1]);
		p /= vec2(res[0], res[1]);
		p = p * 2 - 1;
		real px = p[0], py = -p[1];
		CAGE_COMPONENT_ENGINE(Transform, ts, getPrimaryCameraEntity());
		CAGE_COMPONENT_ENGINE(Camera, cs, getPrimaryCameraEntity());
		mat4 view = inverse(mat4(ts.position, ts.orientation, vec3(ts.scale, ts.scale, ts.scale)));
		mat4 proj = perspectiveProjection(cs.camera.perspectiveFov, real(res[0]) / real(res[1]), cs.near, cs.far);
		mat4 inv = inverse(proj * view);
		vec4 pn = inv * vec4(px, py, -1, 1);
		vec4 pf = inv * vec4(px, py, 1, 1);
		vec3 near = vec3(pn) / pn[3];
		vec3 far = vec3(pf) / pf[3];
		mouseCurrentPosition = intersection(makeLine(near, far), Plane(vec3(), vec3(0, 1, 0)));

		if (false)
		{
			// debug visualization of the mouse position
			Entity *e = engineEntities()->createAnonymous();
			CAGE_COMPONENT_ENGINE(Transform, t, e);
			t.position = mouseCurrentPosition;
			t.scale = 3;
			CAGE_COMPONENT_ENGINE(Render, r, e);
			r.object = HashString("cage/mesh/sphere.obj");
			DEGRID_COMPONENT(Timeout, tt, e);
			tt.ttl = 1;
		}
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
		for (uint32 o = 0; o < sizeof(Letters); o++)
			if (Letters[o] == key)
				eventAction(o + 4);
	}

	bool mousePress(MouseButtonsFlags buttons, ModifiersFlags modifiers, const ivec2 &point)
	{
		if (game.paused)
			return false;

		statistics.buttonPressed++;
		buttonMap |= buttons;

		return false;
	}

	bool mouseRelease(MouseButtonsFlags buttons, ModifiersFlags modifiers, const ivec2 &point)
	{
		buttonMap &= ~buttons;

		if (game.paused)
			return false;

		if (any(buttons & MouseButtonsFlags::Left))
			eventAction(0);
		if (any(buttons & MouseButtonsFlags::Right))
			eventAction(1);
		if (any(buttons & MouseButtonsFlags::Middle))
			eventAction(2);

		return false;
	}

	bool keyPress(uint32 key, uint32, ModifiersFlags modifiers)
	{
		if (game.paused)
			return false;

		statistics.keyPressed++;
		if (key < sizeof(keyMap))
			keyMap[key] = true;

		return false;
	}

	bool keyRelease(uint32 key, uint32, ModifiersFlags modifiers)
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
		windowListeners.attachAll(engineWindow());
		windowListeners.mousePress.bind<&mousePress>();
		windowListeners.mouseRelease.bind<&mouseRelease>();
		windowListeners.keyPress.bind<&keyPress>();
		windowListeners.keyRelease.bind<&keyRelease>();

		// process some Events before gui
		windowListeners.mouseRelease.attach(engineWindow()->events.mouseRelease, -1);
		windowListeners.keyRelease.attach(engineWindow()->events.keyRelease, -1);

#ifdef DEGRID_TESTING
		CAGE_LOG(SeverityEnum::Info, "degrid", string() + "TESTING GAME BUILD");
#endif // DEGRID_TESTING

		game.cinematic = true;
		gameStartEvent().dispatch();
	}

	void engineUpdate()
	{
		OPTICK_EVENT("controls");
		CAGE_ASSERT(!game.gameOver || game.paused);

		if (game.paused)
			return;

		arrowsDirection = vec3();
		game.moveDirection = vec3();
		game.fireDirection = vec3();

		if (game.cinematic)
		{
			uint32 cnt = MonsterComponent::component->group()->count();
			if (cnt == 0)
				game.fireDirection = randomDirection3();
			else
			{
				cnt = randomRange(0u, cnt);
				for (Entity *e : MonsterComponent::component->entities())
				{
					if (cnt-- == 0)
					{
						CAGE_COMPONENT_ENGINE(Transform, p, game.playerEntity);
						CAGE_COMPONENT_ENGINE(Transform, t, e);
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
				arrowsDirection += vec3(0, 0, -1);
			if (keyMap[83] || keyMap[264]) // s, down
				arrowsDirection += vec3(0, 0, 1);
			if (keyMap[65] || keyMap[263]) // a, left
				arrowsDirection += vec3(-1, 0, 0);
			if (keyMap[68] || keyMap[262]) // d, right
				arrowsDirection += vec3(1, 0, 0);
			if (arrowsDirection != vec3())
				arrowsDirection = normalize(arrowsDirection);
		}

		constexpr float MouseMultiplier = 0.05f;
		CAGE_COMPONENT_ENGINE(Transform, playerTransform, game.playerEntity);

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
		}
		game.moveDirection[1] = 0;
		if (lengthSquared(game.moveDirection) > sqr(0.9))
			game.moveDirection = normalize(game.moveDirection);
		else
			game.moveDirection = vec3();

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
		}
		game.fireDirection[1] = 0;
		if (lengthSquared(game.fireDirection) > sqr(0.9))
			game.fireDirection = normalize(game.fireDirection);
		else
			game.fireDirection = vec3();
	}

	void gameStart()
	{
		CAGE_LOG(SeverityEnum::Info, "degrid", stringizer() + "new game, cinematic: " + game.cinematic);

		for (uint32 i = 0; i < sizeof(keyMap); i++)
			keyMap[i] = false;
		buttonMap = MouseButtonsFlags::None;

		for (Entity *e : engineEntities()->group()->entities())
			e->add(entitiesToDestroy);

		{
			bool cinematic = game.cinematic;
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
	}

	void gameStop()
	{
		if (!game.cinematic)
		{
			CAGE_LOG(SeverityEnum::Info, "degrid", stringizer() + "game over");
			CAGE_LOG(SeverityEnum::Info, "degrid", stringizer() + "score: " + game.score);
			CAGE_LOG(SeverityEnum::Info, "degrid", stringizer() + "money: " + game.money);
		}
		game.paused = game.gameOver = true;
		setScreenGameover();
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener;
		EventListener<void()> gameStartListener;
		EventListener<void()> gameStopListener;
	public:
		Callbacks() : engineInitListener("controls"), engineUpdateListener("controls"), gameStartListener("controls"), gameStopListener("controls")
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
