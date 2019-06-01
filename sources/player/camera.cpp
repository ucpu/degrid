#include "../game.h"

#include <cage-core/config.h>
#include <cage-core/entities.h>
#include <cage-core/hashString.h>
#include <cage-core/variableSmoothingBuffer.h>

namespace
{
	entityClass *primaryCameraEntity;
	entityClass *skyboxPrimaryCameraEntity;
	entityClass *secondaryCameraEntity;
	entityClass *skyboxSecondaryCameraEntity;

	const vec3 ambientLight = vec3(0.2);
	const real cameraDistance = 220;

	variableSmoothingBuffer<vec3, 8> cameraSmoother;

	void engineUpdate()
	{
		if (game.gameOver)
			return;

		{ // camera
			ENGINE_GET_COMPONENT(transform, tr, primaryCameraEntity);
			ENGINE_GET_COMPONENT(transform, p, game.playerEntity);
			cameraSmoother.add(p.position + vec3(0, cameraDistance, 0));
			tr.position = cameraSmoother.oldest();
			tr.orientation = quat(p.position - tr.position, vec3(0, 0, -1));
		}

		{ // secondaryCamera
			static configBool secondaryCamera("degrid.secondary-camera.enabled", false);
			if (secondaryCamera)
			{
				ENGINE_GET_COMPONENT(transform, tp, game.playerEntity);
				{
					ENGINE_GET_COMPONENT(transform, ts, skyboxSecondaryCameraEntity);
					ts.orientation = tp.orientation;
					ENGINE_GET_COMPONENT(camera, c, skyboxSecondaryCameraEntity);
					c.cameraOrder = 3;
					c.renderMask = 2;
					c.near = 0.5;
					c.far = 3;
					c.ambientLight = vec3(1);
					c.camera.perspectiveFov = degs(60);
					c.viewportOrigin = vec2(0.7, 0);
					c.viewportSize = vec2(0.3, 0.3);
				}
				{
					ENGINE_GET_COMPONENT(transform, tc, secondaryCameraEntity);
					tc.position = tp.position;
					tc.orientation = tp.orientation;
					ENGINE_GET_COMPONENT(camera, c, secondaryCameraEntity);
					c.cameraOrder = 4;
					c.renderMask = 1;
					c.near = 3;
					c.far = 500;
					c.ambientLight = ambientLight;
					c.clear = (cameraClearFlags)0;
					c.camera.perspectiveFov = degs(60);
					c.viewportOrigin = vec2(0.7, 0);
					c.viewportSize = vec2(0.3, 0.3);
					c.effects = cameraEffectsFlags::CombinedPass & ~cameraEffectsFlags::AmbientOcclusion;
				}
			}
			else
			{
				skyboxSecondaryCameraEntity->remove(cameraComponent::component);
				secondaryCameraEntity->remove(cameraComponent::component);
			}
		}
	}

	void gameStart()
	{
		{
			skyboxPrimaryCameraEntity = entities()->createUnique();
			ENGINE_GET_COMPONENT(transform, transform, skyboxPrimaryCameraEntity);
			transform.orientation = quat(degs(-90), degs(), degs());
			ENGINE_GET_COMPONENT(camera, c, skyboxPrimaryCameraEntity);
			c.cameraOrder = 1;
			c.renderMask = 2;
			c.near = 0.5;
			c.far = 3;
			c.camera.perspectiveFov = degs(40);
			c.ambientLight = vec3(1);
		}

		{
			primaryCameraEntity = entities()->createUnique();
			ENGINE_GET_COMPONENT(transform, transform, primaryCameraEntity);
			transform.orientation = quat(degs(-90), degs(), degs());
			transform.position = vec3(0, cameraDistance, 0);
			cameraSmoother.seed(transform.position);
			ENGINE_GET_COMPONENT(camera, c, primaryCameraEntity);
			c.cameraOrder = 2;
			c.renderMask = 1;
			c.near = 150;
			c.far = 1000;
			c.camera.perspectiveFov = degs(40);
			c.ambientLight = ambientLight;
			c.clear = (cameraClearFlags)0;
			c.effects = cameraEffectsFlags::CombinedPass & ~cameraEffectsFlags::AmbientOcclusion;
			ENGINE_GET_COMPONENT(listener, l, primaryCameraEntity);
			static const float halfVolumeDistance = 30;
			l.attenuation[1] = 2.0 / halfVolumeDistance;
			l.attenuation[0] = l.attenuation[1] * transform.position[1] * -1;
		}

		{
			skyboxSecondaryCameraEntity = entities()->createUnique();
			secondaryCameraEntity = entities()->createUnique();
		}
	}

	class callbacksClass
	{
		eventListener<void()> engineUpdateListener;
		eventListener<void()> gameStartListener;
	public:
		callbacksClass()
		{
			engineUpdateListener.attach(controlThread().update, 50);
			engineUpdateListener.bind<&engineUpdate>();
			gameStartListener.attach(gameStartEvent(), 50);
			gameStartListener.bind<&gameStart>();
		}
	} callbacksInstance;
}

entityClass *getPrimaryCameraEntity()
{
	return primaryCameraEntity;
}

