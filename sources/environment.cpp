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
	void engineUpdate()
	{
		if (game.gameOver || game.paused)
			return;

		{ // update grid markers
			for (entityClass *e : gridComponent::component->getComponentEntities()->entities())
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
		{
			entityClass *light = entities()->newUniqueEntity();
			ENGINE_GET_COMPONENT(transform, t, light);
			t.orientation = quat(degs(), degs(45), degs());
			ENGINE_GET_COMPONENT(light, l, light);
			l.lightType = lightTypeEnum::Directional;
			l.color = vec3(1, 1, 1) * 2;
		}

		const real radius = mapNoPullRadius * 1.5;
#ifdef CAGE_DEBUG
		const real step = 50;
#else
		const real step = 10;
#endif
		for (real y = -radius; y < radius + 1e-3; y += step)
		{
			for (real x = -radius; x < radius + 1e-3; x += step)
			{
				const real d = vec3(x, 0, y).length();
				if (d > radius || d < 1e-7)
					continue;
				entityClass *e = entities()->newUniqueEntity();
				GRID_GET_COMPONENT(velocity, velocity, e);
				velocity.velocity = randomDirection3();
				GRID_GET_COMPONENT(grid, grid, e);
				grid.place = vec3(x, -2, y) + vec3(random(), random() * 0.1, random()) * 2 - 1;
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
			entityClass *e = entities()->newUniqueEntity();
			GRID_GET_COMPONENT(grid, grid, e);
			ENGINE_GET_COMPONENT(transform, transform, e);
			transform.position = grid.place = vec3(sin(ang), 0, cos(ang)) * (radius + step * 0.5);
			transform.scale = 0.6;
			ENGINE_GET_COMPONENT(render, render, e);
			render.object = hashString("grid/environment/grid.object");
			grid.originalColor = render.color = vec3(1, 1, 1);
		}

		statistics.environmentGridMarkers = gridComponent::component->getComponentEntities()->entitiesCount();
	}

	class callbacksClass
	{
		eventListener<void()> engineUpdateListener;
		eventListener<void()> gameStartListener;
	public:
		callbacksClass()
		{
			engineUpdateListener.attach(controlThread().update, -5);
			engineUpdateListener.bind<&engineUpdate>();
			gameStartListener.attach(gameStartEvent(), -5);
			gameStartListener.bind<&gameStart>();
		}
	} callbacksInstance;
}

void environmentExplosion(const vec3 &position, const vec3 &velocity, const vec3 &color, real size, real scale)
{
	statistics.environmentExplosions++;

	// colorize nearby grids
	spatialQuery->intersection(sphere(position, size));
	for (uint32 otherName : spatialQuery->result())
	{
		if (!entities()->hasEntity(otherName))
			continue;
		entityClass *e = entities()->getEntity(otherName);
		if (!e->hasComponent(gridComponent::component))
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
	uint32 cnt = numeric_cast<uint32>(clamp(size, 2, 20));
	for (uint32 i = 0; i < cnt; i++)
	{
		entityClass *e = entities()->newAnonymousEntity();
		GRID_GET_COMPONENT(timeout, timeout, e);
		timeout.ttl = numeric_cast<uint32>(random() * 5 + 10);
		GRID_GET_COMPONENT(velocity, vel, e);
		vel.velocity = randomDirection3();
		if (velocity.squaredLength() > 0)
			vel.velocity = normalize(vel.velocity + velocity.normalize() * velocity.length().sqrt());
		vel.velocity *= random() + 0.5;
		ENGINE_GET_COMPONENT(transform, transform, e);
		transform.scale = scale * (random() * 0.4 + 0.8) * 0.4;
		transform.position = position + vec3(random() * 2 - 1, 0, random() * 2 - 1);
		transform.orientation = randomDirectionQuat();
		ENGINE_GET_COMPONENT(render, render, e);
		render.object = hashString("grid/environment/explosion.object");
		render.color = colorVariation(color);
		e->addGroup(entitiesPhysicsEvenWhenPaused);
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
	vec3 dev = vec3(random(), random(), random()) * 0.1 - 0.05;
	vec3 hsv = convertRgbToHsv(color) + dev;
	hsv[0] = (hsv[0] + 1) % 1;
	return convertHsvToRgb(clamp(hsv, vec3(0, 0, 0), vec3(1, 1, 1)));
}
