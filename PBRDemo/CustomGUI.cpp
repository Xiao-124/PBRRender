#include "CustomGUI.h"
#include "Interface.h"
#include <vector>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <imgui.h>
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>
#include "ImGuiExtension.h"
#include "GLFWWindow.h"




CCustomGUI::CCustomGUI(const std::string& vName, int vExcutionOrder) : IGUI(vName, vExcutionOrder)
{



}

CCustomGUI::~CCustomGUI()
{


}

void CCustomGUI::initV()
{
    ElayGraphics::ResourceManager::registerSharedData("MaterialSettings", material);
    ElayGraphics::ResourceManager::registerSharedData("LightSettings", light);
    ElayGraphics::ResourceManager::registerSharedData("CameraSetting", cameraSetting);

}

void CCustomGUI::updateV()
{
    if (collapsingHeader("Light"))
    {
        indent();
        
        if (collapsingHeader("Indirect light")) 
        {          
            sliderFloat("IBL intensity", &light.iblIntensity, 0.0f, 100000.0f);
            sliderAngle("IBL rotation", &light.iblRotation);
        }
        

        std::vector<std::string> chooseType = {"Sun", "Point", "Direct", "Spot"};
        combo("Type##lightType", light.lightType, chooseType);
        if (light.lightType == 0)
        {
            if (collapsingHeader("Sunlight"))
            {
                checkBox("Enable sunlight", light.sunLight.enableSunlight);
                sliderFloat("Sun intensity", &light.sunLight.sunlightIntensity, 0.0f, 150000.0f);
                sliderFloat("Halo size", &light.sunLight.sunlightHaloSize, 1.01f, 40.0f);
                sliderFloat("Halo falloff", &light.sunLight.sunlightHaloFalloff, 4.0f, 1024.0f);
                sliderFloat("Sun radius", &light.sunLight.sunlightAngularRadius, 0.1f, 10.0f);
                directionWidget("Sun direction", &light.sunLight.sunlightDirection[0]);
            }
        }
        else if (light.lightType == 1)
        {
            if (collapsingHeader("PointLight"))
            {
                if (button("AddLight"))
                {
                    light.pointLights.push_back(PointLightSetting());
                }
                for (int i = 0; i < light.pointLights.size(); i++)
                {
                    if (collapsingHeader("Light" + std::to_string(i)))
                    {
                        checkBox("Enablelight##" + std::to_string(i), light.pointLights[i].enable);
                        sliderFloat("Intensity##" + std::to_string(i), &light.pointLights[i].pointlightIntensity, 0, 1);
                        inputFloat3("Position##" + std::to_string(i), light.pointLights[i].pointLightPosition);
                        if (button("DeleteLight##" + std::to_string(i)))
                        {
                            light.pointLights.erase(light.pointLights.begin() + i);
                        }
                    }
                }
            }
        }
        unIndent();
    }
    
    if (collapsingHeader("Material"))
    {
        indent();
        //sliderFloat4("baseColor", material.baseColor, 0, 1);
        //colorButton("baseColor", material.baseColor);
        colorEdit("baseColor", material.baseColor);

        sliderFloat("metallic", &material.metallic, 0, 1);
        sliderFloat("roughness", &material.roughness, 0.0f, 1.0f);
        sliderFloat("reflectance", &material.reflectance, 0.0f, 1.0f);
        sliderFloat("ambientOcclusion", &material.ambientOcclusion, 0.0f, 1.0f);
        colorEdit("emissive", material.emissive);
        colorEdit("sheenColor", material.sheenColor);
        sliderFloat("sheenRoughness", &material.sheenRoughness, 0.0f, 1.0f);
        sliderFloat("clearCoat", &material.clearCoat, 0.0f, 1.0f);
        sliderFloat("clearCoatRoughness", &material.clearCoatRoughness, 0.0f, 1.0f);
        unIndent();
    }


    if (collapsingHeader("Camera"))
    {
        indent();
        sliderFloat("Aperture", &cameraSetting.cameraAperture, 1.0f, 32.0f);
        sliderFloat("Speed (1/s)", &cameraSetting.cameraSpeed, 1000.0f, 1.0f);
        sliderFloat("ISO", &cameraSetting.cameraISO, 25.0f, 6400.0f);
        unIndent();
    }

    ElayGraphics::ResourceManager::updateSharedDataByName("MaterialSettings", material);
    ElayGraphics::ResourceManager::updateSharedDataByName("LightSettings", light);
    ElayGraphics::ResourceManager::updateSharedDataByName("CameraSetting", cameraSetting);

}
