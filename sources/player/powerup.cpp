#include <cage-core/entities.h>
#include <cage-core/entitiesVisitor.h>
#include <cage-core/config.h>
#include <cage-core/color.h>
#include <cage-core/hashString.h>

#include "../game.h"

namespace
{
	struct TurretComponent
	{
		uint32 shooting = 0;
	};

	struct DecoyComponent
	{};

	void turretsUpdate()
	{
		entitiesVisitor([&](const TransformComponent &tr, TurretComponent &tu) {
			if (tu.shooting > 0)
			{
				tu.shooting--;
				return;
			}
			tu.shooting = 10;
			for (uint32 i = 0; i < 6; i++)
			{
				statistics.shotsTurret++;
				Entity *shot = engineEntities()->createUnique();
				TransformComponent &transform = shot->value<TransformComponent>();
				transform.orientation = Quat(Degs(), Degs(i * 60), Degs()) * tr.orientation;
				transform.position = tr.position + transform.orientation * Vec3(0, 0, -1) * 2;
				RenderComponent &render = shot->value<RenderComponent>();
				render.object = HashString("degrid/player/shot.object");
				render.color = game.shotsColor;
				shot->value<VelocityComponent>().velocity = transform.orientation * Vec3(0, 0, -1) * 2.5;
				shot->value<ShotComponent>().damage = 1;
				shot->value<TimeoutComponent>().ttl = ShotsTtl;
			}
		}, engineEntities(), false);
	}

	void decoysUpdate()
	{
		entitiesVisitor([&](TransformComponent &tr, VelocityComponent &vel, const DecoyComponent &) {
			tr.position[1] = 0;
			vel.velocity *= 0.97;
			game.monstersTarget = tr.position;
		}, engineEntities(), false);
	}

	void wastedPowerup()
	{
		statistics.powerupsWasted++;
		game.money += PowerupSellPriceBase * (game.defeatedBosses + 1);
		soundSpeech(HashString("degrid/speech/pickup/sold.wav"));
	}

