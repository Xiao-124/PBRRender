#pragma once
#include "GameObject.h"
class CGroundObject : public IGameObject
{
public:
	CGroundObject(const std::string& vGameObjectName, int vExecutionOrder);
	virtual ~CGroundObject();

	virtual void initV() override;
	virtual void updateV() override;
	
};