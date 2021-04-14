#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/assetManager.h>
#include <cage-core/config.h>
#include <cage-core/spatialStructure.h>
#include <cage-core/hashString.h>
#include <cage-core/color.h>

#include "game.h"

namespace
{
	quat skyboxOrientation;
	quat skyboxRotation;

	struct SkyboxComponent
	{
		static EntityComponent *component;

		bool dissipating = false;
	};

	EntityComponent *SkyboxComponent::component;

	void engineInit()
	{
		SkyboxComponent::component = engineEntities()->defineComponent(SkyboxComponent(), true);
		skyboxOrientation = randomDirectionQuat();
		skyboxRotation = interpolate(quat(), randomDirectionQuat(), 5e-5);
	}

	void engineUpdate()
	{
		OPTICK_EVENT("environment");

		{ // update skybox
			skyboxOrientation = skyboxRotation * skyboxOrientation;

			for (Entity *e : SkyboxComponent::component->entities())
			{
				CAGE_COMPONENT_ENGINE(Transform, t, e);
				t.orientation = skyboxOrientation;
				DEGRID_COMPONENT(Skybox, s, e);
				if (s.dissipating)
				{
					CAGE_COMPONENT_ENGINE(Render, r, e);
					CAGE_ASSERT(r.opacity.valid());
					r.opacity -= 0.05;
					if (r.opacity < 1e-5)
						e->add(entitiesToDestroy);
				}
			}

			{ // hurt
				if (statistics.updateIterationIgnorePause == statistics.monstersLastHit && !game.cinematic)
				{
					Entity *e = engineEntities()->createUnique();
					CAGE_COMPONENT_ENGINE(Transform, t, e);
					t.orientation = randomDirectionQuat();
					CAGE_COMPONENT_ENGINE(Render, r, e);
					r.sceneMask = 2;
					r.object = HashString("degrid/environment/skyboxes/skybox.obj;hurt");
					r.opacity = 1;
					DEGRID_COMPONENT(Skybox, s, e);
					s.dissipating = true;
				}
			}
		}

		if (game.gameOver || game.paused)
			return;

		{ // update degrid markers
			for (Entity *e : GridComponent::component->entities())
			{
				CAGE_COMPONENT_ENGINE(Transform, t, e);
				CAGE_COMPONENT_ENGINE(Render, r, e);
				DEGRID_COMPONENT(Velocity, v, e);
				DEGRID_COMPONENT(Grid, g, e);
				v.velocity *= 0.95;
				v.velocity += (g.place - t.position) * 0.005;
				r.color = interpolate(r.color, g.originalColor, 0.002);
			}
		}
	}

	void gameStart()
	{
		for (Entity *e : SkyboxComponent::component->entities())
		{
			// prevent the skyboxes to be destroyed so that they can dissipate properly
			DEGRID_COMPONENT(Skybox, s, e);
			e->remove(entitiesToDestroy);
		}

		if (game.cinematic)
			setSkybox(HashString("degrid/environment/skyboxes/skybox.obj;menu"));
		else
			setSkybox(HashString("degrid/environment/skyboxes/skybox.obj;0"));

		{ // the sun
			Entity *light = engineEntities()->createUnique();
			CAGE_COMPONENT_ENGINE(Transform, t, light);
			t.orientation = quat(degs(-55), degs(70), degs());
			CAGE_COMPONENT_ENGINE(Light, l, light);
			l.lightType = LightTypeEnum::Directional;
			l.color = vec3(1);
			l.intensity = 3;
		}

		constexpr float radius = MapNoPullRadius + PlayerScale;
#ifdef CAGE_DEBUG
		constexpr float step = 50;
#else
		constexpr float step = 12;
#endif
		for (real y = -radius; y < radius + 1e-3; y += step)
		{
			for (real x = -radius; x < radius + 1e-3; x += step)
			{
				const real d = length(vec3(x, 0, y));
				if (d > radius || d < 1e-7)
					continue;
				Entity *e = engineEntities()->createUnique();
				DEGRID_COMPONENT(Velocity, velocity, e);
				velocity.velocity = randomDirection3();
				DEGRID_COMPONENT(Grid, grid, e);
				grid.place = vec3(x, -2, y) + vec3(randomChance(), randomChance() * 0.1, randomChance()) * 2 - 1;
				CAGE_COMPONENT_ENGINE(Transform, transform, e);
				transform.scale = 0.7;
				transform.position = grid.place + randomDirection3() * vec3(10, 0.1, 10);
				CAGE_COMPONENT_ENGINE(Render, render, e);
				render.object = HashString("degrid/environment/grid.object");
				real ang = real(atan2(x, y)) / (real::Pi() * 2) + 0.5;
				real dst = d / radius;
				render.color = colorHsvToRgb(vec3(ang, 1, interpolate(real(0.6), real(0.4), sqr(dst))));
				grid.originalColor = render.color;
			}
		}

#ifdef CAGE_DEBUG
		constexpr float angStep = 9;
#else
		constexpr float angStep = 3;
#endif
		for (rads ang = degs(0); ang < degs(360); ang += degs(angStep))
		{
			Entity *e = engineEntities()->createUnique();
			DEGRID_COMPONENT(Grid, grid, e);
			CAGE_COMPONENT_ENGINE(Transform, transform, e);
			transform.position = grid.place = vec3(sin(ang), 0, cos(ang)) * (radius + step * 0.5);
			transform.scale = 0.6;
			CAGE_COMPONENT_ENGINE(Render, render, e);
			render.object = HashString("degrid/environment/grid.object");
			grid.originalColor = render.color = vec3(1);
		}

		statistics.environmentGridMarkers = GridComponent::component->group()->count();
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener;
		EventListener<void()> gameStartListener;
	public:
		Callbacks() : engineInitListener("environment"), engineUpdateListener("environment"), gameStartListener("environment")
		{
			engineInitListener.attach(controlThread().initialize, -35);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener.attach(controlThread().update, 5);
			engineUpdateListener.bind<&engineUpdate>();
			gameStartListener.attach(gameStartEvent(), -5);
			gameStartListener.bind<&gameStart>();
		}
	} callbacksInstance;
}

