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
#include "Shlobj.h"
#include "CommDlg.h"
#include <Windows.h>
#include <tchar.h>


std::string TCHAR2STRING(TCHAR* STR)
{
    int iLen = WideCharToMultiByte(CP_ACP, 0, STR, -1, NULL, 0, NULL, NULL);
    char* chRtn = new char[iLen * sizeof(char)];
    WideCharToMultiByte(CP_ACP, 0, STR, -1, chRtn, iLen, NULL, NULL);
    std::string str(chRtn);
    delete chRtn;
    return str;
}

static void computeToneMapPlot(ColorGradingSettings& settings, float* plot)
{
    float hdrMax = 10.0f;
    ToneMapper* mapper;
    switch (settings.toneMapping)
    {
    case ToneMappingSeeting::LINEAR:
        mapper = new LinearToneMapper;
        break;
    case ToneMappingSeeting::ACES_LEGACY:
        mapper = new ACESLegacyToneMapper;
        break;
    case ToneMappingSeeting::ACES:
        mapper = new ACESToneMapper;
        break;
    case ToneMappingSeeting::FILMIC:
        mapper = new FilmicToneMapper;
        break;
    case ToneMappingSeeting::AGX:
        mapper = new AgxToneMapper(settings.agxToneMapper.look);
        break;
    case ToneMappingSeeting::GENERIC:
        mapper = new GenericToneMapper(
            settings.genericToneMapper.contrast,
            settings.genericToneMapper.midGrayIn,
            settings.genericToneMapper.midGrayOut,
            settings.genericToneMapper.hdrMax
        );
        hdrMax = settings.genericToneMapper.hdrMax;
        break;
    case ToneMappingSeeting::DISPLAY_RANGE:
        mapper = new DisplayRangeToneMapper;
        break;
    default:
        mapper = new LinearToneMapper;
    }

    float a = std::log10(hdrMax * 1.5f / 1e-6f);
    for (size_t i = 0; i < 1024; i++) {
        float v = i;
        float x = 1e-6f * std::pow(10.0f, a * v / 1023.0f);
        plot[i] = (*mapper)(glm::vec3(x)).r;
    }
    delete mapper;
}

static void computeRangePlot(ColorGradingSettings& colorGrading, float* rangePlot)
{
    glm::vec4& ranges = colorGrading.ranges;
    ranges.y = glm::clamp(ranges.y, ranges.x + 1e-5f, ranges.w - 1e-5f); // darks
    ranges.z = glm::clamp(ranges.z, ranges.x + 1e-5f, ranges.w - 1e-5f); // lights

    for (size_t i = 0; i < 1024; i++)
    {
        float x = i / 1024.0f;
        float s = 1.0f - glm::smoothstep(ranges.x, ranges.y, x);
        float h = glm::smoothstep(ranges.z, ranges.w, x);
        rangePlot[i] = s;
        rangePlot[1024 + i] = 1.0f - s - h;
        rangePlot[2048 + i] = h;
    }
}

inline glm::vec3 curves(glm::vec3 v, glm::vec3 shadowGamma, glm::vec3 midPoint, glm::vec3 highlightScale)
{
    glm::vec3 d = 1.0f / (pow(midPoint, shadowGamma - 1.0f));
    glm::vec3 dark = pow(v, shadowGamma) * d;
    glm::vec3 light = highlightScale * (v - midPoint) + midPoint;
    return glm::vec3{
            v.r <= midPoint.r ? dark.r : light.r,
            v.g <= midPoint.g ? dark.g : light.g,
            v.b <= midPoint.b ? dark.b : light.b,
    };
}

static void computeCurvePlot(ColorGradingSettings& colorGrading, float* curvePlot)
{
    const auto& colorGradingOptions = colorGrading;
    for (size_t i = 0; i < 1024; i++) {
        glm::vec3 x{ i / 1024.0f * 2.0f };
        glm::vec3 y = curves(x,
            colorGradingOptions.gamma,
            colorGradingOptions.midPoint,
            colorGradingOptions.scale);
        curvePlot[i] = y.r;
        curvePlot[1024 + i] = y.g;
        curvePlot[2048 + i] = y.b;
    }
}

static float getRangePlotValue(int series, void* data, int index)
{
    return ((float*)data)[series * 1024 + index];
}

CCustomGUI::CCustomGUI(const std::string& vName, int vExcutionOrder) : IGUI(vName, vExcutionOrder)
{
    mToneMapPlot.resize(1024);
    mRangePlot.resize(1024 * 3);
    mCurvePlot.resize(1024 * 3);
}

