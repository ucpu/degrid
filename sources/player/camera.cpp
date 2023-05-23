#include <cage-core/config.h>
#include <cage-core/entities.h>
#include <cage-core/hashString.h>
#include <cage-core/variableSmoothingBuffer.h>
#include <cage-engine/sceneScreenSpaceEffects.h>

#include "../game.h"

namespace
{
	Entity *primaryCameraEntity;

	constexpr float CameraDistance = 220;

	VariableSmoothingBuffer<Vec3, 16> playerPosSmoother;
	VariableSmoothingBuffer<Vec3, 16> cameraPosSmoother;
	Real cameraFovShakeAmplitude;

	const auto engineUpdateListener = controlThread().update.listen([]() {
		if (game.gameOver)
			return;

		// fov (breathing effect)
		cameraFovShakeAmplitude = interpolate(cameraFovShakeAmplitude, (Real)engineEntities()->component<BossComponent>()->count(), 0.003);
		const Rads fov = Degs(40 + 0.5 * cage::sin(Degs(engineControlTime() * 2.5e-4)) * saturate(cameraFovShakeAmplitude));
		primaryCameraEntity->value<CameraComponent>().camera.perspectiveFov = fov;

		{ // camera
			playerPosSmoother.add(game.playerEntity->value<TransformComponent>().position);
			TransformComponent &c = primaryCameraEntity->value<TransformComponent>();
			const Quat rot = Quat(Degs(game.cinematic ? -40 : -90), {}, {});
			const Vec3 ct = playerPosSmoother.smooth() + rot * Vec3(0, 0, CameraDistance);
			cameraPosSmoother.add(ct);
			c.position = cameraPosSmoother.smooth();
			c.orientation = Quat(playerPosSmoother.smooth() - c.position, Vec3(0, 0, -1));
		}
	}, 50);

	const auto gameStartListener = gameStartEvent().listen([]() {
		primaryCameraEntity = engineEntities()->createUnique();
		CameraComponent &c = primaryCameraEntity->value<CameraComponent>();
		c.near = 150;
		c.far = 1000;
		c.camera.perspectiveFov = Degs(40);
		c.ambientColor = Vec3(1);
		c.ambientIntensity = 0.3;
		primaryCameraEntity->value<ScreenSpaceEffectsComponent>().effects = ScreenSpaceEffectsFlags::Default & ~ScreenSpaceEffectsFlags::AmbientOcclusion;
		primaryCameraEntity->value<ListenerComponent>().rolloffFactor = 0.5 / CameraDistance;
	}, 50);
}

Entity *getPrimaryCameraEntity()
{
	return primaryCameraEntity;
}

