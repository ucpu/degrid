#include "../game.h"

#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/color.h>
#include <cage-core/HashString.h>

namespace
{
	struct turretComponent
	{
		static EntityComponent *component;
		uint32 shooting;
		turretComponent() : shooting(0) {}
	};

	struct decoyComponent
	{
		static EntityComponent *component;
	};

	EntityComponent *turretComponent::component;
	EntityComponent *decoyComponent::component;

	void turretsUpdate()
	{
		for (Entity *e : turretComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(Transform, tr, e);
			DEGRID_COMPONENT(turret, tu, e);
			if (tu.shooting > 0)
			{
				tu.shooting--;
				continue;
			}
			tu.shooting = 10;
			for (uint32 i = 0; i < 6; i++)
			{
				statistics.shotsTurret++;
				Entity *shot = entities()->createUnique();
				CAGE_COMPONENT_ENGINE(Transform, transform, shot);
				transform.orientation = quat(degs(), degs(i * 60), degs()) * tr.orientation;
				transform.position = tr.position + transform.orientation * vec3(0, 0, -1) * 2;
				CAGE_COMPONENT_ENGINE(Render, render, shot);
				render.object = HashString("degrid/player/shot.object");
				render.color = game.shotsColor;
				DEGRID_COMPONENT(velocity, vel, shot);
				vel.velocity = transform.orientation * vec3(0, 0, -1) * 2.5;
				DEGRID_COMPONENT(shot, sh, shot);
				sh.damage = 1;
				DEGRID_COMPONENT(timeout, ttl, shot);
				ttl.ttl = shotsTtl;
			}
		}
	}

	void decoysUpdate()
	{
		for (Entity *e : decoyComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(Transform, tr, e);
			DEGRID_COMPONENT(velocity, vel, e);
			tr.position[1] = 0;
			vel.velocity *= 0.97;
			game.monstersTarget = tr.position;
		}
	}

	void wastedPowerup()
	{
		statistics.powerupsWasted++;
		game.money += powerupSellPrice;
		soundSpeech(HashString("degrid/speech/pickup/sold.wav"));
	}