	void powerupsUpdate()
	{
		const TransformComponent &playerTransform = game.playerEntity->value<TransformComponent>();
		const VelocityComponent &playerVelocity = game.playerEntity->value<VelocityComponent>();
		entitiesVisitor([&](Entity *e, TransformComponent &tr, const PowerupComponent &p) {
			CAGE_ASSERT(p.type < PowerupTypeEnum::Total);

			if (!collisionTest(playerTransform.position, PlayerScale, playerVelocity.velocity, tr.position, tr.scale, Vec3()))
			{
				const Vec3 toPlayer = playerTransform.position - tr.position;
				const Real dist = max(length(toPlayer) - PlayerScale - tr.scale, 1);
				tr.position += normalize(toPlayer) * (2 / dist);
				return;
			}

			game.score += 20;
			statistics.powerupsPicked++;
			e->add(entitiesToDestroy);
			switch (PowerupMode[(uint32)p.type])
			{
			case 0: // collectible
				if (game.powerups[(uint32)p.type] < 3)
				{
					game.powerups[(uint32)p.type]++;
					switch (p.type)
					{
					case PowerupTypeEnum::Bomb: soundSpeech(HashString("degrid/speech/pickup/a-bomb.wav")); break;
					case PowerupTypeEnum::Decoy: soundSpeech(HashString("degrid/speech/pickup/a-decoy.wav")); break;
					case PowerupTypeEnum::Turret: soundSpeech(HashString("degrid/speech/pickup/a-turret.wav")); break;
					default: CAGE_THROW_CRITICAL(Exception, "invalid powerup type");
					}
					{ // achievement
						bool ok = true;
						for (uint32 i = 0; i < (uint32)PowerupTypeEnum::Total; i++)
						{
							if (PowerupMode[i] == 0 && game.powerups[i] < 3)
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
				const uint32 duration = 30 * (30 + 15 * game.powerups[(uint32)PowerupTypeEnum::Duration]);
				game.powerups[(uint32)p.type] += duration;
				switch (p.type)
				{
				case PowerupTypeEnum::Shield: soundSpeech(HashString("degrid/speech/pickup/shield-engaged.wav")); break;
				case PowerupTypeEnum::HomingShots: soundSpeech(HashString("degrid/speech/pickup/homing-missiles.wav")); break;
				case PowerupTypeEnum::SuperDamage: soundSpeech(HashString("degrid/speech/pickup/super-damage.wav")); break;
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
					case PowerupTypeEnum::Acceleration: soundSpeech(HashString("degrid/speech/pickup/acceleration-improved.wav")); break;
					case PowerupTypeEnum::MaxSpeed: soundSpeech(HashString("degrid/speech/pickup/movement-speed-improved.wav")); break;
					case PowerupTypeEnum::Multishot: soundSpeech(HashString("degrid/speech/pickup/additional-cannon.wav")); break;
					case PowerupTypeEnum::ShotsSpeed: soundSpeech(HashString("degrid/speech/pickup/missiles-speed-improved.wav")); break;
					case PowerupTypeEnum::ShotsDamage: soundSpeech(HashString("degrid/speech/pickup/missiles-damage-improved.wav")); break;
					case PowerupTypeEnum::FiringSpeed: soundSpeech(HashString("degrid/speech/pickup/firing-speed-improved.wav")); break;
					case PowerupTypeEnum::Armor: soundSpeech(HashString("degrid/speech/pickup/improved-armor.wav")); break;
					case PowerupTypeEnum::Duration: soundSpeech(HashString("degrid/speech/pickup/increased-powerup-duration.wav")); break;
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
		}, engineEntities(), false);
	}

	void engineInit()
	{
		engineEntities()->defineComponent(TurretComponent());
		engineEntities()->defineComponent(DecoyComponent());
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		{ // decrease timed power ups
			for (uint32 i = 0; i < (uint32)PowerupTypeEnum::Total; i++)
			{
				if (PowerupMode[i] == 1 && game.powerups[i] > 0)
					game.powerups[i]--;
			}
		}

		decoysUpdate();
		turretsUpdate();
		powerupsUpdate();
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener;
	public:
		Callbacks()
		{
			engineInitListener.attach(controlThread().initialize, -15);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener.attach(controlThread().update, -15);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInstance;

	PowerupTypeEnum powerupSpawnType()
	{
		Real p = randomChance();
		uint32 c = 0;
		while (true)
		{
			CAGE_ASSERT(c < sizeof(PowerupChances) / sizeof(PowerupChances[0]));
			p -= PowerupChances[c];
			if (p < 0)
				return (PowerupTypeEnum)c;
			c++;
		}
	}
}

void powerupSpawn(const Vec3 &position)
{
	if (game.cinematic)
		return;

	PowerupTypeEnum type = powerupSpawnType();
	if (statistics.powerupsSpawned == 0)
	{
		while (PowerupMode[(uint32)type] != 2)
			type = powerupSpawnType();
	}
	const bool coin = type == PowerupTypeEnum::Coin;
	if (coin)
		statistics.coinsSpawned++;
	else
		statistics.powerupsSpawned++;

	Entity *e = engineEntities()->createUnique();
	TransformComponent &transform = e->value<TransformComponent>();
	transform.position = position * Vec3(1, 0, 1);
	transform.orientation = randomDirectionQuat();
	transform.scale = coin ? 2.0 : 2.5;
	e->value<TimeoutComponent>().ttl = 120 * 30;
	PowerupComponent &p = e->value<PowerupComponent>();
	p.type = type;
	e->value<RotationComponent>().rotation = interpolate(Quat(), randomDirectionQuat(), 0.01);
	e->value<VelocityComponent>();
	static constexpr const uint32 ObjectName[4] = {
		HashString("degrid/player/powerupCollectible.object"),
		HashString("degrid/player/powerupOnetime.object"),
		HashString("degrid/player/powerupPermanent.object"),
		HashString("degrid/player/coin.object")
	};
	RenderComponent &render = e->value<RenderComponent>();
	render.object = ObjectName[PowerupMode[(uint32)p.type]];
	render.color = colorHsvToRgb(Vec3(randomChance(), 1, 1));
	soundEffect(coin ? HashString("degrid/player/coin.ogg") : HashString("degrid/player/powerup.ogg"), transform.position);
}

void eventBomb()
{
	if (game.powerups[(uint32)PowerupTypeEnum::Bomb] == 0)
		return;

	game.powerups[(uint32)PowerupTypeEnum::Bomb]--;
	statistics.bombsUsed++;

	{
		const uint32 count = engineEntities()->component<MonsterComponent>()->count();
		statistics.bombsHitTotal += count;
		statistics.bombsHitMax = max(statistics.bombsHitMax, count);
	}

	uint32 kills = 0;
	entitiesVisitor([&](Entity *e, MonsterComponent &m) {
		m.life -= 10;
		if (m.life <= 1e-5)
		{
			kills++;
			killMonster(e, false);
		}
	}, engineEntities(), false);
	statistics.bombsKillTotal += kills;
	statistics.bombsKillMax = max(statistics.bombsKillMax, kills);

	{
		const TransformComponent &playerTransform = game.playerEntity->value<TransformComponent>();
		entitiesVisitor([&](TransformComponent &t, const GridComponent &) {
			t.position = playerTransform.position + randomDirection3() * Vec3(100, 1, 100);
		}, engineEntities(), false);
	}

	if (engineEntities()->component<BossComponent>()->count() == 0)
		monstersSpawnInitial();

	static constexpr const uint32 Sounds[] = {
		HashString("degrid/speech/use/bomb-them-all.wav"),
		HashString("degrid/speech/use/burn-them-all.wav"),
		HashString("degrid/speech/use/let-them-burn.wav"),
		0 };
	soundSpeech(Sounds);

	if (kills == 0)
		achievementFullfilled("wasted");
}

void eventTurret()
{
	if (game.powerups[(uint32)PowerupTypeEnum::Turret] == 0)
		return;
	game.powerups[(uint32)PowerupTypeEnum::Turret]--;
	statistics.turretsPlaced++;
	Entity *turret = engineEntities()->createUnique();
	TransformComponent &transform = turret->value<TransformComponent>();
	transform.position = game.playerEntity->value<TransformComponent>().position;
	transform.position[1] = 0;
	transform.orientation = Quat(Degs(), randomAngle(), Degs());
	transform.scale = 3;
	turret->value<VelocityComponent>();
	turret->value<RenderComponent>().object = HashString("degrid/player/turret.object");
	turret->value<TurretComponent>().shooting = 2;
	turret->value<TimeoutComponent>().ttl = 60 * 30;
	turret->value<RotationComponent>().rotation = Quat(Degs(), Degs(1), Degs());

	static constexpr const uint32 Sounds[] = {
		HashString("degrid/speech/use/engaging-a-turret.wav"),
		HashString("degrid/speech/use/turret-engaged.wav"),
		0 };
	soundSpeech(Sounds);

	if (engineEntities()->component<TurretComponent>()->count() >= 4)
		achievementFullfilled("turrets");
}

void eventDecoy()
{
	if (game.powerups[(uint32)PowerupTypeEnum::Decoy] == 0)
		return;
	game.powerups[(uint32)PowerupTypeEnum::Decoy]--;
	statistics.decoysUsed++;
	Entity *decoy = engineEntities()->createUnique();
	TransformComponent &transform = decoy->value<TransformComponent>();
	transform = game.playerEntity->value<TransformComponent>();
	transform.scale *= 2;
	decoy->value<RenderComponent>().object = HashString("degrid/player/player.object");
	decoy->value<VelocityComponent>().velocity = -game.playerEntity->value<VelocityComponent>().velocity;
	decoy->value<TimeoutComponent>().ttl = 60 * 30;
	decoy->value<DecoyComponent>();
	decoy->value<RotationComponent>().rotation = interpolate(Quat(), Quat(randomAngle(), randomAngle(), randomAngle()), 3e-3);

	static constexpr const uint32 Sounds[] = {
		HashString("degrid/speech/use/decoy-launched.wav"),
		HashString("degrid/speech/use/launching-a-decoy.wav"),
		0 };
	soundSpeech(Sounds);
}

uint32 permanentPowerupLimit()
{
	return achievements.bosses + 5;
}

uint32 currentPermanentPowerups()
{
	uint32 sum = 0;
	for (uint32 i = (uint32)PowerupTypeEnum::MaxSpeed; i < (uint32)PowerupTypeEnum::Coin; i++)
		sum += game.powerups[i];
	return sum;
}

bool canAddPermanentPowerup()
{
	return currentPermanentPowerups() < permanentPowerupLimit();
}
