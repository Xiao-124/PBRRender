#pragma once
#include "GameObject.h"
#include <GLM/glm.hpp>
#include "Light.h"
class CLightCamera : public IGameObject
{
public:
	CLightCamera(const std::string& vGameObjectName, int vExecutionOrder);
	virtual ~CLightCamera();

	virtual void initV() override;
	virtual void updateV() override;

private:
	
};