CCustomGUI::~CCustomGUI()
{


}

void CCustomGUI::pushSliderColors(float hue)
{
    pushStyleColor(ElayGraphics::EGUIItemColor::GUIItemColor_FrameBg, (glm::vec4)ImColor::HSV(hue, 0.5f, 0.5f));
    pushStyleColor(ElayGraphics::EGUIItemColor::GUIItemColor_FrameBgHovered, (glm::vec4)ImColor::HSV(hue, 0.6f, 0.5f));
    pushStyleColor(ElayGraphics::EGUIItemColor::GUIItemColor_FrameBgActive, (glm::vec4)ImColor::HSV(hue, 0.7f, 0.5f));
    pushStyleColor(ElayGraphics::EGUIItemColor::GUIItemColor_SliderGrab, (glm::vec4)ImColor::HSV(hue, 0.9f, 0.9f));
}

void CCustomGUI::tooltipFloat(float value)
{
    if (isItemActive() || isItemHovered()) 
    {
        setTooltip("%.2f", value);
    }
}

void CCustomGUI::popSliderColors()
{ 
    popStyleColor(4); 
}

void CCustomGUI::rangePlotSeriesStart(int series)
{
    switch (series) 
    {
    case 0:
        pushStyleColor(ElayGraphics::EGUIItemColor::GUIItemColor_PlotLines, (glm::vec4)ImColor::HSV(0.4f, 0.25f, 1.0f));
        break;
    case 1:
        pushStyleColor(ElayGraphics::EGUIItemColor::GUIItemColor_PlotLines, (glm::vec4)ImColor::HSV(0.8f, 0.25f, 1.0f));
        break;
    case 2:
        pushStyleColor(ElayGraphics::EGUIItemColor::GUIItemColor_PlotLines, (glm::vec4)ImColor::HSV(0.17f, 0.21f, 1.0f));
        break;
    }
}

void CCustomGUI::rangePlotSeriesEnd(int series)
{
    if (series < 3) 
    {
        popStyleColor();
    }
}


