#pragma once
#include "GUI.h"
#include <GLM/glm.hpp>
#include "Light.h"



struct SunLightSetting
{
    bool enableSunlight = true;
    float sunlightIntensity = 100000.0f;
    float sunlightHaloSize = 10.0f;
    float sunlightHaloFalloff = 80.0f;
    float sunlightAngularRadius = 1.9f;
    glm::vec3 sunlightDirection = { 0.6, -1.0, -0.8 };
    glm::vec3 sunlightColor = Color::toLinear<ACCURATE>({ 0.98, 0.92, 0.89 });
};


struct PointLightSetting
{
    bool enable = true;
    float pointlightIntensity = 100000.0f;
    glm::vec3 pointLightPosition = glm::vec3(0,0,0);
    glm::vec3 pointlightColor = Color::toLinear<ACCURATE>({ 0.98, 0.92, 0.89 });
};


struct LightSettings 
{
    int lightType = 0;
    float iblIntensity = 30000.0f;
    float iblRotation = 0.0f;

    SunLightSetting sunLight;
    std::vector<PointLightSetting> pointLights;

};


struct CameraSetting
{
    float cameraAperture = 16.0f;
    float cameraSpeed = 125.0f;
    float cameraISO = 100.0f;
};

struct MaterialSettings
{
    glm::vec4 baseColor = glm::vec4(0.8, 0.8, 0.8, 1.0);
    float metallic = 1.0;
    float roughness = 0.4;
    float reflectance = 0.5;
    glm::vec4 emissive = glm::vec4(0.0, 0.0, 0.0, 0);
    float ambientOcclusion = 1.0;
};

class CCustomGUI : public IGUI
{
public:
	CCustomGUI(const std::string& vName, int vExcutionOrder);
	virtual ~CCustomGUI();

	virtual void initV() override;
	virtual void updateV() override;

private:
	glm::vec3 m_LightPos = glm::vec3(0, 1, 0);	//30, 308, -130
	glm::vec3 m_LightDir = glm::normalize(glm::vec3(-0.3, -1, 0));


    LightSettings light;
    MaterialSettings material;
    CameraSetting cameraSetting;
};