#include "../game.h"

#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/color.h>
#include <cage-core/hashString.h>

namespace
{
	void turretsUpdate()
	{
		for (entityClass *e : turretComponent::component->getComponentEntities()->entities())
		{
			ENGINE_GET_COMPONENT(transform, tr, e);
			tr.orientation = quat(degs(), degs(1), degs()) * tr.orientation;
			GRID_GET_COMPONENT(turret, tu, e);
			if (tu.shooting > 0)
			{
				tu.shooting--;
				continue;
			}
			tu.shooting = 10;
			for (uint32 i = 0; i < 6; i++)
			{
				statistics.shotsTurret++;
				entityClass *shot = entities()->newUniqueEntity();
				ENGINE_GET_COMPONENT(transform, transform, shot);
				transform.orientation = quat(degs(), degs(i * 60), degs()) * tr.orientation;
				transform.position = tr.position + transform.orientation * vec3(0, 0, -1) * 2;
				ENGINE_GET_COMPONENT(render, render, shot);
				render.object = hashString("grid/player/shot.object");
				render.color = player.shotsColor;
				GRID_GET_COMPONENT(velocity, vel, shot);
				vel.velocity = transform.orientation * vec3(0, 0, -1) * 2.5;
				GRID_GET_COMPONENT(shot, sh, shot);
				sh.damage = 1;
				sh.homing = false;
			}
		}
	}

	void decoysUpdate()
	{
		for (entityClass *e : decoyComponent::component->getComponentEntities()->entities())
		{
			ENGINE_GET_COMPONENT(transform, tr, e);
			GRID_GET_COMPONENT(velocity, vel, e);
			GRID_GET_COMPONENT(decoy, dec, e);
			tr.orientation = dec.rotation * tr.orientation;
			tr.position[1] = 0;
			vel.velocity *= 0.97;
			player.monstersTarget = tr.position;
		}
	}

	void wastedPowerup()
	{
		statistics.powerupsWasted++;
		player.score += 100;
		soundSpeech(hashString("grid/speech/pickup/wasted.wav"));
	}