	void powerupsUpdate()
	{
		CAGE_COMPONENT_ENGINE(Transform, playerTransform, game.playerEntity);
		DEGRID_COMPONENT(velocity, playerVelocity, game.playerEntity);
		for (Entity *e : powerupComponent::component->entities())
		{
			DEGRID_COMPONENT(powerup, p, e);
			CAGE_ASSERT(p.type < powerupTypeEnum::Total, p.type);

			CAGE_COMPONENT_ENGINE(Transform, tr, e);
			if (!collisionTest(playerTransform.position, playerScale, playerVelocity.velocity, tr.position, tr.scale, vec3()))
			{
				vec3 toPlayer = playerTransform.position - tr.position;
				real dist = max(length(toPlayer) - playerScale - tr.scale, 1);
				tr.position += normalize(toPlayer) * (2 / dist);
				continue;
			}

			game.score += 20;
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
					case powerupTypeEnum::Bomb: soundSpeech(HashString("degrid/speech/pickup/a-bomb.wav")); break;
					case powerupTypeEnum::Decoy: soundSpeech(HashString("degrid/speech/pickup/a-decoy.wav")); break;
					case powerupTypeEnum::Turret: soundSpeech(HashString("degrid/speech/pickup/a-turret.wav")); break;
					default: CAGE_THROW_CRITICAL(Exception, "invalid powerup type");
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
				uint32 duration = 30 * (30 + 15 * game.powerups[(uint32)powerupTypeEnum::Duration]);
				game.powerups[(uint32)p.type] += duration;
				switch (p.type)
				{
				case powerupTypeEnum::Shield: soundSpeech(HashString("degrid/speech/pickup/shield-engaged.wav")); break;
				case powerupTypeEnum::HomingShots: soundSpeech(HashString("degrid/speech/pickup/homing-missiles.wav")); break;
				case powerupTypeEnum::SuperDamage: soundSpeech(HashString("degrid/speech/pickup/super-damage.wav")); break;
				default: CAGE_THROW_CRITICAL(Exception, "invalid powerup type");
				}
				if (game.powerups[(uint32)p.type] > 2 * duration)
					achievementFullfilled("timelord");
			} break;
			case 2: // permanent
			{
				if (canAddPermanentPowerup())
				{
					game.powerups[(uint32)p.type]++;
					switch (p.type)
					{
					case powerupTypeEnum::Acceleration: soundSpeech(HashString("degrid/speech/pickup/acceleration-improved.wav")); break;
					case powerupTypeEnum::MaxSpeed: soundSpeech(HashString("degrid/speech/pickup/movement-speed-improved.wav")); break;
					case powerupTypeEnum::Multishot: soundSpeech(HashString("degrid/speech/pickup/additional-cannon.wav")); break;
					case powerupTypeEnum::ShotsSpeed: soundSpeech(HashString("degrid/speech/pickup/missiles-speed-improved.wav")); break;
					case powerupTypeEnum::ShotsDamage: soundSpeech(HashString("degrid/speech/pickup/missiles-damage-improved.wav")); break;
					case powerupTypeEnum::FiringSpeed: soundSpeech(HashString("degrid/speech/pickup/firing-speed-improved.wav")); break;
					case powerupTypeEnum::Armor: soundSpeech(HashString("degrid/speech/pickup/improved-armor.wav")); break;
					case powerupTypeEnum::Duration: soundSpeech(HashString("degrid/speech/pickup/increased-powerup-duration.wav")); break;
					default: CAGE_THROW_CRITICAL(Exception, "invalid powerup type");
					}
				}
				else
					wastedPowerup();
			} break;
			case 3: // coin
			{
				game.powerups[(uint32)p.type]++; // count the coins (for statistics & achievement)
				game.money += game.defeatedBosses + 1;
				soundSpeech(HashString("degrid/speech/pickup/a-coin.wav"));
				if (game.powerups[(uint32)p.type] == 1000)
					achievementFullfilled("vault-digger");
			} break;
			default:
				CAGE_THROW_CRITICAL(Exception, "invalid powerup mode");
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
		OPTICK_EVENT("powerups");

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
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener;
	public:
		callbacksClass() : engineInitListener("powerup"), engineUpdateListener("powerup")
		{
			engineInitListener.attach(controlThread().initialize, -15);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener.attach(controlThread().update, -15);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInstance;

	powerupTypeEnum powerupSpawnType()
	{
		real p = randomChance();
		uint32 c = 0;
		while (true)
		{
			CAGE_ASSERT(c < sizeof(powerupChances) / sizeof(powerupChances[0]));
			p -= powerupChances[c];
			if (p < 0)
				return (powerupTypeEnum)c;
			c++;
		}
	}
}

void powerupSpawn(const vec3 &position)
{
	if (game.cinematic)
		return;

	powerupTypeEnum type = powerupSpawnType();
	if (statistics.powerupsSpawned == 0)
	{
		while (powerupMode[(uint32)type] != 2)
			type = powerupSpawnType();
	}
	bool coin = type == powerupTypeEnum::Coin;
	if (coin)
		statistics.coinsSpawned++;
	else
		statistics.powerupsSpawned++;

	Entity *e = entities()->createUnique();
	CAGE_COMPONENT_ENGINE(Transform, transform, e);
	transform.position = position * vec3(1, 0, 1);
	transform.orientation = randomDirectionQuat();
	transform.scale = coin ? 1.5 : 2.5;
	DEGRID_COMPONENT(timeout, ttl, e);
	ttl.ttl = 120 * 30;
	DEGRID_COMPONENT(powerup, p, e);
	p.type = type;
	DEGRID_COMPONENT(rotation, rot, e);
	rot.rotation = interpolate(quat(), randomDirectionQuat(), 0.01);
	DEGRID_COMPONENT(velocity, velocity, e); // to make the powerup affected by gravity
	CAGE_COMPONENT_ENGINE(Render, render, e);
	static const uint32 objectName[4] = {
		HashString("degrid/player/powerupCollectible.object"),
		HashString("degrid/player/powerupOnetime.object"),
		HashString("degrid/player/powerupPermanent.object"),
		HashString("degrid/player/coin.object")
	};
	render.object = objectName[powerupMode[(uint32)p.type]];
	render.color = colorHsvToRgb(vec3(randomChance(), 1, 1));
	soundEffect(coin ? HashString("degrid/player/coin.ogg") : HashString("degrid/player/powerup.ogg"), transform.position);
}

void eventBomb()
{
	if (game.powerups[(uint32)powerupTypeEnum::Bomb] == 0)
		return;

	game.powerups[(uint32)powerupTypeEnum::Bomb]--;
	uint32 kills = 0;
	uint32 count = monsterComponent::component->group()->count();
	for (Entity *e : monsterComponent::component->entities())
	{
		DEGRID_COMPONENT(monster, m, e);
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
		CAGE_COMPONENT_ENGINE(Transform, playerTransform, game.playerEntity);
		for (Entity *e : gridComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(Transform, t, e);
			t.position = playerTransform.position + randomDirection3() * vec3(100, 1, 100);
		}
	}

	if (bossComponent::component->group()->count() == 0)
		monstersSpawnInitial();

	uint32 sounds[] = {
		HashString("degrid/speech/use/bomb-them-all.wav"),
		HashString("degrid/speech/use/burn-them-all.wav"),
		HashString("degrid/speech/use/let-them-burn.wav"),
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
	Entity *turret = entities()->createUnique();
	CAGE_COMPONENT_ENGINE(Transform, playerTransform, game.playerEntity);
	CAGE_COMPONENT_ENGINE(Transform, transform, turret);
	DEGRID_COMPONENT(velocity, vel, turret); // allow it to be affected by gravity
	transform.position = playerTransform.position;
	transform.position[1] = 0;
	transform.orientation = quat(degs(), randomAngle(), degs());
	transform.scale = 3;
	CAGE_COMPONENT_ENGINE(Render, render, turret);
	render.object = HashString("degrid/player/turret.object");
	DEGRID_COMPONENT(turret, tr, turret);
	tr.shooting = 2;
	DEGRID_COMPONENT(timeout, ttl, turret);
	ttl.ttl = 60 * 30;
	DEGRID_COMPONENT(rotation, rot, turret);
	rot.rotation = quat(degs(), degs(1), degs());

	uint32 sounds[] = {
		HashString("degrid/speech/use/engaging-a-turret.wav"),
		HashString("degrid/speech/use/turret-engaged.wav"),
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
	Entity *decoy = entities()->createUnique();
	CAGE_COMPONENT_ENGINE(Transform, playerTransform, game.playerEntity);
	CAGE_COMPONENT_ENGINE(Transform, transform, decoy);
	transform = playerTransform;
	transform.scale *= 2;
	CAGE_COMPONENT_ENGINE(Render, render, decoy);
	render.object = HashString("degrid/player/player.object");
	DEGRID_COMPONENT(velocity, playerVelocity, game.playerEntity);
	DEGRID_COMPONENT(velocity, vel, decoy);
	vel.velocity = -playerVelocity.velocity;
	DEGRID_COMPONENT(timeout, ttl, decoy);
	ttl.ttl = 60 * 30;
	DEGRID_COMPONENT(decoy, dec, decoy);
	DEGRID_COMPONENT(rotation, rot, decoy);
	rot.rotation = interpolate(quat(), quat(randomAngle(), randomAngle(), randomAngle()), 3e-3);

	uint32 sounds[] = {
		HashString("degrid/speech/use/decoy-launched.wav"),
		HashString("degrid/speech/use/launching-a-decoy.wav"),
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
