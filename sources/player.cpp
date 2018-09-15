#include <set>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/config.h>
#include <cage-core/entities.h>
#include <cage-core/utility/spatial.h>
#include <cage-core/utility/color.h>
#include <cage-core/utility/hashString.h>
#include <cage-core/utility/hashTable.h> // hash function

#include <cage-client/core.h>
#include <cage-client/engine.h>
#include <cage-client/window.h>

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

			ENGINE_GET_COMPONENT(transform, playerTransform, player.playerEntity);

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
				player.moveDirection = player.mouseLeftPosition - playerTransform.position;
				if (player.moveDirection.squaredLength() < 100)
					player.moveDirection = vec3();
				break;
			case 3: // rmb
				player.moveDirection = player.mouseRightPosition - playerTransform.position;
				if (player.moveDirection.squaredLength() < 100)
					player.moveDirection = vec3();
				break;
			case 4: // cursor position
				player.moveDirection = player.mouseCurrentPosition - playerTransform.position;
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
				player.fireDirection = player.mouseLeftPosition - playerTransform.position;
				break;
			case 3: // rmb
				player.fireDirection = player.mouseRightPosition - playerTransform.position;
				break;
			case 4: // cursor position
				player.fireDirection = player.mouseCurrentPosition - playerTransform.position;
				break;
			}
		}

		void shipMovement()
		{
			if (player.cinematic)
				return;

			ENGINE_GET_COMPONENT(transform, tr, player.playerEntity);
			GRID_GET_COMPONENT(velocity, vl, player.playerEntity);

			if (player.moveDirection != vec3())
			{
				real maxSpeed = player.powerups[(uint32)powerupTypeEnum::MaxSpeed] * 0.3 + 0.8;
				vec3 change = player.moveDirection.normalize() * (player.powerups[(uint32)powerupTypeEnum::Acceleration] + 1) * 0.1;
				if (confControlMovement == 1 && ((tr.orientation * vec3(0, 0, -1)).dot(normalize(vl.velocity + change)) < 1e-5))
					vl.velocity = vec3();
				else
					vl.velocity += change;
				if (vl.velocity.squaredLength() > maxSpeed * maxSpeed)
					vl.velocity = vl.velocity.normalize() * maxSpeed;
				if (change.squaredLength() > 0.01)
				{
					entityClass *spark = entities()->newAnonymousEntity();
					ENGINE_GET_COMPONENT(transform, transform, spark);
					transform.scale = cage::random() * 0.2 + 0.3;
					transform.position = tr.position + tr.orientation * vec3((sint32)(statistics.updateIterationNoPause % 2) * 1.2 - 0.6, 0, 1) * tr.scale;
					transform.orientation = randomDirectionQuat();
					ENGINE_GET_COMPONENT(render, render, spark);
					render.object = hashString("grid/environment/spark.object");
					GRID_GET_COMPONENT(velocity, vel, spark);
					vel.velocity = (change + randomDirection3() * 0.05) * cage::random() * -5;
					GRID_GET_COMPONENT(timeout, ttl, spark);
					ttl.ttl = random(10, 15);
					ENGINE_GET_COMPONENT(animatedTexture, at, spark);
					at.startTime = currentControlTime();
					at.speed = 30.f / ttl.ttl;
				}
			}
			else
				vl.velocity *= 0.97;

			// pull to center
			if (tr.position.length() > mapNoPullRadius)
			{
				vec3 pullToCenter = -tr.position.normalize() * pow((tr.position.length() - mapNoPullRadius) / mapNoPullRadius, 2);
				vl.velocity += pullToCenter;
			}

			vl.velocity[1] = 0;
			tr.position[1] = 0.5;
			player.monstersTarget = tr.position + vl.velocity * 3;
			if (vl.velocity.squaredLength() > 1e-5)
				tr.orientation = quat(degs(), aTan2(-vl.velocity[2], -vl.velocity[0]), degs());
		}

		void shipShield()
		{
			if (!player.playerEntity || !player.shieldEntity)
				return;
			ENGINE_GET_COMPONENT(transform, tr, player.playerEntity);
			ENGINE_GET_COMPONENT(transform, trs, player.shieldEntity);
			trs.position = tr.position;
			trs.scale = tr.scale;
			if (player.powerups[(uint32)powerupTypeEnum::Shield] > 0)
			{
				ENGINE_GET_COMPONENT(render, render, player.shieldEntity);
				render.object = hashString("grid/player/shield.object");
				ENGINE_GET_COMPONENT(voice, sound, player.shieldEntity);
				sound.name = hashString("grid/player/shield.ogg");
				sound.startTime = -1;
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

			player.shootingCooldown += real(4) * real(1.3).pow(player.powerups[(uint32)powerupTypeEnum::Multishot]) * real(0.7).pow(player.powerups[(uint32)powerupTypeEnum::FiringSpeed]);

			ENGINE_GET_COMPONENT(transform, playerTransform, player.playerEntity);
			GRID_GET_COMPONENT(velocity, playerVelocity, player.playerEntity);

			for (real i = player.powerups[(uint32)powerupTypeEnum::Multishot] * -0.5; i < player.powerups[(uint32)powerupTypeEnum::Multishot] * 0.5 + 1e-5; i += 1)
			{
				statistics.shotsFired++;
				entityClass *shot = entities()->newUniqueEntity();
				ENGINE_GET_COMPONENT(transform, transform, shot);
				rads dir = aTan2(-player.fireDirection[2], -player.fireDirection[0]);
				dir += degs(i * 10);
				transform.orientation = quat(degs(), dir, degs());
				if (player.powerups[(uint32)powerupTypeEnum::SuperDamage] > 0)
					transform.scale *= 1.2;
				ENGINE_GET_COMPONENT(render, render, shot);
				render.object = hashString("grid/player/shot.object");
				render.color = player.shotsColor;
				GRID_GET_COMPONENT(velocity, vel, shot);
				vel.velocity = vec3(-sin(dir), 0, -cos(dir)) * (player.powerups[(uint32)powerupTypeEnum::ShotsSpeed] + 1.5) + playerVelocity.velocity * 0.3;
				GRID_GET_COMPONENT(shot, sh, shot);
				sh.damage = player.powerups[(uint32)powerupTypeEnum::ShotsDamage] + (player.powerups[(uint32)powerupTypeEnum::SuperDamage] ? 4 : 1);
				sh.homing = player.powerups[(uint32)powerupTypeEnum::HomingShots] > 0;
				transform.position = playerTransform.position + vel.velocity.normalize() * playerTransform.scale;
			}
		}

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

		struct shotUpdateStruct
		{
			transformComponent &tr;
			shotComponent &sh;
			velocityComponent &vl;
			uint32 closestMonster;
			real closestDistance;
			uint32 homingMonster;
			real homingDistance;
			uint32 myName;

			shotUpdateStruct(entityClass *e) :
				tr(e->value<transformComponent>(transformComponent::component)),
				sh(e->value<shotComponent>(shotComponent::component)),
				vl(e->value<velocityComponent>(velocityComponent::component)),
				closestMonster(0), closestDistance(real::PositiveInfinity),
				homingMonster(0), homingDistance(real::PositiveInfinity),
				myName(e->getName())
			{
				ENGINE_GET_COMPONENT(transform, playerTransform, player.playerEntity);
				if (tr.position.squaredDistance(playerTransform.position) > 500 * 500)
				{
					e->addGroup(entitiesToDestroy); // destroy itself
					statistics.shotsDissipated++;
					return;
				}

				spatialQuery->intersection(sphere(tr.position, vl.velocity.length() + tr.scale + (sh.homing ? 20 : 10)));
				for (uint32 otherName : spatialQuery->result())
				{
					if (otherName == myName || !entities()->hasEntity(otherName))
						continue;

					entityClass *e = entities()->getEntity(otherName);
					ENGINE_GET_COMPONENT(transform, ot, e);
					vec3 toOther = ot.position - tr.position;
					if (e->hasComponent(gridComponent::component))
					{
						GRID_GET_COMPONENT(velocity, og, e);
						og.velocity += vl.velocity.normalize() * (0.2f / max(1, toOther.length()));
						continue;
					}
					if (!e->hasComponent(monsterComponent::component))
						continue;
					GRID_GET_COMPONENT(monster, om, e);
					if (om.life <= 0)
						continue;
					real dist = toOther.length();
					if (dist < closestDistance)
					{
						GRID_GET_COMPONENT(velocity, ov, e);
						if (collisionTest(tr.position, tr.scale, vl.velocity, ot.position, ot.scale, ov.velocity))
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
						player.score += numeric_cast<uint32>(clamp(om.damage, 1, 200));
					}
					if (sh.damage <= 1e-5)
					{
						shotExplosion(e);
						e->addGroup(entitiesToDestroy); // destroy the shot
						return;
					}
					vl.velocity += randomDirection3() * 0.5;
					sh.homing = false;
				}
				else if (sh.homing)
				{
					if (homingMonster)
					{
						entityClass *m = entities()->getEntity(homingMonster);
						ENGINE_GET_COMPONENT(transform, mtr, m);
						vec3 toOther = normalize(mtr.position - tr.position);
						real spd = vl.velocity.length();
						vl.velocity = toOther * spd;
						tr.orientation = quat(degs(), aTan2(-toOther[2], -toOther[0]), degs());
					}
					else
					{
						// homing missiles are shivering
						tr.position += vl.velocity.normalize() * quat(degs(), degs(90), degs()) * sin(rads::Full * statistics.updateIterationPaused / 10 + degs(detail::hash(myName) % 360)) * (vl.velocity.length() * 0.3);
					}
				}

				vl.velocity[1] = 0;
				tr.position[1] = 0;
			}
		};

		void shotsUpdate()
		{
			for (entityClass *e : shotComponent::component->getComponentEntities()->entities())
				shotUpdateStruct u(e);
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
			ENGINE_GET_COMPONENT(transform, playerTransform, player.playerEntity);
			real closestMonsterToPlayer = real::PositiveInfinity;
			spatialQuery->intersection(sphere(playerTransform.position, distMax));
			for (uint32 otherName : spatialQuery->result())
			{
				if (!entities()->hasEntity(otherName))
					continue;
				entityClass *e = entities()->getEntity(otherName);
				if (e->hasComponent(monsterComponent::component))
				{
					ENGINE_GET_COMPONENT(transform, p, e);
					real d = p.position.distance(playerTransform.position);
					closestMonsterToPlayer = min(closestMonsterToPlayer, d);
				}
			}
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
			player.playerEntity = entities()->newUniqueEntity();
			ENGINE_GET_COMPONENT(transform, transform, player.playerEntity);
			transform.scale = playerScale;
			ENGINE_GET_COMPONENT(render, render, player.playerEntity);
			render.object = hashString("grid/player/player.object");
			player.monstersTarget = vec3();
		}

		{
			player.shieldEntity = entities()->newUniqueEntity();
			ENGINE_GET_COMPONENT(transform, transform, player.shieldEntity);
			(void)transform;
			ENGINE_GET_COMPONENT(animatedTexture, aniTex, player.shieldEntity);
			aniTex.speed = 0.05;
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
		ENGINE_GET_COMPONENT(transform, playerTransform, player.playerEntity);
		GRID_GET_COMPONENT(velocity, playerVelocity, player.playerEntity);
		environmentExplosion(playerTransform.position, playerVelocity.velocity, player.deathColor, 20, playerScale);
		for (uint32 i = 0; i < 10; i++)
			environmentExplosion(playerTransform.position, randomDirection3() * vec3(1, 0.1, 1), player.deathColor, 5, playerScale);
		player.playerEntity->addGroup(entitiesToDestroy);
		player.playerEntity = nullptr;
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