	void powerupsUpdate()
	{
		ENGINE_GET_COMPONENT(transform, playerTransform, player.playerEntity);
		GRID_GET_COMPONENT(velocity, playerVelocity, player.playerEntity);
		for (entityClass *e : powerupComponent::component->getComponentEntities()->entities())
		{
			GRID_GET_COMPONENT(powerup, p, e);
			CAGE_ASSERT_RUNTIME(p.type < powerupTypeEnum::Total, p.type);

			ENGINE_GET_COMPONENT(transform, tr, e);
			tr.orientation = p.animation * tr.orientation;
			if (!collisionTest(playerTransform.position, playerScale, playerVelocity.velocity, tr.position, tr.scale, vec3()))
			{
				vec3 toPlayer = playerTransform.position - tr.position;
				real dist = max(toPlayer.length() - playerScale - tr.scale, 1);
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
					wastedPowerup();
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
					wastedPowerup();
			} break;
			default:
				CAGE_THROW_CRITICAL(exception, "invalid powerup mode");
			}
		}
	}

	void engineUpdate()
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

		decoysUpdate();
		turretsUpdate();
		powerupsUpdate();
	}

	class callbacksClass
	{
		eventListener<void()> engineUpdateListener;
	public:
		callbacksClass()
		{
			engineUpdateListener.attach(controlThread().update, -15);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInstance;
}

void powerupSpawn(const vec3 &position)
{
	statistics.powerupsSpawned++;
	entityClass *e = entities()->newUniqueEntity();
	ENGINE_GET_COMPONENT(transform, transform, e);
	transform.position = position * vec3(1, 0, 1);
	transform.orientation = quat(randomAngle(), randomAngle(), randomAngle());
	transform.scale = 2.5;
	GRID_GET_COMPONENT(timeout, ttl, e);
	ttl.ttl = 120 * 30;
	GRID_GET_COMPONENT(powerup, p, e);
	p.animation = quat(randomAngle() / 100, randomAngle() / 100, randomAngle() / 100);
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

void eventBomb()
{
	if (player.powerups[(uint32)powerupTypeEnum::Bomb] == 0)
		return;
	player.powerups[(uint32)powerupTypeEnum::Bomb]--;
	uint32 kills = 0;
	uint32 count = monsterComponent::component->getComponentEntities()->entitiesCount();
	for (entityClass *e : monsterComponent::component->getComponentEntities()->entities())
	{
		GRID_GET_COMPONENT(monster, m, e);
		m.life -= 10;
		if (m.life <= 1e-5)
		{
			kills++;
			e->addGroup(entitiesToDestroy);
			monsterExplosion(e);
			player.score += clamp(numeric_cast<uint32>(m.damage), 1u, 200u);
		}
	}
	statistics.bombsHitTotal += count;
	statistics.bombsKillTotal += kills;
	statistics.bombsHitMax = max(statistics.bombsHitMax, count);
	statistics.bombsKillMax = max(statistics.bombsKillMax, kills);
	statistics.bombsUsed++;
	{
		vec3 playerPosition;
		{
			ENGINE_GET_COMPONENT(transform, p, player.playerEntity);
			playerPosition = p.position;
		}
		for (entityClass *e : gridComponent::component->getComponentEntities()->entities())
		{
			ENGINE_GET_COMPONENT(transform, t, e);
			t.position = playerPosition + randomDirection3() * vec3(100, 1, 100);
		}
	}
	monstersSpawnInitial();

	uint32 sounds[] = {
		hashString("grid/speech/use/bomb-them-all.wav"),
		hashString("grid/speech/use/burn-them-all.wav"),
		hashString("grid/speech/use/let-them-burn.wav"),
		0 };
	soundSpeech(sounds);
}

void eventTurret()
{
	if (player.powerups[(uint32)powerupTypeEnum::Turret] == 0)
		return;
	player.powerups[(uint32)powerupTypeEnum::Turret]--;
	statistics.turretsPlaced++;
	entityClass *turret = entities()->newUniqueEntity();
	ENGINE_GET_COMPONENT(transform, playerTransform, player.playerEntity);
	ENGINE_GET_COMPONENT(transform, transform, turret);
	transform.position = playerTransform.position;
	transform.position[1] = 0;
	transform.orientation = quat(degs(), randomAngle(), degs());
	transform.scale = 3;
	ENGINE_GET_COMPONENT(render, render, turret);
	render.object = hashString("grid/player/turret.object");
	GRID_GET_COMPONENT(turret, tr, turret);
	tr.shooting = 2;
	GRID_GET_COMPONENT(timeout, ttl, turret);
	ttl.ttl = 60 * 30;

	uint32 sounds[] = {
		hashString("grid/speech/use/engaging-a-turret.wav"),
		hashString("grid/speech/use/turret-engaged.wav"),
		0 };
	soundSpeech(sounds);
}

void eventDecoy()
{
	if (player.powerups[(uint32)powerupTypeEnum::Decoy] == 0)
		return;
	player.powerups[(uint32)powerupTypeEnum::Decoy]--;
	statistics.decoysUsed++;
	entityClass *decoy = entities()->newUniqueEntity();
	ENGINE_GET_COMPONENT(transform, playerTransform, player.playerEntity);
	ENGINE_GET_COMPONENT(transform, transform, decoy);
	transform = playerTransform;
	transform.scale *= 2;
	ENGINE_GET_COMPONENT(render, render, decoy);
	render.object = hashString("grid/player/player.object");
	GRID_GET_COMPONENT(velocity, playerVelocity, player.playerEntity);
	GRID_GET_COMPONENT(velocity, vel, decoy);
	vel.velocity = -playerVelocity.velocity;
	GRID_GET_COMPONENT(timeout, ttl, decoy);
	ttl.ttl = 60 * 30;
	GRID_GET_COMPONENT(decoy, dec, decoy);
	dec.rotation = interpolate(quat(), quat(randomAngle(), randomAngle(), randomAngle()), 3e-3);

	uint32 sounds[] = {
		hashString("grid/speech/use/decoy-launched.wav"),
		hashString("grid/speech/use/launching-a-decoy.wav"),
		0 };
	soundSpeech(sounds);
}
