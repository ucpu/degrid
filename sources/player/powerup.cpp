#include "../game.h"

#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/color.h>
#include <cage-core/hashString.h>

namespace
{
	struct turretComponent
	{
		static componentClass *component;
		uint32 shooting;
		turretComponent() : shooting(0) {}
	};

	struct decoyComponent
	{
		static componentClass *component;
	};

	componentClass *turretComponent::component;
	componentClass *decoyComponent::component;

	void turretsUpdate()
	{
		for (entityClass *e : turretComponent::component->entities())
		{
			ENGINE_GET_COMPONENT(transform, tr, e);
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
				entityClass *shot = entities()->createUnique();
				ENGINE_GET_COMPONENT(transform, transform, shot);
				transform.orientation = quat(degs(), degs(i * 60), degs()) * tr.orientation;
				transform.position = tr.position + transform.orientation * vec3(0, 0, -1) * 2;
				ENGINE_GET_COMPONENT(render, render, shot);
				render.object = hashString("grid/player/shot.object");
				render.color = game.shotsColor;
				GRID_GET_COMPONENT(velocity, vel, shot);
				vel.velocity = transform.orientation * vec3(0, 0, -1) * 2.5;
				GRID_GET_COMPONENT(shot, sh, shot);
				sh.damage = 1;
				GRID_GET_COMPONENT(timeout, ttl, shot);
				ttl.ttl = shotsTtl;
			}
		}
	}

	void decoysUpdate()
	{
		for (entityClass *e : decoyComponent::component->entities())
		{
			ENGINE_GET_COMPONENT(transform, tr, e);
			GRID_GET_COMPONENT(velocity, vel, e);
			tr.position[1] = 0;
			vel.velocity *= 0.97;
			game.monstersTarget = tr.position;
		}
	}

	void wastedPowerup()
	{
		statistics.powerupsWasted++;
		game.money += 10;
		soundSpeech(hashString("grid/speech/pickup/sold.wav"));
	}

	void powerupsUpdate()
	{
		ENGINE_GET_COMPONENT(transform, playerTransform, game.playerEntity);
		GRID_GET_COMPONENT(velocity, playerVelocity, game.playerEntity);
		for (entityClass *e : powerupComponent::component->entities())
		{
			GRID_GET_COMPONENT(powerup, p, e);
			CAGE_ASSERT_RUNTIME(p.type < powerupTypeEnum::Total, p.type);

			ENGINE_GET_COMPONENT(transform, tr, e);
			if (!collisionTest(playerTransform.position, playerScale, playerVelocity.velocity, tr.position, tr.scale, vec3()))
			{
				vec3 toPlayer = playerTransform.position - tr.position;
				real dist = max(toPlayer.length() - playerScale - tr.scale, 1);
				tr.position += normalize(toPlayer) * (2 / dist);
				continue;
			}

			game.score += 10;
			statistics.powerupsPicked++;
			e->add(entitiesToDestroy);
			switch (powerupMode[(uint32)p.type])
			{
			case 0: // collectible
				if (game.powerups[(uint32)p.type] < 3)
				{
					game.powerups[(uint32)p.type]++;
					switch (p.type)
					{
					case powerupTypeEnum::Bomb: soundSpeech(hashString("grid/speech/pickup/a-bomb.wav")); break;
					case powerupTypeEnum::Decoy: soundSpeech(hashString("grid/speech/pickup/a-decoy.wav")); break;
					case powerupTypeEnum::Turret: soundSpeech(hashString("grid/speech/pickup/a-turret.wav")); break;
					default: CAGE_THROW_CRITICAL(exception, "invalid powerup type");
					}
					{ // achievement
						bool ok = true;
						for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
						{
							if (powerupMode[i] == 0 && game.powerups[i] < 3)
								ok = false;
						}
						if (ok)
							achievementFullfilled("collector");
					}
				}
				else
					wastedPowerup();
				break;
			case 1: // timed
			{
				game.powerups[(uint32)p.type] += 30 * 30;
				switch (p.type)
				{
				case powerupTypeEnum::Shield: soundSpeech(hashString("grid/speech/pickup/shield-engaged.wav")); break;
				case powerupTypeEnum::HomingShots: soundSpeech(hashString("grid/speech/pickup/homing-missiles.wav")); break;
				case powerupTypeEnum::SuperDamage: soundSpeech(hashString("grid/speech/pickup/super-damage.wav")); break;
				default: CAGE_THROW_CRITICAL(exception, "invalid powerup type");
				}
				if (game.powerups[(uint32)p.type] > 2 * 30 * 30)
					achievementFullfilled("timelord");
			} break;
			case 2: // permanent
			{
				if (canAddPermanentPowerup())
				{
					game.powerups[(uint32)p.type]++;
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
			case 3: // coin
			{
				game.powerups[(uint32)p.type]++; // count the coins (for statistics & achievement)
				game.money += 1;
				if (randomChance() > powerupIsCoin)
					soundSpeech(hashString("grid/speech/pickup/a-coin.wav"));
				if (game.powerups[(uint32)p.type] == 1000)
					achievementFullfilled("vault-digger");
			} break;
			default:
				CAGE_THROW_CRITICAL(exception, "invalid powerup mode");
			}
		}
	}

	void engineInit()
	{
		turretComponent::component = entities()->defineComponent(turretComponent(), true);
		decoyComponent::component = entities()->defineComponent(decoyComponent(), true);
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		{ // decrease timed power ups
			for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
			{
				if (powerupMode[i] == 1 && game.powerups[i] > 0)
					game.powerups[i]--;
			}
		}

		decoysUpdate();
		turretsUpdate();
		powerupsUpdate();
	}

	class callbacksClass
	{
		eventListener<void()> engineInitListener;
		eventListener<void()> engineUpdateListener;
	public:
		callbacksClass()
		{
			engineInitListener.attach(controlThread().initialize, -15);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener.attach(controlThread().update, -15);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInstance;
}

void powerupSpawn(const vec3 &position)
{
	if (game.cinematic)
		return;

	bool coin = randomChance() < powerupIsCoin;
	if (coin)
		statistics.coinsSpawned++;
	else
		statistics.powerupsSpawned++;

	entityClass *e = entities()->createUnique();
	ENGINE_GET_COMPONENT(transform, transform, e);
	transform.position = position * vec3(1, 0, 1);
	transform.orientation = randomDirectionQuat();
	transform.scale = coin ? 1.5 : 2.5;
	GRID_GET_COMPONENT(timeout, ttl, e);
	ttl.ttl = 120 * 30;
	GRID_GET_COMPONENT(powerup, p, e);
	p.type = coin ? powerupTypeEnum::Coin : (powerupTypeEnum)randomRange(0u, (uint32)powerupTypeEnum::Coin);
	GRID_GET_COMPONENT(rotation, rot, e);
	rot.rotation = interpolate(quat(), randomDirectionQuat(), 0.01);
	GRID_GET_COMPONENT(velocity, velocity, e); // to make the powerup affected by gravity
	ENGINE_GET_COMPONENT(render, render, e);
	static const uint32 objectName[4] = {
		hashString("grid/player/powerupCollectible.object"),
		hashString("grid/player/powerupOnetime.object"),
		hashString("grid/player/powerupPermanent.object"),
		hashString("grid/player/coin.object")
	};
	render.object = objectName[powerupMode[(uint32)p.type]];
	render.color = convertHsvToRgb(vec3(randomChance(), 1, 1));
	soundEffect(coin ? hashString("grid/player/coin.ogg") : hashString("grid/player/powerup.ogg"), transform.position);
}

void eventBomb()
{
	if (game.powerups[(uint32)powerupTypeEnum::Bomb] == 0)
		return;
	game.powerups[(uint32)powerupTypeEnum::Bomb]--;
	uint32 kills = 0;
	uint32 count = monsterComponent::component->group()->count();
	for (entityClass *e : monsterComponent::component->entities())
	{
		GRID_GET_COMPONENT(monster, m, e);
		m.life -= 10;
		if (m.life <= 1e-5)
		{
			kills++;
			killMonster(e, false);
		}
	}
	statistics.bombsHitTotal += count;
	statistics.bombsKillTotal += kills;
	statistics.bombsHitMax = max(statistics.bombsHitMax, count);
	statistics.bombsKillMax = max(statistics.bombsKillMax, kills);
	statistics.bombsUsed++;
	{
		ENGINE_GET_COMPONENT(transform, playerTransform, game.playerEntity);
		for (entityClass *e : gridComponent::component->entities())
		{
			ENGINE_GET_COMPONENT(transform, t, e);
			t.position = playerTransform.position + randomDirection3() * vec3(100, 1, 100);
		}
	}
	monstersSpawnInitial();

	uint32 sounds[] = {
		hashString("grid/speech/use/bomb-them-all.wav"),
		hashString("grid/speech/use/burn-them-all.wav"),
		hashString("grid/speech/use/let-them-burn.wav"),
		0 };
	soundSpeech(sounds);

	if (kills == 0)
		achievementFullfilled("wasted");
}

void eventTurret()
{
	if (game.powerups[(uint32)powerupTypeEnum::Turret] == 0)
		return;
	game.powerups[(uint32)powerupTypeEnum::Turret]--;
	statistics.turretsPlaced++;
	entityClass *turret = entities()->createUnique();
	ENGINE_GET_COMPONENT(transform, playerTransform, game.playerEntity);
	ENGINE_GET_COMPONENT(transform, transform, turret);
	GRID_GET_COMPONENT(velocity, vel, turret); // allow it to be affected by gravity
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
	GRID_GET_COMPONENT(rotation, rot, turret);
	rot.rotation = quat(degs(), degs(1), degs());

	uint32 sounds[] = {
		hashString("grid/speech/use/engaging-a-turret.wav"),
		hashString("grid/speech/use/turret-engaged.wav"),
		0 };
	soundSpeech(sounds);

	if (turretComponent::component->group()->count() >= 4)
		achievementFullfilled("turrets");
}

void eventDecoy()
{
	if (game.powerups[(uint32)powerupTypeEnum::Decoy] == 0)
		return;
	game.powerups[(uint32)powerupTypeEnum::Decoy]--;
	statistics.decoysUsed++;
	entityClass *decoy = entities()->createUnique();
	ENGINE_GET_COMPONENT(transform, playerTransform, game.playerEntity);
	ENGINE_GET_COMPONENT(transform, transform, decoy);
	transform = playerTransform;
	transform.scale *= 2;
	ENGINE_GET_COMPONENT(render, render, decoy);
	render.object = hashString("grid/player/player.object");
	GRID_GET_COMPONENT(velocity, playerVelocity, game.playerEntity);
	GRID_GET_COMPONENT(velocity, vel, decoy);
	vel.velocity = -playerVelocity.velocity;
	GRID_GET_COMPONENT(timeout, ttl, decoy);
	ttl.ttl = 60 * 30;
	GRID_GET_COMPONENT(decoy, dec, decoy);
	GRID_GET_COMPONENT(rotation, rot, decoy);
	rot.rotation = interpolate(quat(), quat(randomAngle(), randomAngle(), randomAngle()), 3e-3);

	uint32 sounds[] = {
		hashString("grid/speech/use/decoy-launched.wav"),
		hashString("grid/speech/use/launching-a-decoy.wav"),
		0 };
	soundSpeech(sounds);
}

uint32 permanentPowerupLimit()
{
	return achievements.bosses + 5;
}

uint32 currentPermanentPowerups()
{
	uint32 sum = 0;
	for (uint32 i = (uint32)powerupTypeEnum::MaxSpeed; i < (uint32)powerupTypeEnum::Coin; i++)
		sum += game.powerups[i];
	return sum;
}

bool canAddPermanentPowerup()
{
	return currentPermanentPowerups() < permanentPowerupLimit();
}