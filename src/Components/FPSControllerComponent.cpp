#include "Components/FPSControllerComponent.h"
#include "Renderer/Camera.h"
#include "function/Object.h"
#include "function/InputManager.h"
#include <iostream>
namespace Render {
	void Render::FPSControllerComponent::onEnable()
	{
		if (mCamera) {
			mCamera->setPosition(owner()->worldPosition());
			mControl.setDirection(mCamera->getDirection());
		}
	}

	void FPSControllerComponent::setCamera(Camera* cam) { 
		mCamera = cam; 
		if (mCamera) {
			mCamera->setPosition(owner()->worldPosition());
			mControl.setDirection(mCamera->getDirection());
		}
	}
	void FPSControllerComponent::setSensitivity(float s) {
		mSensitivity = s;
		mControl.setSensitivity(s);
	}
	void FPSControllerComponent::setSpeed(float s) { mSpeed = s; }
	float FPSControllerComponent::getSensitivity() const { return mSensitivity; }
	float FPSControllerComponent::getSpeed() const { return mSpeed; }

	FPSControllerComponent::FPSControllerComponent()
	{
		this->setSensitivity(0.025f);
		this->setSpeed(1.f);
	}

	void FPSControllerComponent::onUpdate(float dt)
	{
		auto inputMgr = InputManager::instance();
		bool isA = inputMgr->isKeyDown(KeyCode::A);
		bool isD = inputMgr->isKeyDown(KeyCode::D);
		bool isW = inputMgr->isKeyDown(KeyCode::W);
		bool isS = inputMgr->isKeyDown(KeyCode::S);
		bool isSpace = inputMgr->isKeyDown		(KeyCode::Space);
		bool isCtrlLeft = inputMgr->isKeyDown	(KeyCode::LeftControl);

		double dx, dy;
		inputMgr->getDeltaCursorPos(dx, dy);

		mControl.setCursorDelta(dx, -dy);
		mControl.update(dt);

		auto dir = mControl.getDirection();
		float xChange = (isD ? 1.f : 0.f) - (isA ? 1.f : 0.f);
		float zChange = (isW ? 1.f : 0.f) - (isS ? 1.f : 0.f);
		float yChange = (isSpace ? 1.f : 0.f) - (isCtrlLeft ? 1.f : 0.f);


		vec3 worldUp = vec3(0.f, 1.f, 0.f);
		auto forwardDir = mControl.getDirection();
		vec3 rightDir = normalize(cross(forwardDir, worldUp));

		vec3 moveDelta = (rightDir * xChange) + (worldUp * yChange) + (forwardDir * zChange);

		if (length(moveDelta) > 0.001f) {
			moveDelta = normalize(moveDelta) * getSpeed() * dt;
			const auto& curPosition = owner()->localPosition();
			owner()->setLocalPosition(curPosition + moveDelta);
		}

		if (mCamera) {
			mCamera->setDirection(forwardDir);
		}
	}

	void FPSControllerComponent::onTransformChanged()
	{
		if (mCamera) {
			mCamera->setPosition(owner()->worldPosition());
		}
	}

}

