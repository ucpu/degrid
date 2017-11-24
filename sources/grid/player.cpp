#include <set>

#include "includes.h"
#include "game.h"

namespace grid
{
	namespace
	{
		bool keyMap[512];
		mouseButtonsFlags buttonMap;

		void setMousePosition()
		{
			if (!window()->isFocused())
				return;
			pointStruct point = window()->mousePosition();
			pointStruct res = window()->resolution();
			vec2 p = vec2(point.x, point.y);
			p /= vec2(res.x, res.y);
			p = p * 2 - 1;
			real px = p[0], py = -p[1];
			ENGINE_GET_COMPONENT(transform, ts, player.primaryCameraEntity);
			ENGINE_GET_COMPONENT(camera, cs, player.primaryCameraEntity);
			mat4 view = mat4(ts.position, ts.orientation, vec3(ts.scale, ts.scale, ts.scale)).inverse();
			mat4 proj = perspectiveProjection(cs.perspectiveFov, real(res.x) / real(res.y), cs.near, cs.far);
			mat4 inv = (proj * view).inverse();
			vec4 pn = inv * vec4(px, py, -1, 1);
			vec4 pf = inv * vec4(px, py, 1, 1);
			vec3 near = vec3(pn) / pn[3];
			vec3 far = vec3(pf) / pf[3];
			player.mouseCurrentPosition = (near + far) * 0.5;
		}

