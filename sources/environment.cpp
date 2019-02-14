#include "game.h"

#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/assets.h>
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
		static componentClass *component;

		bool dissipating;

		skyboxComponent() : dissipating(false)
		{}
	};

	componentClass *skyboxComponent::component;

	void engineInit()
	{
		skyboxComponent::component = entities()->defineComponent(skyboxComponent(), true);
		skyboxOrientation = randomDirectionQuat();
		skyboxRotation = interpolate(quat(), randomDirectionQuat(), 5e-5);
	}

	void engineUpdate()
	{
		{ // update skybox
			skyboxOrientation = skyboxRotation * skyboxOrientation;

			for (entityClass *e : skyboxComponent::component->entities())
			{
				ENGINE_GET_COMPONENT(transform, t, e);
				t.orientation = skyboxOrientation;
				GRID_GET_COMPONENT(skybox, s, e);
				if (s.dissipating)
				{
					ENGINE_GET_COMPONENT(render, r, e);
					r.opacity -= 0.05;
					if (r.opacity < 1e-5)
						e->add(entitiesToDestroy);
				}
			}

			{ // hurt
				if (statistics.updateIterationIgnorePause == statistics.monstersLastHit && !game.cinematic)
				{
					entityClass *e = entities()->createUnique();
					ENGINE_GET_COMPONENT(transform, t, e);
					ENGINE_GET_COMPONENT(render, r, e);
					r.renderMask = 2;
					r.object = hashString("grid/environment/skyboxes/skybox.obj;hurt");
					GRID_GET_COMPONENT(skybox, s, e);
					s.dissipating = true;
				}
			}
		}

		if (game.gameOver || game.paused)
			return;

		{ // update grid markers
			for (entityClass *e : gridComponent::component->entities())
			{
				ENGINE_GET_COMPONENT(transform, t, e);
				ENGINE_GET_COMPONENT(render, r, e);
				GRID_GET_COMPONENT(velocity, v, e);
				GRID_GET_COMPONENT(grid, g, e);
				v.velocity *= 0.95;
				v.velocity += (g.place - t.position) * 0.005;
				r.color = interpolate(r.color, g.originalColor, 0.002);
			}
		}
	}

	void gameStart()
	{
		for (entityClass *e : skyboxComponent::component->entities())
		{
			// prevent the skyboxes to be destroyed so that they can dissipate properly
			GRID_GET_COMPONENT(skybox, s, e);
			if (!s.dissipating)
				e->remove(entitiesToDestroy);
		}

		if (game.cinematic)
			setSkybox(hashString("grid/environment/skyboxes/skybox.obj;menu"));
		else
			setSkybox(hashString("grid/environment/skyboxes/skybox.obj;0"));

		{
			entityClass *light = entities()->createUnique();
			ENGINE_GET_COMPONENT(transform, t, light);
			t.orientation = quat(degs(-55), degs(70), degs());
			ENGINE_GET_COMPONENT(light, l, light);
			l.lightType = lightTypeEnum::Directional;
			l.color = vec3(2);
		}

		const real radius = mapNoPullRadius * 1.5;
#ifdef CAGE_DEBUG
		const real step = 50;
#else
		const real step = 8;
#endif
		for (real y = -radius; y < radius + 1e-3; y += step)
		{
			for (real x = -radius; x < radius + 1e-3; x += step)
			{
				const real d = vec3(x, 0, y).length();
				if (d > radius || d < 1e-7)
					continue;
				entityClass *e = entities()->createUnique();
				GRID_GET_COMPONENT(velocity, velocity, e);
				velocity.velocity = randomDirection3();
				GRID_GET_COMPONENT(grid, grid, e);
				grid.place = vec3(x, -2, y) + vec3(randomChance(), randomChance() * 0.1, randomChance()) * 2 - 1;
				ENGINE_GET_COMPONENT(transform, transform, e);
				transform.scale = 0.6;
				transform.position = grid.place + randomDirection3() * vec3(10, 0.1, 10);
				ENGINE_GET_COMPONENT(render, render, e);
				render.object = hashString("grid/environment/grid.object");
				real ang = real(aTan2(x, y)) / real::TwoPi + 0.5;
				real dst = d / radius;
				render.color = convertHsvToRgb(vec3(ang, 1, interpolate(real(0.5), real(0.2), sqr(dst))));
				grid.originalColor = render.color;
			}
		}

#ifdef CAGE_DEBUG
		const real angStep = 5;
#else
		const real angStep = 1;
#endif
		for (rads ang = degs(0); ang < degs(360); ang += degs(angStep))
		{
			entityClass *e = entities()->createUnique();
			GRID_GET_COMPONENT(grid, grid, e);
			ENGINE_GET_COMPONENT(transform, transform, e);
			transform.position = grid.place = vec3(sin(ang), 0, cos(ang)) * (radius + step * 0.5);
			transform.scale = 0.6;
			ENGINE_GET_COMPONENT(render, render, e);
			render.object = hashString("grid/environment/grid.object");
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
	{ // initiate dissapearing of old skyboxes
		for (entityClass *e : skyboxComponent::component->entities())
		{
			ENGINE_GET_COMPONENT(transform, t, e);
			t.position[2] *= 0.9; // move the skybox closer to camera
			GRID_GET_COMPONENT(skybox, s, e);
			s.dissipating = true;
		}
	}

	{ // create new skybox
		entityClass *e = entities()->createUnique();
		ENGINE_GET_COMPONENT(transform, t, e);
		t.position[2] = -1e-5; // semitransparent objects are rendered back-to-front; this makes the skybox the furthest
		ENGINE_GET_COMPONENT(render, r, e);
		r.renderMask = 2;
		r.object = objectName;
		GRID_GET_COMPONENT(skybox, s, e);
	}
}

void environmentExplosion(const vec3 &position, const vec3 &velocity, const vec3 &color, real size, real scale)
{
	statistics.environmentExplosions++;

	// colorize nearby grids
	spatialQuery->intersection(sphere(position, size));
	for (uint32 otherName : spatialQuery->result())
	{
		if (!entities()->has(otherName))
			continue;
		entityClass *e = entities()->get(otherName);
		if (!e->has(gridComponent::component))
			continue;
		ENGINE_GET_COMPONENT(transform, ot, e);
		ENGINE_GET_COMPONENT(render, orc, e);
		GRID_GET_COMPONENT(velocity, og, e);
		vec3 toOther = ot.position - position;
		vec3 change = toOther.normalize() * (size / max(2, toOther.squaredLength())) * 2;
		og.velocity += change;
		ot.position += change * 2;
		orc.color = interpolate(color, orc.color, toOther.length() / size);
	}

	// create some debris
	uint32 cnt = numeric_cast<uint32>(clamp(size, 2, 40));
	for (uint32 i = 0; i < cnt; i++)
	{
		entityClass *e = entities()->createAnonymous();
		GRID_GET_COMPONENT(timeout, timeout, e);
		timeout.ttl = numeric_cast<uint32>(randomChance() * 5 + 10);
		GRID_GET_COMPONENT(velocity, vel, e);
		vel.velocity = randomDirection3();
		if (velocity.squaredLength() > 0)
			vel.velocity = normalize(vel.velocity + velocity.normalize() * velocity.length().sqrt());
		vel.velocity *= randomChance() + 0.5;
		ENGINE_GET_COMPONENT(transform, transform, e);
		transform.scale = scale * (randomRange(0.8, 1.2) * 0.35);
		transform.position = position + randomRange3(-1, 1) * scale;
		transform.orientation = randomDirectionQuat();
		ENGINE_GET_COMPONENT(render, render, e);
		render.object = hashString("grid/environment/explosion.object");
		render.color = colorVariation(color) * 2;
		e->add(entitiesPhysicsEvenWhenPaused);
		ENGINE_GET_COMPONENT(light, light, e);
		light.color = render.color;
		light.lightType = lightTypeEnum::Point;
		light.attenuation = vec3(0, 0, 0.01);
	}
}

void monsterExplosion(entityClass *e)
{
	ENGINE_GET_COMPONENT(transform, t, e);
	ENGINE_GET_COMPONENT(render, r, e);
	GRID_GET_COMPONENT(velocity, v, e);
	GRID_GET_COMPONENT(monster, m, e);
	environmentExplosion(t.position, v.velocity, r.color, min(m.damage, 10) + 2, t.scale);
}

void shotExplosion(entityClass *e)
{
	ENGINE_GET_COMPONENT(transform, t, e);
	ENGINE_GET_COMPONENT(render, r, e);
	GRID_GET_COMPONENT(velocity, v, e);
	environmentExplosion(t.position, v.velocity, r.color, 5, t.scale * 2);
}

vec3 colorVariation(const vec3 &color)
{
	vec3 dev = randomChance3() * 0.1 - 0.05;
	vec3 hsv = convertRgbToHsv(color) + dev;
	hsv[0] = (hsv[0] + 1) % 1;
	return convertHsvToRgb(clamp(hsv, vec3(0), vec3(1)));
}
