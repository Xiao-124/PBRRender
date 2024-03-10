#include "ModelLoad.h"
#include "Interface.h"

CModelLoad::CModelLoad(const std::string& vGameObjectName, int vExecutionOrder) : IGameObject(vGameObjectName, vExecutionOrder)
{
}

CModelLoad::~CModelLoad()
{
}

void CModelLoad::initV()
{
	//setModel(ElayGraphics::ResourceManager::getOrCreateModel("../Model/Dragon/dragon.obj"));
	setModel(ElayGraphics::ResourceManager::getOrCreateModel("../Model/Monkey/monkey.obj"));	//../Model/CornellBox/HalfCornell/HalfCornell.obj
}

void CModelLoad::updateV()
{
}