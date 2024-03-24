#include "EpipolarLightScatteringPass.h"
#include <stb_image.h>
#include <Interface.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Utils.h"
#include "Shader.h"
#include "Common.h"
#include <Basetsd.h>

using float4 = glm::vec4;
#define EARTH_RADIUS 6371000.0


struct AirScatteringAttribs
{
    // Angular Rayleigh scattering coefficient contains all the terms exepting 1 + cos^2(Theta):
    // Pi^2 * (n^2-1)^2 / (2*N) * (6+3*Pn)/(6-7*Pn)
    float4 f4AngularRayleighSctrCoeff;
    // Total Rayleigh scattering coefficient is the integral of angular scattering coefficient in all directions
    // and is the following:
    // 8 * Pi^3 * (n^2-1)^2 / (3*N) * (6+3*Pn)/(6-7*Pn)
    float4 f4TotalRayleighSctrCoeff;
    float4 f4RayleighExtinctionCoeff;

    // Note that angular scattering coefficient is essentially a phase function multiplied by the
    // total scattering coefficient
    float4 f4AngularMieSctrCoeff;
    float4 f4TotalMieSctrCoeff;
    float4 f4MieExtinctionCoeff;

    float4 f4TotalExtinctionCoeff;
    // Cornette-Shanks phase function (see Nishita et al. 93) normalized to unity has the following form:
    // F(theta) = 1/(4*PI) * 3*(1-g^2) / (2*(2+g^2)) * (1+cos^2(theta)) / (1 + g^2 - 2g*cos(theta))^(3/2)
    float4 f4CS_g; // x == 3*(1-g^2) / (2*(2+g^2))
    // y == 1 + g^2
    // z == -2*g

// Earth parameters can't be changed at run time
    float fEarthRadius              =    (static_cast<float>(EARTH_RADIUS));
    float fAtmBottomAltitude        =    (0.f);     // Altitude of the bottom atmosphere boundary (sea level by default)
    float fAtmTopAltitude           =    (80000.f); // Altitude of the top atmosphere boundary, 80 km by default
    float fTurbidity                =    (1.02f);
                                       
    float fAtmBottomRadius          =    (fEarthRadius + fAtmBottomAltitude);
    float fAtmTopRadius             =    (fEarthRadius + fAtmTopAltitude);
    float fAtmAltitudeRangeInv      =    (1.f / (fAtmTopAltitude - fAtmBottomAltitude));
    float fAerosolPhaseFuncG        =    (0.76f);
                                       
    float4 f4ParticleScaleHeight    =    (float4(7994.f, 1200.f, 1.f / 7994.f, 1.f / 1200.f));
};

EpipolarLightScattering::EpipolarLightScattering(const std::string& vPassName, int vExcutionOrder):IRenderPass(vPassName, vExcutionOrder)
{



}

EpipolarLightScattering::~EpipolarLightScattering()
{
}

//void EpipolarLightScattering::CreateRandomSphereSamplingTexture()
//{
//    TextureDesc RandomSphereSamplingTexDesc;
//    RandomSphereSamplingTexDesc.Type = RESOURCE_DIM_TEX_2D;
//    RandomSphereSamplingTexDesc.Width = m_uiNumRandomSamplesOnSphere;
//    RandomSphereSamplingTexDesc.Height = 1;
//    RandomSphereSamplingTexDesc.MipLevels = 1;
//    RandomSphereSamplingTexDesc.Format = TEX_FORMAT_RGBA32_FLOAT;
//    RandomSphereSamplingTexDesc.Usage = USAGE_IMMUTABLE;
//    RandomSphereSamplingTexDesc.BindFlags = BIND_SHADER_RESOURCE;
//
//    std::vector<float4> SphereSampling(m_uiNumRandomSamplesOnSphere);
//    for (Uint32 iSample = 0; iSample < m_uiNumRandomSamplesOnSphere; ++iSample)
//    {
//        float4& f4Sample = SphereSampling[iSample];
//        f4Sample.z = ((float)rand() / (float)RAND_MAX) * 2.f - 1.f;
//        float t = ((float)rand() / (float)RAND_MAX) * 2.f * PI_F;
//        float r = sqrt(std::max(1.f - f4Sample.z * f4Sample.z, 0.f));
//        f4Sample.x = r * cos(t);
//        f4Sample.y = r * sin(t);
//        f4Sample.w = 0;
//    }
//    TextureSubResData Mip0Data;
//    Mip0Data.pData = SphereSampling.data();
//    Mip0Data.Stride = m_uiNumRandomSamplesOnSphere * sizeof(float4);
//
//    TextureData TexData;
//    TexData.NumSubresources = 1;
//    TexData.pSubResources = &Mip0Data;
//
//    RefCntAutoPtr<ITexture> ptex2DSphereRandomSampling;
//    pDevice->CreateTexture(RandomSphereSamplingTexDesc, &TexData, &ptex2DSphereRandomSampling);
//    m_ptex2DSphereRandomSamplingSRV = ptex2DSphereRandomSampling->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
//    m_ptex2DSphereRandomSamplingSRV->SetSampler(m_pLinearClampSampler);
//    m_pResMapping->AddResource("g_tex2DSphereRandomSampling", m_ptex2DSphereRandomSamplingSRV, true);
//}