		void eventBomb()
		{
			if (player.powerups[puBomb] == 0)
				return;
			player.powerups[puBomb]--;
			uint32 kills = 0;
			uint32 count = monsterStruct::component->getComponentEntities()->entitiesCount();
			entityClass *const *monsters = monsterStruct::component->getComponentEntities()->entitiesArray();
			for (uint32 i = 0; i < count; i++)
			{
				entityClass *e = monsters[i];
				GRID_GET_COMPONENT(monster, m, e);
				m.life -= 10;
				if (m.life <= 1e-5)
				{
					kills++;
					e->addGroup(entitiesToDestroy);
					monsterExplosion(e);
				}
			}
			player.score += kills;
			statistics.bombsHitTotal += count;
			statistics.bombsKillTotal += kills;
			statistics.bombsHitMax = max(statistics.bombsHitMax, count);
			statistics.bombsKillMax = max(statistics.bombsKillMax, kills);
			statistics.bombsUsed++;
			{
				uint32 count = gridStruct::component->getComponentEntities()->entitiesCount();
				entityClass *const *grids = gridStruct::component->getComponentEntities()->entitiesArray();
				for (uint32 i = 0; i < count; i++)
				{
					entityClass *e = grids[i];
					ENGINE_GET_COMPONENT(transform, t, e);
					t.position = player.position + randomDirection3() * vec3(100, 1, 100);
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
			if (player.powerups[puTurret] == 0)
				return;
			player.powerups[puTurret]--;
			statistics.turretsPlaced++;
			entityClass *turret = entities()->newEntity(entities()->generateUniqueName());
			ENGINE_GET_COMPONENT(transform, transform, turret);
			transform.position = player.position;
			transform.position[1] = 0;
			transform.orientation = quat(degs(), randomAngle(), degs());
			transform.scale = 3;
			ENGINE_GET_COMPONENT(render, render, turret);
			render.object = hashString("grid/player/turret.object");
			GRID_GET_COMPONENT(turret, tr, turret);
			tr.timeout = 60 * 30;
			tr.shooting = 2;

			uint32 sounds[] = {
				hashString("grid/speech/use/engaging-a-turret.wav"),
				hashString("grid/speech/use/turret-engaged.wav"),
				0 };
			soundSpeech(sounds);
		}

		void eventDecoy()
		{
			if (player.powerups[puDecoy] == 0)
				return;
			player.powerups[puDecoy]--;
			statistics.decoysUsed++;
			entityClass *decoy = entities()->newEntity(entities()->generateUniqueName());
			ENGINE_GET_COMPONENT(transform, playerTransform, player.playerEntity);
			ENGINE_GET_COMPONENT(transform, transform, decoy);
			transform = playerTransform;
			transform.scale *= 2;
			ENGINE_GET_COMPONENT(render, render, decoy);
			render.object = hashString("grid/player/player.object");
			GRID_GET_COMPONENT(decoy, dec, decoy);
			dec.speed = -player.speed;
			dec.timeout = 10 * 30;
			dec.rotation = interpolate(quat(), quat(randomAngle(), randomAngle(), randomAngle()), 3e-3);

			uint32 sounds[] = {
				hashString("grid/speech/use/decoy-launched.wav"),
				hashString("grid/speech/use/launching-a-decoy.wav"),
				0 };
			soundSpeech(sounds);
		}

		void controlsUpdate()
		{
			player.arrowsDirection = vec3();
			player.moveDirection = vec3();
			player.fireDirection = vec3();

			if (player.cinematic)
			{
				player.fireDirection = randomDirection3();
				player.fireDirection[1] = 0;
				return;
			}

			{
				setMousePosition();
				if ((buttonMap & mouseButtonsFlags::Left) == mouseButtonsFlags::Left)
					player.mouseLeftPosition = player.mouseCurrentPosition;
				if ((buttonMap & mouseButtonsFlags::Right) == mouseButtonsFlags::Right)
					player.mouseRightPosition = player.mouseCurrentPosition;
			}

			{
				if (keyMap[87] || keyMap[265]) // w, up
					player.arrowsDirection += vec3(0, 0, -1);
				if (keyMap[83] || keyMap[264]) // s, down
					player.arrowsDirection += vec3(0, 0, 1);
				if (keyMap[65] || keyMap[263]) // a, left
					player.arrowsDirection += vec3(-1, 0, 0);
				if (keyMap[68] || keyMap[262]) // d, right
					player.arrowsDirection += vec3(1, 0, 0);
			}

			switch (confControlMovement)
			{
			case 0: // arrows (absolute)
				player.moveDirection = player.arrowsDirection;
				break;
			case 1: // arrows (relative)
			{
				ENGINE_GET_COMPONENT(transform, tr, player.playerEntity);
				player.moveDirection = tr.orientation * player.arrowsDirection;
			} break;
			case 2: // lmb
				player.moveDirection = player.mouseLeftPosition - player.position;
				if (player.moveDirection.squaredLength() < 100)
					player.moveDirection = vec3();
				break;
			case 3: // rmb
				player.moveDirection = player.mouseRightPosition - player.position;
				if (player.moveDirection.squaredLength() < 100)
					player.moveDirection = vec3();
				break;
			case 4: // cursor position
				player.moveDirection = player.mouseCurrentPosition - player.position;
				if (player.moveDirection.squaredLength() < 100)
					player.moveDirection = vec3();
				break;
			}

			switch (confControlFiring)
			{
			case 0: // arrows (absolute)
				player.fireDirection = player.arrowsDirection;
				break;
			case 1: // arrows (relative)
			{
				ENGINE_GET_COMPONENT(transform, tr, player.playerEntity);
				player.fireDirection = tr.orientation * player.arrowsDirection;
			} break;
			case 2: // lmb
				player.fireDirection = player.mouseLeftPosition - player.position;
				break;
			case 3: // rmb
				player.fireDirection = player.mouseRightPosition - player.position;
				break;
			case 4: // cursor position
				player.fireDirection = player.mouseCurrentPosition - player.position;
				break;
			}
		}

		void shipMovement()
		{
			if (player.cinematic)
				return;

			ENGINE_GET_COMPONENT(transform, tr, player.playerEntity);
			player.position = tr.position;
			player.scale = tr.scale;

			if (player.moveDirection != vec3())
			{
				real maxSpeed = player.powerups[puMaxSpeed] * 0.3 + 0.8;
				vec3 change = player.moveDirection.normalize() * (player.powerups[puAcceleration] + 1) * 0.1;
				if (confControlMovement == 1 && ((tr.orientation * vec3(0, 0, -1)).dot(normalize(player.speed + change)) < 1e-5))
					player.speed = vec3();
				else
					player.speed += change;
				if (player.speed.squaredLength() > maxSpeed * maxSpeed)
					player.speed = player.speed.normalize() * maxSpeed;
				if (change.squaredLength() > 0.01)
				{
					entityClass *spark = entities()->newEntity();
					ENGINE_GET_COMPONENT(transform, transform, spark);
					transform.scale = cage::random() * 0.2 + 0.3;
					transform.position = player.position + tr.orientation * vec3((sint32)(statistics.updateIterationNoPause % 2) * 1.2 - 0.6, 0, 1) * tr.scale;
					transform.orientation = randomDirectionQuat();
					ENGINE_GET_COMPONENT(render, render, spark);
					render.object = hashString("grid/environment/spark.object");
					GRID_GET_COMPONENT(effect, ef, spark);
					ef.speed = (change + randomDirection3() * 0.05) * cage::random() * -5;
					ef.ttl = random(10, 15);
					ENGINE_GET_COMPONENT(animatedTexture, at, spark);
					at.animationStart = player.updateTime;
					at.animationSpeed = 30.f / ef.ttl;
				}
			}
			else
				player.speed *= 0.97;

			if (player.position.length() > player.mapNoPullRadius)
			{
				vec3 pullToCenter = -player.position.normalize() * pow((player.position.length() - player.mapNoPullRadius) / player.mapNoPullRadius, 2);
				player.speed += pullToCenter;
			}

			player.speed[1] = 0;
			player.position += player.speed;
			player.position[1] = 0.5;
			tr.position = player.position;
			player.monstersTarget = tr.position + player.speed * 3;
			tr.scale = player.scale;
			if (player.speed.squaredLength() > 1e-5)
				tr.orientation = quat(degs(), aTan2(-player.speed[2], -player.speed[0]), degs());
		}

		void shipShield()
		{
			if (!player.playerEntity || !player.shieldEntity)
				return;
			ENGINE_GET_COMPONENT(transform, tr, player.playerEntity);
			ENGINE_GET_COMPONENT(transform, trs, player.shieldEntity);
			trs.position = tr.position;
			trs.scale = tr.scale;
			if (player.powerups[puShield] > 0)
			{
				ENGINE_GET_COMPONENT(render, render, player.shieldEntity);
				render.object = hashString("grid/player/shield.object");
				ENGINE_GET_COMPONENT(voice, sound, player.shieldEntity);
				sound.sound = hashString("grid/player/shield.ogg");
				sound.soundStart = -1;
			}
			else
			{
				player.shieldEntity->removeComponent(renderComponent::component);
				player.shieldEntity->removeComponent(voiceComponent::component);
			}
		}

		void shipFiring()
		{
			if (player.shootingCooldown > 0)
			{
				player.shootingCooldown = max(-1, player.shootingCooldown - 1);
				return;
			}
			if (player.fireDirection == vec3())
				return;

			player.shootingCooldown += real(4) * real(1.3).pow(player.powerups[puMultishot]) * real(0.7).pow(player.powerups[puFiringSpeed]);

			for (real i = player.powerups[puMultishot] * -0.5; i < player.powerups[puMultishot] * 0.5 + 1e-5; i += 1)
			{
				statistics.shotsFired++;
				entityClass *shot = entities()->newEntity(entities()->generateUniqueName());
				ENGINE_GET_COMPONENT(transform, transform, shot);
				rads dir = aTan2(-player.fireDirection[2], -player.fireDirection[0]);
				dir += degs(i * 10);
				transform.orientation = quat(degs(), dir, degs());
				if (player.powerups[puSuperDamage] > 0)
					transform.scale *= 1.2;
				ENGINE_GET_COMPONENT(render, render, shot);
				render.object = hashString("grid/player/shot.object");
				render.color = player.shotsColor;
				GRID_GET_COMPONENT(shot, sh, shot);
				sh.speed = vec3(-sin(dir), 0, -cos(dir)) * (player.powerups[puShotsSpeed] + 1.5) + player.speed * 0.3;
				sh.damage = player.powerups[puShotsDamage] + (player.powerups[puSuperDamage] ? 4 : 1);
				sh.homing = player.powerups[puHomingShots] > 0;
				transform.position = player.position + sh.speed.normalize() * player.scale;
			}
		}

		void turretsUpdate()
		{
			uint32 count = turretStruct::component->getComponentEntities()->entitiesCount();
			entityClass *const *turrets = turretStruct::component->getComponentEntities()->entitiesArray();
			for (uint32 i = 0; i < count; i++)
			{
				entityClass *e = turrets[i];
				GRID_GET_COMPONENT(turret, tu, e);
				if (tu.timeout-- == 0)
				{
					e->addGroup(entitiesToDestroy);
					continue;
				}
				ENGINE_GET_COMPONENT(transform, tr, e);
				tr.orientation = quat(degs(), degs(1), degs()) * tr.orientation;
				if (tu.shooting > 0)
				{
					tu.shooting--;
					continue;
				}
				tu.shooting = 10;
				for (uint32 i = 0; i < 6; i++)
				{
					statistics.shotsTurret++;
					entityClass *shot = entities()->newEntity(entities()->generateUniqueName());
					ENGINE_GET_COMPONENT(transform, transform, shot);
					transform.orientation = quat(degs(), degs(i * 60), degs()) * tr.orientation;
					transform.position = tr.position + transform.orientation * vec3(0, 0, -1) * 2;
					ENGINE_GET_COMPONENT(render, render, shot);
					render.object = hashString("grid/player/shot.object");
					render.color = player.shotsColor;
					GRID_GET_COMPONENT(shot, sh, shot);
					sh.speed = transform.orientation * vec3(0, 0, -1) * 2.5;
					sh.damage = 1;
					sh.homing = false;
				}
			}
		}

		void decoysUpdate()
		{
			uint32 count = decoyStruct::component->getComponentEntities()->entitiesCount();
			entityClass *const *decoys = decoyStruct::component->getComponentEntities()->entitiesArray();
			for (uint32 i = 0; i < count; i++)
			{
				entityClass *e = decoys[i];
				GRID_GET_COMPONENT(decoy, dec, e);
				if (dec.timeout-- == 0)
				{
					e->addGroup(entitiesToDestroy);
					continue;
				}
				ENGINE_GET_COMPONENT(transform, tr, e);
				tr.orientation = dec.rotation * tr.orientation;
				tr.position += dec.speed;
				tr.position[1] = 0;
				dec.speed *= 0.97;
				player.monstersTarget = tr.position;
			}
		}

		struct shotUpdateStruct
		{
			transformComponent &tr;
			shotStruct &sh;
			uint32 closestMonster;
			real closestDistance;
			uint32 homingMonster;
			real homingDistance;
			uint32 myName;

			shotUpdateStruct(entityClass *e) :
				tr(e->value<transformComponent>(transformComponent::component)),
				sh(e->value<shotStruct>(shotStruct::component)),
				closestMonster(0), closestDistance(real::PositiveInfinity),
				homingMonster(0), homingDistance(real::PositiveInfinity),
				myName(e->getName())
			{
				if (tr.position.squaredDistance(player.position) > 500 * 500)
				{
					e->addGroup(entitiesToDestroy); // destroy itself
					statistics.shotsDissipated++;
					return;
				}

				spatialQuery->intersection(sphere(tr.position, sh.speed.length() + tr.scale + (sh.homing ? 20 : 10)));
				const uint32 *res = spatialQuery->resultArray();
				for (uint32 i = 0, e = spatialQuery->resultCount(); i != e; i++)
					test(res[i]);

				if (closestMonster)
				{
					statistics.shotsHit++;
					entityClass *m = entities()->getEntity(closestMonster);
					GRID_GET_COMPONENT(monster, om, m);
					real dmg = sh.damage;
					sh.damage -= om.life;
					om.life -= dmg;
					if (om.life <= 1e-5)
					{
						statistics.shotsKill++;
						ENGINE_GET_COMPONENT(transform, mtr, m);
						m->addGroup(entitiesToDestroy); // destroy the monster
						monsterExplosion(m);
						if (om.destroyedSound)
							soundEffect(om.destroyedSound, mtr.position);
						if (om.shotDownCallback)
						{
							om.shotDownCallback(closestMonster);
							om.shotDownCallback.clear();
						}
						else
						{
							real r = cage::random();
							if (r < player.powerupSpawnChance)
							{
								player.powerupSpawnChance -= 1;
								powerupSpawn(mtr.position);
							}
							player.powerupSpawnChance += interpolate(1.0 / 50, 1.0 / 400, clamp((statistics.powerupsSpawned + 5) / 30.f, 0.f, 1.f));
						}
						player.score += max(numeric_cast<uint32>(min(om.damage, 200)), 1u);
					}
					if (sh.damage <= 1e-5)
					{
						shotExplosion(e);
						e->addGroup(entitiesToDestroy); // destroy the shot
						return;
					}
					sh.speed += randomDirection3() * 0.5;
					sh.homing = false;
				}
				else if (sh.homing)
				{
					if (homingMonster)
					{
						entityClass *m = entities()->getEntity(homingMonster);
						ENGINE_GET_COMPONENT(transform, mtr, m);
						vec3 toOther = normalize(mtr.position - tr.position);
						real spd = sh.speed.length();
						sh.speed = toOther * spd;
						tr.orientation = quat(degs(), aTan2(-toOther[2], -toOther[0]), degs());
					}
					else
					{
						// homing missiles are shivering
						tr.position += sh.speed.normalize() * quat(degs(), degs(90), degs()) * sin(rads::Full * (statistics.updateIterationPaused + myName * 5) / 30 * 2) * 0.5;
					}
				}

				sh.speed[1] = 0;
				tr.position += sh.speed;
				tr.position[1] = 0;
			}

			void test(uint32 otherName)
			{
				if (otherName == myName || !entities()->hasEntity(otherName))
					return;

				entityClass *e = entities()->getEntity(otherName);
				ENGINE_GET_COMPONENT(transform, ot, e);
				vec3 toOther = ot.position - tr.position;
				if (e->hasComponent(gridStruct::component))
				{
					GRID_GET_COMPONENT(grid, og, e);
					og.speed += sh.speed.normalize() * (0.2f / max(1, toOther.length()));
					return;
				}
				if (!e->hasComponent(monsterStruct::component))
					return;
				GRID_GET_COMPONENT(monster, om, e);
				if (om.life <= 0)
					return;
				real dist = toOther.length();
				if (dist < closestDistance)
				{
					if (collisionTest(tr.position, tr.scale, sh.speed, ot.position, ot.scale, om.speed))
					{
						closestMonster = otherName;
						closestDistance = dist;
					}
				}
				if (dist < homingDistance)
				{
					homingMonster = otherName;
					homingDistance = dist;
				}
			}
		};

		void shotsUpdate()
		{
			uint32 count = shotStruct::component->getComponentEntities()->entitiesCount();
			entityClass *const *shots = shotStruct::component->getComponentEntities()->entitiesArray();
			for (uint32 i = 0; i < count; i++)
			{
				entityClass *e = shots[i];
				shotUpdateStruct u(e);
			}
		}

		void scoreUpdate()
		{
			uint32 lg = player.scorePrevious >= 20000 ? 10 : player.scorePrevious >= 2000 ? 2 : 1;
			uint32 sg = lg * 500;
			uint32 ld = (player.score - player.scorePrevious) / sg;
			if (ld)
			{
				player.life += ld * lg;
				player.scorePrevious += ld * sg;

				uint32 sounds[] = {
					hashString("grid/speech/gain/additional-life.wav"),
					hashString("grid/speech/gain/doing-fine.wav"),
					hashString("grid/speech/gain/doing-well.wav"),
					hashString("grid/speech/gain/fantastic.wav"),
					hashString("grid/speech/gain/go-on.wav"),
					hashString("grid/speech/gain/keep-going.wav"),
					hashString("grid/speech/gain/lets-roll.wav"),
					0 };
				soundSpeech(sounds);
			}
		}

		void eventAction(uint32 option)
		{
			if (confControlBomb == option)
				eventBomb();
			if (confControlTurret == option)
				eventTurret();
			if (confControlDecoy == option)
				eventDecoy();
		}

		void eventLetter(uint32 key)
		{
			for (uint32 o = 0; o < sizeof(letters); o++)
				if (letters[o] == key)
					eventAction(o + 4);
		}

		real closestMonsterToPlayer;
		void closestMonsterTestEntity(uint32 otherName)
		{
			if (!entities()->hasEntity(otherName))
				return;
			entityClass *e = entities()->getEntity(otherName);
			if (e->hasComponent(monsterStruct::component))
			{
				ENGINE_GET_COMPONENT(transform, p, e);
				real d = p.position.distance(player.position);
				closestMonsterToPlayer = min(closestMonsterToPlayer, d);
			}
		}

		void musicUpdate()
		{
			if (player.cinematic)
			{
				sounds.suspense = 0;
				return;
			}
			if (player.paused)
			{
				sounds.suspense = 1;
				return;
			}

			static const real distMin = 25;
			static const real distMax = 35;
			closestMonsterToPlayer = real::PositiveInfinity;
			spatialQuery->intersection(sphere(player.position, distMax));
			const uint32 *res = spatialQuery->resultArray();
			for (uint32 i = 0, e = spatialQuery->resultCount(); i != e; i++)
				closestMonsterTestEntity(res[i]);
			closestMonsterToPlayer = clamp(closestMonsterToPlayer, distMin, distMax);
			sounds.suspense = (closestMonsterToPlayer - distMin) / (distMax - distMin);
		}
	}

	void playerInit()
	{
		for (uint32 i = 0; i < sizeof(keyMap); i++)
			keyMap[i] = false;
		buttonMap = (mouseButtonsFlags)0;

		{
			player.playerEntity = entities()->newEntity(entities()->generateUniqueName());
			ENGINE_GET_COMPONENT(transform, transform, player.playerEntity);
			transform.scale = player.scale;
			ENGINE_GET_COMPONENT(render, render, player.playerEntity);
			render.object = hashString("grid/player/player.object");
			player.monstersTarget = player.position = vec3();
		}

		{
			player.shieldEntity = entities()->newEntity(entities()->generateUniqueName());
			ENGINE_GET_COMPONENT(transform, transform, player.shieldEntity);
			ENGINE_GET_COMPONENT(animatedTexture, aniTex, player.shieldEntity);
			aniTex.animationSpeed = 0.05;
		}

		player.shotsColor = player.cinematic ? convertHsvToRgb(vec3(cage::random(), 1, 1)) : vec3((float)confPlayerShotColorR, (float)confPlayerShotColorG, (float)confPlayerShotColorB);
	}

	void playerUpdate()
	{
		if (!player.paused)
		{
			controlsUpdate();
			shipMovement();
			decoysUpdate();
			shipFiring();
			turretsUpdate();
			shotsUpdate();
			scoreUpdate();
		}
		musicUpdate();
		shipShield();
	}

	void playerDone()
	{
		player.playerEntity->addGroup(entitiesToDestroy);
		player.playerEntity = nullptr;
		environmentExplosion(player.position, player.speed, player.deathColor, 20, player.scale);
		for (uint32 i = 0; i < 10; i++)
			environmentExplosion(player.position, randomDirection3() * vec3(1, 0.1, 1), player.deathColor, 5, player.scale);
	}

	void mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		statistics.buttonPressed++;
		buttonMap = (mouseButtonsFlags)(buttonMap | buttons);
	}

	void mouseRelease(mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{
		buttonMap = (mouseButtonsFlags)(buttonMap & ~buttons);
		if ((buttons & mouseButtonsFlags::Left) == mouseButtonsFlags::Left)
			eventAction(0);
		if ((buttons & mouseButtonsFlags::Right) == mouseButtonsFlags::Right)
			eventAction(1);
		if ((buttons & mouseButtonsFlags::Middle) == mouseButtonsFlags::Middle)
			eventAction(2);
	}

	void mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point)
	{}

	void keyPress(uint32 key, modifiersFlags modifiers)
	{
		statistics.keyPressed++;
		if (key < sizeof(keyMap))
			keyMap[key] = true;
	}

	void keyRelease(uint32 key, modifiersFlags modifiers)
	{
		if (key < sizeof(keyMap))
			keyMap[key] = false;

		switch (key)
		{
		case ' ':
			eventAction(3);
			break;
		default:
			eventLetter(key);
			break;
		}
	}
}