void CCustomGUI::colorGradingUI(ColorGradingSettings& colorGrading, std::vector<float>& rangePlot, std::vector<float>& curvePlot, std::vector<float>& toneMapPlot)
{
    static glm::vec2 verticalSliderSize(18.0f, 160.0f);
    static glm::vec2 plotLinesSize(0.0f, 160.0f);
    static glm::vec2 plotLinesWideSize(0.0f, 120.0f);
    if (collapsingHeader("Color grading")) 
    {

        indent();
        checkBox("Enabled##colorGrading", colorGrading.enabled);

        int quality = (int)colorGrading.quality;
        
        combo("Quality##colorGradingQuality", quality, std::vector<std::string>{"Low", "Medium", "High", "Ultra"});
        colorGrading.quality = (decltype(colorGrading.quality))quality;

        int colorspace = (colorGrading.colorspace == Rec709 - Linear - D65) ? 0 : 1;     
        combo("Output color space", colorspace, std::vector<std::string>{"Rec709-Linear-D65", "Rec709-sRGB-D65"});
        colorGrading.colorspace = (colorspace == 0) ? Rec709 - Linear - D65 : Rec709 - sRGB - D65;

        int toneMapping = (int)colorGrading.toneMapping;

        
        combo("Tone-mapping", toneMapping,
            std::vector<std::string>{"Linear", "ACES (legacy)", "ACES", "Filmic", "AgX", "Generic", "Display Range"});
        colorGrading.toneMapping = (decltype(colorGrading.toneMapping))toneMapping;
        if (colorGrading.toneMapping == ToneMappingSeeting::GENERIC)
        {
            if (collapsingHeader("Tonemap parameters")) 
            {
                GenericToneMapperSettings& generic = colorGrading.genericToneMapper;
                sliderFloat("Contrast##genericToneMapper", &generic.contrast, 1e-5f, 3.0f);
                sliderFloat("Mid-gray in##genericToneMapper", &generic.midGrayIn, 0.0f, 1.0f);
                sliderFloat("Mid-gray out##genericToneMapper", &generic.midGrayOut, 0.0f, 1.0f);
                sliderFloat("HDR max", &generic.hdrMax, 1.0f, 64.0f);
            }
        }
        if (colorGrading.toneMapping == ToneMappingSeeting::AGX)
        {
            int agxLook = (int)colorGrading.agxToneMapper.look;
            combo("AgX Look", agxLook, std::vector<std::string>{"None", "Punchy", "Golden"});
            colorGrading.agxToneMapper.look = (decltype(colorGrading.agxToneMapper.look))agxLook;
        }

        computeToneMapPlot(colorGrading, &toneMapPlot[0]);

        pushStyleColor(ElayGraphics::EGUIItemColor::GUIItemColor_PlotLines, (glm::vec4)ImColor::HSV(0.17f, 0.21f, 0.9f));
        //std::vector<float> temp;
        plotLines("", toneMapPlot, 0.0f, 1.05f, glm::vec2(0, 160));
        popStyleColor();

        checkBox("Luminance scaling", colorGrading.luminanceScaling);
        checkBox("Gamut mapping", colorGrading.gamutMapping);

        sliderFloat("Exposure", &colorGrading.exposure, -10.0f, 10.0f);
        sliderFloat("Night adaptation", &colorGrading.nightAdaptation, 0.0f, 1.0f);

        if (collapsingHeader("White balance")) 
        {
            int temperature = colorGrading.temperature * 100.0f;
            int tint = colorGrading.tint * 100.0f;
            sliderInt("Temperature", &temperature, -100, 100);
            sliderInt("Tint", &tint, -100, 100);
            colorGrading.temperature = temperature / 100.0f;
            colorGrading.tint = tint / 100.0f;
        }

        if (collapsingHeader("Channel mixer")) 
        {
            pushSliderColors(0.0f / 7.0f);
            vSliderFloat("##outRed.r", verticalSliderSize, &colorGrading.outRed.r, -2.0f, 2.0f, "");
            tooltipFloat(colorGrading.outRed.r);
            sameLine();
            vSliderFloat("##outRed.g", verticalSliderSize, &colorGrading.outRed.g, -2.0f, 2.0f, "");
            tooltipFloat(colorGrading.outRed.g);
            sameLine();
            vSliderFloat("##outRed.b", verticalSliderSize, &colorGrading.outRed.b, -2.0f, 2.0f, "");
            tooltipFloat(colorGrading.outRed.b);
            sameLine(0.0f, 18.0f);
            popSliderColors();
        
            pushSliderColors(2.0f / 7.0f);
            vSliderFloat("##outGreen.r", verticalSliderSize, &colorGrading.outGreen.r, -2.0f, 2.0f, "");
            tooltipFloat(colorGrading.outGreen.r);
            sameLine();
            vSliderFloat("##outGreen.g", verticalSliderSize, &colorGrading.outGreen.g, -2.0f, 2.0f, "");
            tooltipFloat(colorGrading.outGreen.g);
            sameLine();
            vSliderFloat("##outGreen.b", verticalSliderSize, &colorGrading.outGreen.b, -2.0f, 2.0f, "");
            tooltipFloat(colorGrading.outGreen.b);
            sameLine(0.0f, 18.0f);
            popSliderColors();
        
            pushSliderColors(4.0f / 7.0f);
            vSliderFloat("##outBlue.r", verticalSliderSize, &colorGrading.outBlue.r, -2.0f, 2.0f, "");
            tooltipFloat(colorGrading.outBlue.r);
            sameLine();
            vSliderFloat("##outBlue.g", verticalSliderSize, &colorGrading.outBlue.g, -2.0f, 2.0f, "");
            tooltipFloat(colorGrading.outBlue.g);
            sameLine();
            vSliderFloat("##outBlue.b", verticalSliderSize, &colorGrading.outBlue.b, -2.0f, 2.0f, "");
            tooltipFloat(colorGrading.outBlue.b);
            popSliderColors();
        }

        //if (ImGui::CollapsingHeader("Tonal ranges")) 
        //{
        //    ImGui::ColorEdit3("Shadows", &colorGrading.shadows.x);
        //    ImGui::SliderFloat("Weight##shadowsWeight", &colorGrading.shadows.w, -2.0f, 2.0f);
        //    ImGui::ColorEdit3("Mid-tones", &colorGrading.midtones.x);
        //    ImGui::SliderFloat("Weight##midTonesWeight", &colorGrading.midtones.w, -2.0f, 2.0f);
        //    ImGui::ColorEdit3("Highlights", &colorGrading.highlights.x);
        //    ImGui::SliderFloat("Weight##highlightsWeight", &colorGrading.highlights.w, -2.0f, 2.0f);
        //    ImGui::SliderFloat4("Ranges", &colorGrading.ranges.x, 0.0f, 1.0f);
        //    computeRangePlot(colorGrading, rangePlot);
        //    ImGuiExt::PlotLinesSeries("", 3,
        //        rangePlotSeriesStart, getRangePlotValue, rangePlotSeriesEnd,
        //        rangePlot, 1024, 0, "", 0.0f, 1.0f, plotLinesWideSize);
        //}

        if (collapsingHeader("Color decision list")) 
        {
            sliderFloat3("Slope", colorGrading.slope, 0.0f, 2.0f);
            sliderFloat3("Offset", colorGrading.offset, -0.5f, 0.5f);
            sliderFloat3("Power", colorGrading.power, 0.0f, 2.0f);
        }
        
        if (collapsingHeader("Adjustments")) 
        {
            sliderFloat("Contrast", &colorGrading.contrast, 0.0f, 2.0f);
            sliderFloat("Vibrance", &colorGrading.vibrance, 0.0f, 2.0f);
            sliderFloat("Saturation", &colorGrading.saturation, 0.0f, 2.0f);
        }
        
        if (collapsingHeader("Curves")) 
        {
            checkBox("Linked curves", colorGrading.linkedCurves);
        
            computeCurvePlot(colorGrading, &curvePlot[0]);
        
            if (!colorGrading.linkedCurves) 
            {
                pushSliderColors(0.0f / 7.0f);
                vSliderFloat("##curveGamma.r", verticalSliderSize, &colorGrading.gamma.r, 0.0f, 4.0f, "");
                tooltipFloat(colorGrading.gamma.r);
                sameLine();
                vSliderFloat("##curveMid.r", verticalSliderSize, &colorGrading.midPoint.r, 0.0f, 2.0f, "");
                tooltipFloat(colorGrading.midPoint.r);
                sameLine();
                vSliderFloat("##curveScale.r", verticalSliderSize, &colorGrading.scale.r, 0.0f, 4.0f, "");
                tooltipFloat(colorGrading.scale.r);
                sameLine(0.0f, 18.0f);
                popSliderColors();
        
                pushStyleColor(ElayGraphics::EGUIItemColor::GUIItemColor_PlotLines, (glm::vec4)ImColor::HSV(0.0f, 0.7f, 0.8f));
                plotLines("", curvePlot, 0.0f, 2.0f, plotLinesSize);
                popStyleColor();
        
                pushSliderColors(2.0f / 7.0f);
                vSliderFloat("##curveGamma.g", verticalSliderSize, &colorGrading.gamma.g, 0.0f, 4.0f, "");
                tooltipFloat(colorGrading.gamma.g);
                sameLine();
                vSliderFloat("##curveMid.g", verticalSliderSize, &colorGrading.midPoint.g, 0.0f, 2.0f, "");
                tooltipFloat(colorGrading.midPoint.g);
                sameLine();
                vSliderFloat("##curveScale.g", verticalSliderSize, &colorGrading.scale.g, 0.0f, 4.0f, "");
                tooltipFloat(colorGrading.scale.g);
                sameLine(0.0f, 18.0f);
                popSliderColors();
        
                pushStyleColor(ElayGraphics::EGUIItemColor::GUIItemColor_PlotLines, (glm::vec4)ImColor::HSV(0.3f, 0.7f, 0.8f));
                plotLines("", curvePlot, 0.0f, 2.0f, plotLinesSize);
                popStyleColor();
        
                pushSliderColors(4.0f / 7.0f);
                vSliderFloat("##curveGamma.b", verticalSliderSize, &colorGrading.gamma.b, 0.0f, 4.0f, "");
                tooltipFloat(colorGrading.gamma.b);
                sameLine();
                vSliderFloat("##curveMid.b", verticalSliderSize, &colorGrading.midPoint.b, 0.0f, 2.0f, "");
                tooltipFloat(colorGrading.midPoint.b);
                sameLine();
                vSliderFloat("##curveScale.b", verticalSliderSize, &colorGrading.scale.b, 0.0f, 4.0f, "");
                tooltipFloat(colorGrading.scale.b);
                sameLine(0.0f, 18.0f);
                popSliderColors();
        
                pushStyleColor(ElayGraphics::EGUIItemColor::GUIItemColor_PlotLines, (glm::vec4)ImColor::HSV(0.6f, 0.7f, 0.8f));
                plotLines("", curvePlot, 0.0f, 2.0f, plotLinesSize);
                popStyleColor();
            }
            else 
            {
                vSliderFloat("##curveGamma", verticalSliderSize, &colorGrading.gamma.r, 0.0f, 4.0f, "");
                tooltipFloat(colorGrading.gamma.r);
                sameLine();
                vSliderFloat("##curveMid", verticalSliderSize, &colorGrading.midPoint.r, 0.0f, 2.0f, "");
                tooltipFloat(colorGrading.midPoint.r);
                sameLine();
                vSliderFloat("##curveScale", verticalSliderSize, &colorGrading.scale.r, 0.0f, 4.0f, "");
                tooltipFloat(colorGrading.scale.r);
                sameLine(0.0f, 18.0f);
        
                colorGrading.gamma = glm::vec3{ colorGrading.gamma.r };
                colorGrading.midPoint = glm::vec3{ colorGrading.midPoint.r };
                colorGrading.scale = glm::vec3{ colorGrading.scale.r };
        
                pushStyleColor(ElayGraphics::EGUIItemColor::GUIItemColor_PlotLines, (glm::vec4)ImColor::HSV(0.17f, 0.21f, 0.9f));
                plotLines("", curvePlot, 0.0f, 2.0f, plotLinesSize);
                popStyleColor();
            }
        }
        unIndent();
    }
}

