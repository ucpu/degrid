#include "includes.h"
#include "game.h"

namespace grid
{
	void powerupUpdate()
	{
		if (player.paused)
			return;

		{ // decrease timed power ups
			for (uint32 i = 0; i < puTotal; i++)
			{
				if (powerupMode[i] == 1 && player.powerups[i] > 0)
					player.powerups[i]--;
			}
		}

		{ // update entities
			uint32 count = powerupStruct::component->getComponentEntities()->entitiesCount();
			entityClass *const *powerups = powerupStruct::component->getComponentEntities()->entitiesArray();
			for (uint32 i = 0; i < count; i++)
			{
				entityClass *e = powerups[i];
				GRID_GET_COMPONENT(powerup, p, e);
				CAGE_ASSERT_RUNTIME(p.type < puTotal, p.type);
				if (p.timeout-- == 0)
				{
					statistics.powerupsTimedout++;
					e->addGroup(entitiesToDestroy);
					continue;
				}

				ENGINE_GET_COMPONENT(transform, tr, e);
				tr.orientation = p.animation * tr.orientation;
				if (!collisionTest(player.position, player.scale, player.speed, tr.position, tr.scale, vec3()))
				{
					vec3 toPlayer = player.position - tr.position;
					real dist = max(toPlayer.length() - player.scale - tr.scale, 1);
					tr.position += normalize(toPlayer) * (2 / dist);
					continue;
				}

				statistics.powerupsPicked++;
				e->addGroup(entitiesToDestroy);
				switch (powerupMode[p.type])
				{
				case 0: // collectible
					if (player.powerups[p.type] < 3)
					{
						player.powerups[p.type]++;
						switch (p.type)
						{
						case puBomb: soundSpeech(hashString("grid/speech/pickup/a-bomb.wav")); break;
						case puDecoy: soundSpeech(hashString("grid/speech/pickup/a-decoy.wav")); break;
						case puTurret: soundSpeech(hashString("grid/speech/pickup/a-turret.wav")); break;
						default: CAGE_THROW_CRITICAL(exception, "invalid powerup type");
						}
					}
					else
					{
						statistics.powerupsWasted++;
						player.score += 100;
						soundSpeech(hashString("grid/speech/pickup/wasted.wav"));
					}
					break;
				case 1: // timed
				{
					player.powerups[p.type] += 30 * 30;
					switch (p.type)
					{
					case puShield: soundSpeech(hashString("grid/speech/pickup/shield-engaged.wav")); break;
					case puHomingShots: soundSpeech(hashString("grid/speech/pickup/homing-missiles.wav")); break;
					case puSuperDamage: soundSpeech(hashString("grid/speech/pickup/super-damage.wav")); break;
					default: CAGE_THROW_CRITICAL(exception, "invalid powerup type");
					}
				} break;
				case 2: // permanent
				{
					uint32 sum = 0;
					for (uint32 i = puMaxSpeed; i < puTotal; i++)
						sum += player.powerups[i];
					if (sum < 5)
					{
						player.powerups[p.type]++;
						switch (p.type)
						{
						case puAcceleration: soundSpeech(hashString("grid/speech/pickup/acceleration-improved.wav")); break;
						case puMaxSpeed: soundSpeech(hashString("grid/speech/pickup/movement-speed-improved.wav")); break;
						case puMultishot: soundSpeech(hashString("grid/speech/pickup/additional-cannon.wav")); break;
						case puShotsSpeed: soundSpeech(hashString("grid/speech/pickup/missiles-speed-improved.wav")); break;
						case puShotsDamage: soundSpeech(hashString("grid/speech/pickup/missiles-damage-improved.wav")); break;
						case puFiringSpeed: soundSpeech(hashString("grid/speech/pickup/firing-speed-improved.wav")); break;
						default: CAGE_THROW_CRITICAL(exception, "invalid powerup type");
						}
					}
					else
					{
						statistics.powerupsWasted++;
						player.score += 100;
						soundSpeech(hashString("grid/speech/pickup/wasted.wav"));
					}
				} break;
				default:
					CAGE_THROW_CRITICAL(exception, "invalid powerup mode");
				}
			}
		}
	}

	void powerupSpawn(const vec3 &position)
	{
		statistics.powerupsSpawned++;
		entityClass *e = entities()->newEntity(entities()->generateUniqueName());
		ENGINE_GET_COMPONENT(transform, transform, e);
		transform.position = position * vec3(1, 0, 1);
		transform.orientation = quat(randomAngle(), randomAngle(), randomAngle());
		transform.scale = 2.5;
		GRID_GET_COMPONENT(powerup, p, e);
		p.animation = quat(randomAngle() / 100, randomAngle() / 100, randomAngle() / 100);
		p.timeout = 120 * 30;
		p.type = (powerupTypeEnum)random(0, puTotal);
		ENGINE_GET_COMPONENT(render, render, e);
		static const uint32 objectName[3] = {
			hashString("grid/player/powerupCollectible.object"),
			hashString("grid/player/powerupOnetime.object"),
			hashString("grid/player/powerupPermanent.object")
		};
		render.object = objectName[powerupMode[p.type]];
		render.color = convertHsvToRgb(vec3(random(), 1, 1));
		soundEffect(hashString("grid/player/powerup.ogg"), transform.position);
	}
}