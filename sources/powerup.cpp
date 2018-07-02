#include "includes.h"
#include "game.h"

namespace grid
{
	void powerupUpdate()
	{
		if (player.paused)
			return;

		{ // decrease timed power ups
			for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
			{
				if (powerupMode[i] == 1 && player.powerups[i] > 0)
					player.powerups[i]--;
			}
		}

		{ // update entities
			for (entityClass *e : powerupStruct::component->getComponentEntities()->entities())
			{
				GRID_GET_COMPONENT(powerup, p, e);
				CAGE_ASSERT_RUNTIME(p.type < powerupTypeEnum::Total, p.type);
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
				switch (powerupMode[(uint32)p.type])
				{
				case 0: // collectible
					if (player.powerups[(uint32)p.type] < 3)
					{
						player.powerups[(uint32)p.type]++;
						switch (p.type)
						{
						case powerupTypeEnum::Bomb: soundSpeech(hashString("grid/speech/pickup/a-bomb.wav")); break;
						case powerupTypeEnum::Decoy: soundSpeech(hashString("grid/speech/pickup/a-decoy.wav")); break;
						case powerupTypeEnum::Turret: soundSpeech(hashString("grid/speech/pickup/a-turret.wav")); break;
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
					player.powerups[(uint32)p.type] += 30 * 30;
					switch (p.type)
					{
					case powerupTypeEnum::Shield: soundSpeech(hashString("grid/speech/pickup/shield-engaged.wav")); break;
					case powerupTypeEnum::HomingShots: soundSpeech(hashString("grid/speech/pickup/homing-missiles.wav")); break;
					case powerupTypeEnum::SuperDamage: soundSpeech(hashString("grid/speech/pickup/super-damage.wav")); break;
					default: CAGE_THROW_CRITICAL(exception, "invalid powerup type");
					}
				} break;
				case 2: // permanent
				{
					uint32 sum = 0;
					for (uint32 i = (uint32)powerupTypeEnum::MaxSpeed; i < (uint32)powerupTypeEnum::Total; i++)
						sum += player.powerups[i];
					if (sum < 5)
					{
						player.powerups[(uint32)p.type]++;
						switch (p.type)
						{
						case powerupTypeEnum::Acceleration: soundSpeech(hashString("grid/speech/pickup/acceleration-improved.wav")); break;
						case powerupTypeEnum::MaxSpeed: soundSpeech(hashString("grid/speech/pickup/movement-speed-improved.wav")); break;
						case powerupTypeEnum::Multishot: soundSpeech(hashString("grid/speech/pickup/additional-cannon.wav")); break;
						case powerupTypeEnum::ShotsSpeed: soundSpeech(hashString("grid/speech/pickup/missiles-speed-improved.wav")); break;
						case powerupTypeEnum::ShotsDamage: soundSpeech(hashString("grid/speech/pickup/missiles-damage-improved.wav")); break;
						case powerupTypeEnum::FiringSpeed: soundSpeech(hashString("grid/speech/pickup/firing-speed-improved.wav")); break;
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
		entityClass *e = entities()->newUniqueEntity();
		ENGINE_GET_COMPONENT(transform, transform, e);
		transform.position = position * vec3(1, 0, 1);
		transform.orientation = quat(randomAngle(), randomAngle(), randomAngle());
		transform.scale = 2.5;
		GRID_GET_COMPONENT(powerup, p, e);
		p.animation = quat(randomAngle() / 100, randomAngle() / 100, randomAngle() / 100);
		p.timeout = 120 * 30;
		p.type = (powerupTypeEnum)random(0u, (uint32)powerupTypeEnum::Total);
		ENGINE_GET_COMPONENT(render, render, e);
		static const uint32 objectName[3] = {
			hashString("grid/player/powerupCollectible.object"),
			hashString("grid/player/powerupOnetime.object"),
			hashString("grid/player/powerupPermanent.object")
		};
		render.object = objectName[powerupMode[(uint32)p.type]];
		render.color = convertHsvToRgb(vec3(random(), 1, 1));
		soundEffect(hashString("grid/player/powerup.ogg"), transform.position);
	}
}