void EpipolarLightScattering::PrecomputeScatteringLUT()
{
   

    auto ptex3DSingleSctr = std::make_shared<ElayGraphics::STexture>();
    ptex3DSingleSctr->TextureType = ElayGraphics::STexture::ETextureType::Texture3D;
    ptex3DSingleSctr->InternalFormat = GL_RGBA32F;
    ptex3DSingleSctr->ExternalFormat = GL_RGBA;
    ptex3DSingleSctr->DataType = GL_FLOAT;
    ptex3DSingleSctr->Width = 32;
    ptex3DSingleSctr->Height = 128;
    ptex3DSingleSctr->Depth = 1024;
    genTexture(ptex3DSingleSctr);

    auto m_ptex3DHighOrderSctr = std::make_shared<ElayGraphics::STexture>();
    m_ptex3DHighOrderSctr->TextureType = ElayGraphics::STexture::ETextureType::Texture3D;
    m_ptex3DHighOrderSctr->InternalFormat = GL_RGBA32F;
    m_ptex3DHighOrderSctr->ExternalFormat = GL_RGBA;
    m_ptex3DHighOrderSctr->DataType = GL_FLOAT;
    m_ptex3DHighOrderSctr->Width = 32;
    m_ptex3DHighOrderSctr->Height = 128;
    m_ptex3DHighOrderSctr->Depth = 1024;
    genTexture(m_ptex3DHighOrderSctr);


    auto m_ptex3DHighOrderSctr2 = std::make_shared<ElayGraphics::STexture>();
    m_ptex3DHighOrderSctr2->TextureType = ElayGraphics::STexture::ETextureType::Texture3D;
    m_ptex3DHighOrderSctr2->InternalFormat = GL_RGBA32F;
    m_ptex3DHighOrderSctr2->ExternalFormat = GL_RGBA;
    m_ptex3DHighOrderSctr2->DataType = GL_FLOAT;
    m_ptex3DHighOrderSctr2->Width = 32;
    m_ptex3DHighOrderSctr2->Height = 128;
    m_ptex3DHighOrderSctr2->Depth = 1024;
    genTexture(m_ptex3DHighOrderSctr2);


    
    auto ptex3DMultipleSctr = std::make_shared<ElayGraphics::STexture>();
    ptex3DMultipleSctr->TextureType = ElayGraphics::STexture::ETextureType::Texture3D;
    ptex3DMultipleSctr->InternalFormat = GL_RGBA32F;
    ptex3DMultipleSctr->ExternalFormat = GL_RGBA;
    ptex3DMultipleSctr->DataType = GL_FLOAT;
    ptex3DMultipleSctr->Width = 32;
    ptex3DMultipleSctr->Height = 128;
    ptex3DMultipleSctr->Depth = 1024;
    genTexture(ptex3DMultipleSctr);


    auto ptex3DSctrRadiance = std::make_shared<ElayGraphics::STexture>();
    ptex3DSctrRadiance->TextureType = ElayGraphics::STexture::ETextureType::Texture3D;
    ptex3DSctrRadiance->InternalFormat = GL_RGBA32F;
    ptex3DSctrRadiance->ExternalFormat = GL_RGBA;
    ptex3DSctrRadiance->DataType = GL_FLOAT;
    ptex3DSctrRadiance->Width = 32;
    ptex3DSctrRadiance->Height = 128;
    ptex3DSctrRadiance->Depth = 1024;
    genTexture(ptex3DSctrRadiance);


    auto ptex3DInsctrOrder = std::make_shared<ElayGraphics::STexture>();
    ptex3DInsctrOrder->TextureType = ElayGraphics::STexture::ETextureType::Texture3D;
    ptex3DInsctrOrder->InternalFormat = GL_RGBA32F;
    ptex3DInsctrOrder->ExternalFormat = GL_RGBA;
    ptex3DInsctrOrder->DataType = GL_FLOAT;
    ptex3DInsctrOrder->Width = 32;
    ptex3DInsctrOrder->Height = 128;
    ptex3DInsctrOrder->Depth = 1024;
    genTexture(ptex3DInsctrOrder);






    auto RandomSphereSamplingTex = std::make_shared<ElayGraphics::STexture>();;
    RandomSphereSamplingTex->TextureType = ElayGraphics::STexture::ETextureType::Texture2D;
    RandomSphereSamplingTex->Width = 128;
    RandomSphereSamplingTex->Height = 1;

    double PI_F = 3.1415926;
    std::vector<float4> SphereSampling(128);
    for (int iSample = 0; iSample < 128; ++iSample)
    {
        float4& f4Sample = SphereSampling[iSample];
        f4Sample.z = ((float)rand() / (float)RAND_MAX) * 2.f - 1.f;
        float t = ((float)rand() / (float)RAND_MAX) * 2.f * PI_F;
        float r = sqrt(std::max(1.f - f4Sample.z * f4Sample.z, 0.f));
        f4Sample.x = r * cos(t);
        f4Sample.y = r * sin(t);
        f4Sample.w = 0;
    }
    RandomSphereSamplingTex->pDataSet.resize(1);
    RandomSphereSamplingTex->pDataSet[0] = SphereSampling.data();
    genTexture(RandomSphereSamplingTex);


    auto pPrecomputeSingleSctrCS = std::make_shared<CShader>("PreComputeSingleScattering_CS.glsl");
    auto pComputeSctrRadianceCS = std::make_shared<CShader>("ComputeSctrRadiance_CS.glsl");
    auto pComputeScatteringOrderCS = std::make_shared<CShader>("ComputeScatteringOrder_CS.glsl");
    auto pInitHighOrderScatteringCS = std::make_shared<CShader>("InitHighOrderScattering_CS.glsl");
    auto pUpdateHighOrderScatteringCS = std::make_shared<CShader>("UpdateHighOrderScattering_CS.glsl");
    auto pCombineScatteringOrdersCS = std::make_shared<CShader>("CombineScatteringOrders_CS.glsl");



    int ThreadGroupSize = 16;

   
    std::vector<int> m_GlobalGroupSize;
    m_GlobalGroupSize.push_back(32 / ThreadGroupSize);
    m_GlobalGroupSize.push_back(128 / ThreadGroupSize);
    m_GlobalGroupSize.push_back(1024);
    

    auto albeo = ElayGraphics::ResourceManager::getSharedDataByName<std::shared_ptr<ElayGraphics::STexture>>("TextureConfig4Albedo");
    
    pPrecomputeSingleSctrCS->activeShader();
    pPrecomputeSingleSctrCS->setTextureUniformValue("g_tex2DOccludedNetDensityToAtmTop", albeo);
    glBindImageTexture(0, ptex3DSingleSctr->TextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    //pPrecomputeSingleSctrCS->setTextureUniformValue("g_rwtex3DSingleScattering", ptex3DSingleSctr);
    glDispatchCompute(m_GlobalGroupSize[0], m_GlobalGroupSize[1], m_GlobalGroupSize[2]);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glFlush();
    

    const int iNumScatteringOrders = 4;
    for (int iSctrOrder = 1; iSctrOrder < iNumScatteringOrders; ++iSctrOrder)
    {
        // Step 1: compute differential in-scattering
       //ComputeSctrRadianceTech.SRB->GetVariableByName(SHADER_TYPE_COMPUTE, "g_tex3DPreviousSctrOrder")->Set((iSctrOrder == 1) ? m_ptex3DSingleScatteringSRV : ptex3DInsctrOrderSRV);
       //ComputeSctrRadianceTech.DispatchCompute(pContext, DispatchAttrs);

        pComputeSctrRadianceCS->activeShader();
        if (iSctrOrder == 1)
        {
            pComputeSctrRadianceCS->setTextureUniformValue("g_tex3DPreviousSctrOrder", ptex3DSingleSctr);
        }  
        else
        {
            pComputeSctrRadianceCS->setTextureUniformValue("g_tex3DPreviousSctrOrder", ptex3DInsctrOrder);
        }
        pComputeSctrRadianceCS->setTextureUniformValue("g_tex2DSphereRandomSampling", RandomSphereSamplingTex);
        glBindImageTexture(0, ptex3DSctrRadiance->TextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        glDispatchCompute(m_GlobalGroupSize[0], m_GlobalGroupSize[1], m_GlobalGroupSize[2]);
        glFlush();


        // Step 2: integrate differential in-scattering
        pComputeScatteringOrderCS->activeShader();
        pComputeScatteringOrderCS->setTextureUniformValue("g_tex3DPointwiseSctrRadiance", ptex3DSctrRadiance);
        glBindImageTexture(0, ptex3DInsctrOrder->TextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);     
        glDispatchCompute(m_GlobalGroupSize[0], m_GlobalGroupSize[1], m_GlobalGroupSize[2]);


        auto pRenderTech = pInitHighOrderScatteringCS;
        // Step 3: accumulate high-order scattering scattering
        if (iSctrOrder == 1)
        {
            pRenderTech = pInitHighOrderScatteringCS;
            pRenderTech->activeShader();
        }
        else
        {
            pRenderTech = pUpdateHighOrderScatteringCS;
            std::swap(m_ptex3DHighOrderSctr, m_ptex3DHighOrderSctr2);
            pRenderTech->activeShader();
            pRenderTech->setTextureUniformValue("g_tex3DHighOrderOrderScattering", m_ptex3DHighOrderSctr2);
            
        }
        
        pRenderTech->setTextureUniformValue("g_tex3DCurrentOrderScattering", ptex3DInsctrOrder);
        glBindImageTexture(0, m_ptex3DHighOrderSctr->TextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        glDispatchCompute(m_GlobalGroupSize[0], m_GlobalGroupSize[1], m_GlobalGroupSize[2]);
        // Flush the command buffer to force execution of compute shaders and avoid device
        // reset on low-end Intel GPUs.
        glFlush();
    }


    //pPrecomputeSingleSctrCS->setTextureUniformValue("g_rwtex3DMultipleSctr", ptex3DMultipleSctr);
    //pPrecomputeSingleSctrCS->setTextureUniformValue("g_tex3DSingleSctrLUT", ptex3DSingleSctr);
    //pPrecomputeSingleSctrCS->setTextureUniformValue("g_tex3DMultipleSctrLUT", ptex3DMultipleSctr);

    pCombineScatteringOrdersCS->activeShader();
    pCombineScatteringOrdersCS->setTextureUniformValue("g_tex3DSingleSctrLUT", ptex3DSingleSctr);
    pCombineScatteringOrdersCS->setTextureUniformValue("g_tex3DHighOrderSctrLUT", m_ptex3DHighOrderSctr);
    glBindImageTexture(0, ptex3DMultipleSctr->TextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glDispatchCompute(m_GlobalGroupSize[0], m_GlobalGroupSize[1], m_GlobalGroupSize[2]);

}





void EpipolarLightScattering::initV()
{

	auto TextureConfig4Albedo = std::make_shared<ElayGraphics::STexture>();
	TextureConfig4Albedo->InternalFormat = GL_RG32F;
	TextureConfig4Albedo->ExternalFormat = GL_RG;
	TextureConfig4Albedo->DataType = GL_FLOAT;
	TextureConfig4Albedo->Width = 1024;
	TextureConfig4Albedo->Height = 1024;
	genTexture(TextureConfig4Albedo);

	m_FBO = genFBO({ TextureConfig4Albedo});
	ElayGraphics::ResourceManager::registerSharedData("TextureConfig4Albedo", TextureConfig4Albedo);
	ElayGraphics::ResourceManager::registerSharedData("mFBO", m_FBO);

    AirScatteringAttribs airScatteringAttribs;
    m_sUBO = genBuffer(GL_SHADER_STORAGE_BUFFER, sizeof(airScatteringAttribs), &airScatteringAttribs, GL_STATIC_DRAW, 0);
	m_pShader = std::make_shared<CShader>("PrecomputeNetDensityToAtmTop_VS.glsl", "PrecomputeNetDensityToAtmTop_FS.glsl");
    
    glViewport(0, 0, TextureConfig4Albedo->Width, TextureConfig4Albedo->Height);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_sUBO);
    m_pShader->activeShader();
    drawQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glFlush();
    
    
    PrecomputeScatteringLUT();


    auto PrecomputeAmbientSkyLightTech = std::make_shared<CShader>("PrecomputeAmbientSkyLight_VS.glsl", "PrecomputeAmbientSkyLight_FS.glsl");

    

}

void EpipolarLightScattering::updateV()
{

	

}
