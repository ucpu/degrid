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
	Quat skyboxOrientation;
	Quat skyboxRotation;

	struct SkyboxComponent
	{
		static EntityComponent *component;

		bool dissipating = false;
	};

	EntityComponent *SkyboxComponent::component;

	void engineInit()
	{
		SkyboxComponent::component = engineEntities()->defineComponent(SkyboxComponent());
		skyboxOrientation = randomDirectionQuat();
		skyboxRotation = interpolate(Quat(), randomDirectionQuat(), 5e-5);
	}

	void engineUpdate()
	{
		{ // update skybox
			skyboxOrientation = skyboxRotation * skyboxOrientation;

			for (Entity *e : SkyboxComponent::component->entities())
			{
				TransformComponent &t = e->value<TransformComponent>();
				t.orientation = skyboxOrientation;
				SkyboxComponent &s = e->value<SkyboxComponent>();
				if (s.dissipating)
				{
					RenderComponent &r = e->value<RenderComponent>();
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
					TransformComponent &t = e->value<TransformComponent>();
					t.orientation = randomDirectionQuat();
					RenderComponent &r = e->value<RenderComponent>();
					r.sceneMask = 2;
					r.object = HashString("degrid/environment/skyboxes/skybox.obj;hurt");
					r.opacity = 1;
					SkyboxComponent &s = e->value<SkyboxComponent>();
					s.dissipating = true;
				}
			}
		}

		if (game.gameOver || game.paused)
			return;

		{ // update degrid markers
			for (Entity *e : engineEntities()->component<GridComponent>()->entities())
			{
				TransformComponent &t = e->value<TransformComponent>();
				RenderComponent &r = e->value<RenderComponent>();
				VelocityComponent &v = e->value<VelocityComponent>();
				GridComponent &g = e->value<GridComponent>();
				v.velocity *= 0.95;
				v.velocity += (g.place - t.position) * 0.005;
				r.color = interpolate(r.color, g.originalColor, 0.002);
			}
		}
	}

	void gameStart()
	{
		for (Entity *e : engineEntities()->component<SkyboxComponent>()->entities())
		{
			// prevent the skyboxes to be destroyed so that they can dissipate properly
			SkyboxComponent &s = e->value<SkyboxComponent>();
			e->remove(entitiesToDestroy);
		}

		if (game.cinematic)
			setSkybox(HashString("degrid/environment/skyboxes/skybox.obj;menu"));
		else
			setSkybox(HashString("degrid/environment/skyboxes/skybox.obj;0"));

		{ // the sun
			Entity *light = engineEntities()->createUnique();
			TransformComponent &t = light->value<TransformComponent>();
			t.orientation = Quat(Degs(-55), Degs(70), Degs());
			LightComponent &l = light->value<LightComponent>();
			l.lightType = LightTypeEnum::Directional;
			l.color = Vec3(1);
			l.intensity = 3;
		}

		constexpr Real radius = MapNoPullRadius + PlayerScale;
#ifdef CAGE_DEBUG
		constexpr Real step = 50;
#else
		constexpr Real step = 12;
#endif
		for (Real y = -radius; y < radius + 1e-3; y += step)
		{
			for (Real x = -radius; x < radius + 1e-3; x += step)
			{
				const Real d = length(Vec3(x, 0, y));
				if (d > radius || d < 1e-7)
					continue;
				Entity *e = engineEntities()->createUnique();
				VelocityComponent &velocity = e->value<VelocityComponent>();
				velocity.velocity = randomDirection3();
				GridComponent &grid = e->value<GridComponent>();
				grid.place = Vec3(x, -2, y) + Vec3(randomChance(), randomChance() * 0.1, randomChance()) * 2 - 1;
				TransformComponent &Transform = e->value<TransformComponent>();
				Transform.scale = 0.7;
				Transform.position = grid.place + randomDirection3() * Vec3(10, 0.1, 10);
				RenderComponent &render = e->value<RenderComponent>();
				render.object = HashString("degrid/environment/grid.object");
				Real ang = Real(atan2(x, y)) / (Real::Pi() * 2) + 0.5;
				Real dst = d / radius;
				render.color = colorHsvToRgb(Vec3(ang, 1, interpolate(Real(0.6), Real(0.4), sqr(dst))));
				grid.originalColor = render.color;
			}
		}

#ifdef CAGE_DEBUG
		constexpr Rads angStep = Degs(9);
#else
		constexpr Rads angStep = Degs(3);
#endif
		for (Rads ang = Degs(0); ang < Degs(360); ang += angStep)
		{
			Entity *e = engineEntities()->createUnique();
			GridComponent &grid = e->value<GridComponent>();
			TransformComponent &Transform = e->value<TransformComponent>();
			Transform.position = grid.place = Vec3(sin(ang), 0, cos(ang)) * (radius + step * 0.5);
			Transform.scale = 0.6;
			RenderComponent &render = e->value<RenderComponent>();
			render.object = HashString("degrid/environment/grid.object");
			grid.originalColor = render.color = Vec3(1);
		}

		statistics.environmentGridMarkers = engineEntities()->component<GridComponent>()->count();
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
		for (Entity *e : engineEntities()->component<SkyboxComponent>()->entities())
		{
			TransformComponent &t = e->value<TransformComponent>();
			t.position[2] *= 0.9; // move the sky-box closer to the camera
			SkyboxComponent &s = e->value<SkyboxComponent>();
			s.dissipating = true;
		}
	}

	{ // create new sky-box
		Entity *e = engineEntities()->createUnique();
		TransformComponent &t = e->value<TransformComponent>();
		t.position[2] = -1e-5; // semitransparent objects are rendered back-to-front; this makes the sky-box the furthest
		RenderComponent &r = e->value<RenderComponent>();
		r.sceneMask = 2;
		r.object = objectName;
		r.opacity = 1;
		SkyboxComponent &s = e->value<SkyboxComponent>();
	}
}