void CCustomGUI::initV()
{
    ElayGraphics::ResourceManager::registerSharedData("MaterialSettings", material);
    ElayGraphics::ResourceManager::registerSharedData("LightSettings", light);
    ElayGraphics::ResourceManager::registerSharedData("CameraSetting", cameraSetting);
    ElayGraphics::ResourceManager::registerSharedData("ColorGradingSetting", colorGradingSetting);
}

void CCustomGUI::updateV()
{
    if (button("Open Model File"))
    {
        TCHAR szBuffer[MAX_PATH] = { 0 };
        OPENFILENAME ofn = { 0 };
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFilter = _T("Obj文件(*.obj)\0*.obj\0所有文件(*.*)\0*.*\0");//要选择的文件后缀   
        ofn.lpstrInitialDir = _T("D:\\Program Files");//默认的文件路径   
        ofn.lpstrFile = szBuffer;//存放文件的缓冲区   
        ofn.nMaxFile = sizeof(szBuffer) / sizeof(*szBuffer);
        ofn.nFilterIndex = 0;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;//标志如果是多选要加上OFN_ALLOWMULTISELECT  
        BOOL bSel = GetOpenFileName(&ofn);

        std::string objFileName = TCHAR2STRING(szBuffer);
        
    }
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
        std::vector<std::string> chooseType = { "Standard", "Cloth", "SubSurface"};
        combo("Type##materialType", material.materialType, chooseType);

        if (material.materialType == 0)
        {
            indent();           
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
            directionWidget("anisotropyDirection", &material.anisotropyDirection[0]);
            sliderFloat("anisotropy", &material.anisotropy, 0.0f, 1.0f);
        }
        else if (material.materialType == 1)
        {
            indent();
            colorEdit("baseColor", material.baseColor);
            sliderFloat("roughness", &material.roughness, 0.0f, 1.0f);
            sliderFloat("ambientOcclusion", &material.ambientOcclusion, 0.0f, 1.0f);
            colorEdit("emissive", material.emissive);
            colorEdit("sheenColor", material.sheenColor);
            colorEdit("subsurfaceColor", material.subSurfaceColor);

            sliderFloat("clearCoat", &material.clearCoat, 0.0f, 1.0f);
            sliderFloat("clearCoatRoughness", &material.clearCoatRoughness, 0.0f, 1.0f);
            directionWidget("anisotropyDirection", &material.anisotropyDirection[0]);
            sliderFloat("anisotropy", &material.anisotropy, 0.0f, 1.0f);
        }
        else if (material.materialType == 2)
        {

            indent();
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
            colorEdit("subsurfaceColor", material.subSurfaceColor);
            sliderFloat("thickness", &material.thickness, 0.0f, 1.0f);
            sliderFloat("subsurfacePower", &material.subsurfacePower, 0.0f, 1.0f);

            directionWidget("anisotropyDirection", &material.anisotropyDirection[0]);
            sliderFloat("anisotropy", &material.anisotropy, 0.0f, 1.0f);


        }
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

    colorGradingUI(colorGradingSetting, mRangePlot, mCurvePlot, mToneMapPlot);

    ElayGraphics::ResourceManager::updateSharedDataByName("MaterialSettings", material);
    ElayGraphics::ResourceManager::updateSharedDataByName("LightSettings", light);
    ElayGraphics::ResourceManager::updateSharedDataByName("CameraSetting", cameraSetting);
    ElayGraphics::ResourceManager::updateSharedDataByName("ColorGradingSetting", colorGradingSetting);
}
