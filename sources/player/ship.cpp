#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/spatialStructure.h>
#include <cage-core/hashString.h>

#include "../game.h"

extern ConfigUint32 confControlMovement;

namespace
{
	uint64 scorePreviousSound;
	uint64 scorePreviousAchievements;

	void shipMovement()
	{
		if (game.cinematic)
			return;

		TransformComponent &tr = game.playerEntity->value<TransformComponent>();
		VelocityComponent &vl = game.playerEntity->value<VelocityComponent>();

		if (game.moveDirection != Vec3())
		{
			CAGE_ASSERT(abs(lengthSquared(game.moveDirection) - 1) < 1e-3);
			const Real maxSpeed = game.powerups[(uint32)PowerupTypeEnum::MaxSpeed] * 0.3 + 0.8;
			const Vec3 change = game.moveDirection * (game.powerups[(uint32)PowerupTypeEnum::Acceleration] + 1) * 0.1;
			if (confControlMovement == 1 && dot(tr.orientation * Vec3(0, 0, -1), normalize(vl.velocity + change)) < 1e-5)
				vl.velocity = Vec3();
			else
				vl.velocity += change;
			if (lengthSquared(vl.velocity) > sqr(maxSpeed))
				vl.velocity = normalize(vl.velocity) * maxSpeed;
			if (lengthSquared(change) > 0.01)
			{
				Entity *spark = engineEntities()->createAnonymous();
				TransformComponent &transform = spark->value<TransformComponent>();
				transform.scale = randomChance() * 0.2 + 0.3;
				transform.position = tr.position + tr.orientation * Vec3((sint32)(statistics.updateIterationIgnorePause % 2) * 1.2 - 0.6, 0, 1) * tr.scale;
				transform.orientation = randomDirectionQuat();
				spark->value<RenderComponent>().object = HashString("degrid/environment/spark.object");
				spark->value<VelocityComponent>().velocity = (change + randomDirection3() * 0.05) * randomChance() * -5;
				TimeoutComponent &ttl = spark->value<TimeoutComponent>();
				ttl.ttl = randomRange(10, 15);
				TextureAnimationComponent &at = spark->value<TextureAnimationComponent>();
				at.startTime = engineControlTime();
				at.speed = 30.f / ttl.ttl;
				spark->add(entitiesPhysicsEvenWhenPaused);
			}
		}
		else
			vl.velocity *= 0.97;

		// pull to center
		if (length(tr.position) > MapNoPullRadius)
		{
			const Vec3 pullToCenter = -normalize(tr.position) * pow((length(tr.position) - MapNoPullRadius) * 0.02, 2);
			vl.velocity += pullToCenter;
		}

		vl.velocity[1] = 0;
		tr.position[1] = 0.5;
		game.monstersTarget = tr.position + vl.velocity * 3;
		if (lengthSquared(vl.velocity) > 1e-5)
			tr.orientation = Quat(Degs(), atan2(-vl.velocity[2], -vl.velocity[0]), Degs());
	}

	const auto shieldListener = controlThread().update.listen([]() {
		if (!game.playerEntity || !game.shieldEntity)
			return;
		const TransformComponent &tr = game.playerEntity->value<TransformComponent>();
		TransformComponent &trs = game.shieldEntity->value<TransformComponent>();
		trs.position = tr.position;
		trs.scale = tr.scale;
		if (game.powerups[(uint32)PowerupTypeEnum::Shield] > 0)
		{
			game.shieldEntity->value<RenderComponent>().object = HashString("degrid/player/shield.object");
			SoundComponent &sound = game.shieldEntity->value<SoundComponent>();
			sound.name = HashString("degrid/player/shield.ogg");
			sound.startTime = -1;
		}
		else
		{
			game.shieldEntity->remove<RenderComponent>();
			game.shieldEntity->remove<SoundComponent>();
		}
	}, 40); // after physics

	void checkScore(uint32 limit, const char *name)
	{
		if (game.score >= limit && scorePreviousAchievements < limit)
			achievementFullfilled(name);
	}

	void scoreUpdate()
	{
		if (game.jokeMap)
		{
			checkScore(5000, "joke-map");
		}
		else
		{
			checkScore(5000, "starting-kit");
			checkScore(20000, "bronze");
			checkScore(100000, "silver");
			checkScore(1000000, "gold");
		}
		scorePreviousAchievements = game.score;

		const uint64 lg = scorePreviousSound >= 20000 ? 10 : scorePreviousSound >= 2000 ? 2 : 1;
		const uint64 sg = lg * 500;
		const uint64 ld = (game.score - scorePreviousSound) / sg;
		if (ld)
		{
			scorePreviousSound += ld * sg;

			static constexpr const uint32 Sounds[] = {
				HashString("degrid/speech/progress/doing-fine.wav"),
				HashString("degrid/speech/progress/doing-well.wav"),
				HashString("degrid/speech/progress/fantastic.wav"),
				HashString("degrid/speech/progress/go-on.wav"),
				HashString("degrid/speech/progress/i-hope-the-princess-is-worth-all-the-trouble.wav"),
				HashString("degrid/speech/progress/keep-going.wav"),
				HashString("degrid/speech/progress/lets-roll.wav"),
				HashString("degrid/speech/progress/they-say-the-princess-is-very-beautiful.wav"),
				0
			};
			soundSpeech(Sounds);
		}
	}

	const auto engineUpdateListener = controlThread().update.listen([]() {
		if (!game.paused)
		{
			shipMovement();
			scoreUpdate();
			if (!game.cinematic && game.life <= 1e-7)
			{
				game.life = 0;
				gameStopEvent().dispatch();
			}
		}
	}, -20);

	const auto gameStartListener = gameStartEvent().listen([]() {
		scorePreviousSound = 0;

		{ // player ship Entity
			game.playerEntity = engineEntities()->createUnique();
			game.playerEntity->value<TransformComponent>().scale = PlayerScale;
			game.playerEntity->value<RenderComponent>().object = HashString("degrid/player/player.object");
			game.monstersTarget = Vec3();
		}

		{ // player shield Entity
			game.shieldEntity = engineEntities()->createUnique();
			game.shieldEntity->value<TransformComponent>();
			game.shieldEntity->value<TextureAnimationComponent>().speed = 0.05;
		}
	}, -20);

	const auto gameStopListener = gameStopEvent().listen([]() {
		environmentExplosion(game.playerEntity->value<TransformComponent>().position, game.playerEntity->value<VelocityComponent>().velocity, PlayerDeathColor, PlayerScale);
		game.playerEntity->add(entitiesToDestroy);
		game.playerEntity = nullptr;
		game.shieldEntity->add(entitiesToDestroy);
		game.shieldEntity = nullptr;
	}, -20);
}