void environmentExplosion(const Vec3 &position, const Vec3 &velocity, const Vec3 &color, Real size)
{
	statistics.environmentExplosions++;

	// colorize nearby grids
	spatialSearchQuery->intersection(Sphere(position, size * 2));
	for (uint32 otherName : spatialSearchQuery->result())
	{
		if (!engineEntities()->has(otherName))
			continue;
		Entity *e = engineEntities()->get(otherName);
		if (!e->has<GridComponent>())
			continue;
		TransformComponent &ot = e->value<TransformComponent>();
		RenderComponent &orc = e->value<RenderComponent>();
		VelocityComponent &og = e->value<VelocityComponent>();
		Vec3 toOther = ot.position - position;
		Real dist = length(toOther);
		Real intpr = smoothstep(((size - dist) / size + 1) * 0.5);
		Vec3 dir = normalize(toOther);
		Vec3 change = dir * intpr;
		og.velocity += change;
		ot.position += change * 2;
		orc.color = interpolate(color, orc.color, intpr);
	}

	// create some debris
	uint32 cnt = numeric_cast<uint32>(size * size * 0.5 + 2);
	for (uint32 i = 0; i < cnt; i++)
	{
		Real scale = randomChance();
		Entity *e = engineEntities()->createAnonymous();
		TimeoutComponent &timeout = e->value<TimeoutComponent>();
		timeout.ttl = randomRange(5, 25);
		VelocityComponent &vel = e->value<VelocityComponent>();
		vel.velocity = velocity * 0.5 + randomDirection3() * (1.1 - scale);
		TransformComponent &Transform = e->value<TransformComponent>();
		Transform.scale = randomRange(1.0, 1.3) * (scale * 0.4 + 0.8);
		Transform.position = position + randomDirection3() * (randomChance() * size);
		Transform.orientation = randomDirectionQuat();
		RenderComponent &render = e->value<RenderComponent>();
		render.object = HashString("degrid/environment/explosion.object");
		render.color = colorVariation(color);
		e->add(entitiesPhysicsEvenWhenPaused);
	}

	// create light
	{
		Entity *e = engineEntities()->createAnonymous();
		TimeoutComponent &timeout = e->value<TimeoutComponent>();
		timeout.ttl = randomRange(5, 10);
		VelocityComponent &vel = e->value<VelocityComponent>();
		vel.velocity = velocity * 0.3 + randomDirection3() * 0.02;
		TransformComponent &Transform = e->value<TransformComponent>();
		Transform.position = position + randomDirection3() * Vec3(1, 0.1, 1) * size * randomChance();
		Transform.position[1] = abs(Transform.position[1]); 
		e->add(entitiesPhysicsEvenWhenPaused);
		LightComponent &light = e->value<LightComponent>();
		Vec3 colorinv = colorRgbToHsv(color);
		colorinv[0] = (colorinv[0] + 0.5) % 1;
		colorinv = colorHsvToRgb(colorinv);
		light.color = colorVariation(colorinv);
		light.intensity = randomRange(0.8, 1.2);
		light.lightType = LightTypeEnum::Point;
		light.attenuation = Vec3(0, 0, 0.005);
	}
}

void monsterExplosion(Entity *e)
{
	TransformComponent &t = e->value<TransformComponent>();
	RenderComponent &r = e->value<RenderComponent>();
	VelocityComponent &v = e->value<VelocityComponent>();
	MonsterComponent &m = e->value<MonsterComponent>();
	environmentExplosion(t.position, v.velocity, r.color, t.scale);
}

void shotExplosion(Entity *e)
{
	TransformComponent &t = e->value<TransformComponent>();
	RenderComponent &r = e->value<RenderComponent>();
	VelocityComponent &v = e->value<VelocityComponent>();
	environmentExplosion(t.position, v.velocity, r.color, t.scale);
}

Vec3 colorVariation(const Vec3 &color)
{
	Vec3 dev = randomChance3() * 0.1 - 0.05;
	Vec3 hsv = colorRgbToHsv(color) + dev;
	hsv[0] = (hsv[0] + 1) % 1;
	return colorHsvToRgb(clamp(hsv, 0, 1));
}
