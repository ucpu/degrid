#include "game.h"

#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/assetManager.h>
#include <cage-core/config.h>
#include <cage-core/spatial.h>
#include <cage-core/hashString.h>
#include <cage-core/color.h>

namespace
{
	quat skyboxOrientation;
	quat skyboxRotation;

	struct skyboxComponent
	{
		static entityComponent *component;

		bool dissipating;

		skyboxComponent() : dissipating(false)
		{}
	};

	entityComponent *skyboxComponent::component;

	void engineInit()
	{
		skyboxComponent::component = entities()->defineComponent(skyboxComponent(), true);
		skyboxOrientation = randomDirectionQuat();
		skyboxRotation = interpolate(quat(), randomDirectionQuat(), 5e-5);
	}

	void engineUpdate()
	{
		OPTICK_EVENT("environment");

		{ // update skybox
			skyboxOrientation = skyboxRotation * skyboxOrientation;

			for (entity *e : skyboxComponent::component->entities())
			{
				CAGE_COMPONENT_ENGINE(transform, t, e);
				t.orientation = skyboxOrientation;
				DEGRID_COMPONENT(skybox, s, e);
				if (s.dissipating)
				{
					CAGE_COMPONENT_ENGINE(render, r, e);
					r.opacity -= 0.05;
					if (r.opacity < 1e-5)
						e->add(entitiesToDestroy);
				}
			}

			{ // hurt
				if (statistics.updateIterationIgnorePause == statistics.monstersLastHit && !game.cinematic)
				{
					entity *e = entities()->createUnique();
					CAGE_COMPONENT_ENGINE(transform, t, e);
					t.orientation = skyboxOrientation;
					CAGE_COMPONENT_ENGINE(render, r, e);
					r.renderMask = 2;
					r.object = hashString("degrid/environment/skyboxes/skybox.obj;hurt");
					r.opacity = 1;
					DEGRID_COMPONENT(skybox, s, e);
					s.dissipating = true;
				}
			}
		}

		if (game.gameOver || game.paused)
			return;

		{ // update degrid markers
			for (entity *e : gridComponent::component->entities())
			{
				CAGE_COMPONENT_ENGINE(transform, t, e);
				CAGE_COMPONENT_ENGINE(render, r, e);
				DEGRID_COMPONENT(velocity, v, e);
				DEGRID_COMPONENT(grid, g, e);
				v.velocity *= 0.95;
				v.velocity += (g.place - t.position) * 0.005;
				r.color = interpolate(r.color, g.originalColor, 0.002);
			}
		}
	}

	void gameStart()
	{
		for (entity *e : skyboxComponent::component->entities())
		{
			// prevent the skyboxes to be destroyed so that they can dissipate properly
			DEGRID_COMPONENT(skybox, s, e);
			if (!s.dissipating)
				e->remove(entitiesToDestroy);
		}

		if (game.cinematic)
			setSkybox(hashString("degrid/environment/skyboxes/skybox.obj;menu"));
		else
			setSkybox(hashString("degrid/environment/skyboxes/skybox.obj;0"));

		{ // the sun
			entity *light = entities()->createUnique();
			CAGE_COMPONENT_ENGINE(transform, t, light);
			t.orientation = quat(degs(-55), degs(70), degs());
			CAGE_COMPONENT_ENGINE(light, l, light);
			l.lightType = lightTypeEnum::Directional;
			l.color = vec3(3);
		}

		const real radius = mapNoPullRadius * 1.5;
#ifdef CAGE_DEBUG
		const real step = 50;
#else
		const real step = 12;
#endif
		for (real y = -radius; y < radius + 1e-3; y += step)
		{
			for (real x = -radius; x < radius + 1e-3; x += step)
			{
				const real d = length(vec3(x, 0, y));
				if (d > radius || d < 1e-7)
					continue;
				entity *e = entities()->createUnique();
				DEGRID_COMPONENT(velocity, velocity, e);
				velocity.velocity = randomDirection3();
				DEGRID_COMPONENT(grid, grid, e);
				grid.place = vec3(x, -2, y) + vec3(randomChance(), randomChance() * 0.1, randomChance()) * 2 - 1;
				CAGE_COMPONENT_ENGINE(transform, transform, e);
				transform.scale = 0.6;
				transform.position = grid.place + randomDirection3() * vec3(10, 0.1, 10);
				CAGE_COMPONENT_ENGINE(render, render, e);
				render.object = hashString("degrid/environment/grid.object");
				real ang = real(atan2(x, y)) / (real::Pi() * 2) + 0.5;
				real dst = d / radius;
				render.color = convertHsvToRgb(vec3(ang, 1, interpolate(real(0.5), real(0.2), sqr(dst))));
				grid.originalColor = render.color;
			}
		}

#ifdef CAGE_DEBUG
		const real angStep = 5;
#else
		const real angStep = 2;
#endif
		for (rads ang = degs(0); ang < degs(360); ang += degs(angStep))
		{
			entity *e = entities()->createUnique();
			DEGRID_COMPONENT(grid, grid, e);
			CAGE_COMPONENT_ENGINE(transform, transform, e);
			transform.position = grid.place = vec3(sin(ang), 0, cos(ang)) * (radius + step * 0.5);
			transform.scale = 0.6;
			CAGE_COMPONENT_ENGINE(render, render, e);
			render.object = hashString("degrid/environment/grid.object");
			grid.originalColor = render.color = vec3(1);
		}

		statistics.environmentGridMarkers = gridComponent::component->group()->count();
	}

