#include "LightCamera.h"

CLightCamera::CLightCamera(const std::string& vGameObjectName, int vExecutionOrder)
{
}

CLightCamera::~CLightCamera()
{
}

void CLightCamera::initV()
{
	CLight light;
    light.setColor(Color::toLinear<ACCURATE>(sRGBColor(0.98f, 0.92f, 0.89f)));
    light.setIntensity(110000);
    light.setDirection({ 0.7, -1, -0.8 });
    light.setSunAngularRadius(1.9f);

	//m_LightPos = ElayGraphics::ResourceManager::getSharedDataByName<glm::vec3>("LightPos");
	//m_LightDir = ElayGraphics::ResourceManager::getSharedDataByName<glm::vec3>("LightDir");
	//
	//if (abs(m_LightDir) == m_LightUpVector)
	//	m_LightDir.z += 0.01f;
	//m_LightViewMatrix = glm::lookAt(m_LightPos, m_LightPos + m_LightDir, m_LightUpVector);
	//m_LightProjectionMatrix = glm::ortho(-m_CameraSizeExtent, m_CameraSizeExtent, -m_CameraSizeExtent, m_CameraSizeExtent, 0.1f, 1000.0f);
	//ElayGraphics::ResourceManager::registerSharedData("LightViewMatrix", m_LightViewMatrix);
	//ElayGraphics::ResourceManager::registerSharedData("LightProjectionMatrix", m_LightProjectionMatrix);
	//ElayGraphics::ResourceManager::registerSharedData("LightCameraAreaInWorldSpace", (2.0f * m_CameraSizeExtent) * (2.0f * m_CameraSizeExtent));
   
}

void CLightCamera::updateV()
{
}