void setSkybox(uint32 objectName)
{
	{ // initiate disappearing of old sky-boxes
		for (Entity *e : SkyboxComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(Transform, t, e);
			t.position[2] *= 0.9; // move the sky-box closer to the camera
			DEGRID_COMPONENT(Skybox, s, e);
			s.dissipating = true;
		}
	}

	{ // create new sky-box
		Entity *e = engineEntities()->createUnique();
		CAGE_COMPONENT_ENGINE(Transform, t, e);
		t.position[2] = -1e-5; // semitransparent objects are rendered back-to-front; this makes the sky-box the furthest
		CAGE_COMPONENT_ENGINE(Render, r, e);
		r.sceneMask = 2;
		r.object = objectName;
		r.opacity = 1;
		DEGRID_COMPONENT(Skybox, s, e);
	}
}

void environmentExplosion(const vec3 &position, const vec3 &velocity, const vec3 &color, real size)
{
	statistics.environmentExplosions++;

	// colorize nearby grids
	spatialSearchQuery->intersection(Sphere(position, size * 2));
	for (uint32 otherName : spatialSearchQuery->result())
	{
		if (!engineEntities()->has(otherName))
			continue;
		Entity *e = engineEntities()->get(otherName);
		if (!e->has(GridComponent::component))
			continue;
		CAGE_COMPONENT_ENGINE(Transform, ot, e);
		CAGE_COMPONENT_ENGINE(Render, orc, e);
		DEGRID_COMPONENT(Velocity, og, e);
		vec3 toOther = ot.position - position;
		real dist = length(toOther);
		real intpr = smoothstep(((size - dist) / size + 1) * 0.5);
		vec3 dir = normalize(toOther);
		vec3 change = dir * intpr;
		og.velocity += change;
		ot.position += change * 2;
		orc.color = interpolate(color, orc.color, intpr);
	}

	// create some debris
	uint32 cnt = numeric_cast<uint32>(size * size * 0.5 + 2);
	for (uint32 i = 0; i < cnt; i++)
	{
		real scale = randomChance();
		Entity *e = engineEntities()->createAnonymous();
		DEGRID_COMPONENT(Timeout, timeout, e);
		timeout.ttl = randomRange(5, 25);
		DEGRID_COMPONENT(Velocity, vel, e);
		vel.velocity = velocity * 0.5 + randomDirection3() * (1.1 - scale);
		CAGE_COMPONENT_ENGINE(Transform, transform, e);
		transform.scale = randomRange(1.0, 1.3) * (scale * 0.4 + 0.8);
		transform.position = position + randomDirection3() * (randomChance() * size);
		transform.orientation = randomDirectionQuat();
		CAGE_COMPONENT_ENGINE(Render, render, e);
		render.object = HashString("degrid/environment/explosion.object");
		render.color = colorVariation(color);
		e->add(entitiesPhysicsEvenWhenPaused);
	}

	// create light
	{
		Entity *e = engineEntities()->createAnonymous();
		DEGRID_COMPONENT(Timeout, timeout, e);
		timeout.ttl = randomRange(5, 10);
		DEGRID_COMPONENT(Velocity, vel, e);
		vel.velocity = velocity * 0.3 + randomDirection3() * 0.02;
		CAGE_COMPONENT_ENGINE(Transform, transform, e);
		transform.position = position + randomDirection3() * vec3(1, 0.1, 1) * size * randomChance();
		transform.position[1] = abs(transform.position[1]); // make the light always above the game plane (closer to camera)
		e->add(entitiesPhysicsEvenWhenPaused);
		CAGE_COMPONENT_ENGINE(Light, light, e);
		vec3 colorinv = colorRgbToHsv(color);
		colorinv[0] = (colorinv[0] + 0.5) % 1;
		colorinv = colorHsvToRgb(colorinv);
		light.color = colorVariation(colorinv);
		light.intensity = randomRange(2.0, 3.0);
		light.lightType = LightTypeEnum::Point;
		light.attenuation = vec3(0, 0, 0.005);

		//CAGE_COMPONENT_ENGINE(Render, render, e);
		//render.object = HashString("cage/mesh/fake.obj");
	}
}

void monsterExplosion(Entity *e)
{
	CAGE_COMPONENT_ENGINE(Transform, t, e);
	CAGE_COMPONENT_ENGINE(Render, r, e);
	DEGRID_COMPONENT(Velocity, v, e);
	DEGRID_COMPONENT(Monster, m, e);
	environmentExplosion(t.position, v.velocity, r.color, t.scale);
}

void shotExplosion(Entity *e)
{
	CAGE_COMPONENT_ENGINE(Transform, t, e);
	CAGE_COMPONENT_ENGINE(Render, r, e);
	DEGRID_COMPONENT(Velocity, v, e);
	environmentExplosion(t.position, v.velocity, r.color, t.scale);
}

vec3 colorVariation(const vec3 &color)
{
	vec3 dev = randomChance3() * 0.1 - 0.05;
	vec3 hsv = colorRgbToHsv(color) + dev;
	hsv[0] = (hsv[0] + 1) % 1;
	return colorHsvToRgb(clamp(hsv, 0, 1));
}