	class callbacksClass
	{
		eventListener<void()> engineInitListener;
		eventListener<void()> engineUpdateListener;
		eventListener<void()> gameStartListener;
	public:
		callbacksClass()
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
		for (entity *e : skyboxComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(transform, t, e);
			t.position[2] *= 0.9; // move the sky-box closer to the camera
			DEGRID_COMPONENT(skybox, s, e);
			s.dissipating = true;
		}
	}

	{ // create new sky-box
		entity *e = entities()->createUnique();
		CAGE_COMPONENT_ENGINE(transform, t, e);
		t.position[2] = -1e-5; // semitransparent objects are rendered back-to-front; this makes the sky-box the furthest
		CAGE_COMPONENT_ENGINE(render, r, e);
		r.renderMask = 2;
		r.object = objectName;
		DEGRID_COMPONENT(skybox, s, e);
	}
}

void environmentExplosion(const vec3 &position, const vec3 &velocity, const vec3 &color, real size, real scale)
{
	statistics.environmentExplosions++;

	// colorize nearby grids
	spatialSearchQuery->intersection(sphere(position, size));
	for (uint32 otherName : spatialSearchQuery->result())
	{
		if (!entities()->has(otherName))
			continue;
		entity *e = entities()->get(otherName);
		if (!e->has(gridComponent::component))
			continue;
		CAGE_COMPONENT_ENGINE(transform, ot, e);
		CAGE_COMPONENT_ENGINE(render, orc, e);
		DEGRID_COMPONENT(velocity, og, e);
		vec3 toOther = ot.position - position;
		vec3 change = normalize(toOther) * (size / max(2, squaredLength(toOther))) * 2;
		og.velocity += change;
		ot.position += change * 2;
		orc.color = interpolate(color, orc.color, length(toOther) / size);
	}

	// create some debris
	uint32 cnt = numeric_cast<uint32>(clamp(size, 2, 40));
	for (uint32 i = 0; i < cnt; i++)
	{
		entity *e = entities()->createAnonymous();
		DEGRID_COMPONENT(timeout, timeout, e);
		timeout.ttl = numeric_cast<uint32>(randomChance() * 5 + 10);
		DEGRID_COMPONENT(velocity, vel, e);
		vel.velocity = randomDirection3();
		if (squaredLength(velocity) > 0)
			vel.velocity = normalize(vel.velocity + normalize(velocity) * sqrt(length(velocity)));
		vel.velocity *= randomChance() + 0.5;
		CAGE_COMPONENT_ENGINE(transform, transform, e);
		transform.scale = scale * (randomRange(0.8, 1.2) * 0.35);
		transform.position = position + randomRange3(-1, 1) * scale;
		transform.orientation = randomDirectionQuat();
		CAGE_COMPONENT_ENGINE(render, render, e);
		render.object = hashString("degrid/environment/explosion.object");
		render.color = colorVariation(color) * 2;
		e->add(entitiesPhysicsEvenWhenPaused);
		CAGE_COMPONENT_ENGINE(light, light, e);
		light.color = render.color;
		light.lightType = lightTypeEnum::Point;
		light.attenuation = vec3(0, 0, 0.02);
	}
}

void monsterExplosion(entity *e)
{
	CAGE_COMPONENT_ENGINE(transform, t, e);
	CAGE_COMPONENT_ENGINE(render, r, e);
	DEGRID_COMPONENT(velocity, v, e);
	DEGRID_COMPONENT(monster, m, e);
	environmentExplosion(t.position, v.velocity, r.color, min(m.damage, 10) + 2, t.scale);
}

void shotExplosion(entity *e)
{
	CAGE_COMPONENT_ENGINE(transform, t, e);
	CAGE_COMPONENT_ENGINE(render, r, e);
	DEGRID_COMPONENT(velocity, v, e);
	environmentExplosion(t.position, v.velocity, r.color, 5, t.scale * 2);
}

vec3 colorVariation(const vec3 &color)
{
	vec3 dev = randomChance3() * 0.1 - 0.05;
	vec3 hsv = convertRgbToHsv(color) + dev;
	hsv[0] = (hsv[0] + 1) % 1;
	return convertHsvToRgb(clamp(hsv, vec3(0), vec3(1)));
}
