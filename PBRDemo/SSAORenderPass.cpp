#include "SSAORenderPass.h"
#include "Shader.h"
#include "Interface.h"
#include "Common.h"
#include "Utils.h"
#include "ModelLoad.h"
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#include "Color.h"
#include "Exposure.h"
#include "CustomGUI.h"
#include "AABB.h"


struct AmbientOcclusionOptions 
{
	float radius = 0.3f;    //!< Ambient Occlusion radius in meters, between 0 and ~10.
	float power = 1.0f;     //!< Controls ambient occlusion's contrast. Must be positive.
	float bias = 0.0005f;   //!< Self-occlusion bias in meters. Use to avoid self-occlusion. Between 0 and a few mm.
	float resolution = 0.5f;//!< How each dimension of the AO buffer is scaled. Must be either 0.5 or 1.0.
	float intensity = 1.0f; //!< Strength of the Ambient Occlusion effect.
	float bilateralThreshold = 0.05f; //!< depth distance that constitute an edge for filtering
	bool enabled = false;    //!< enables or disables screen-space ambient occlusion
	bool bentNormals = false; //!< enables bent normals computation from AO, and specular AO
	float minHorizonAngleRad = 0.0f;  //!< min angle in radian to consider
};


CSSAORenderPass::CSSAORenderPass(const std::string& vPassName, int vExcutionOrder) :IRenderPass(vPassName, vExcutionOrder)
{
}

CSSAORenderPass::~CSSAORenderPass()
{
}

void CSSAORenderPass::initV()
{
	auto TextureConfig4Depth = std::make_shared<ElayGraphics::STexture>();
	TextureConfig4Depth->Width = 1280;
	TextureConfig4Depth->Height = 760;
	TextureConfig4Depth->Type4WrapS = GL_CLAMP_TO_BORDER;
	TextureConfig4Depth->Type4WrapT = GL_CLAMP_TO_BORDER;
	TextureConfig4Depth->BorderColor = { 1.0, 1.0, 1.0, 1.0 };
	TextureConfig4Depth->InternalFormat = GL_DEPTH_COMPONENT32F;
	TextureConfig4Depth->ExternalFormat = GL_DEPTH_COMPONENT;
	TextureConfig4Depth->DataType = GL_FLOAT;
	TextureConfig4Depth->Type4MinFilter = GL_LINEAR;
	TextureConfig4Depth->Type4MagFilter = GL_LINEAR;
	TextureConfig4Depth->TextureAttachmentType = ElayGraphics::STexture::ETextureAttachmentType::DepthTexture;
	genTexture(TextureConfig4Depth);
	depthFBO = genFBO({ TextureConfig4Depth });
	ElayGraphics::ResourceManager::registerSharedData("SSAODepthTexture", TextureConfig4Depth);
	genGenerateMipmap(TextureConfig4Depth);


	auto TextureSSAO = std::make_shared<ElayGraphics::STexture>();
	TextureSSAO->Width = 1280;
	TextureSSAO->Height = 760;
	TextureSSAO->Type4WrapS = GL_CLAMP_TO_EDGE;
	TextureSSAO->Type4WrapT = GL_CLAMP_TO_EDGE;
	TextureSSAO->InternalFormat = GL_RGB16F;
	TextureSSAO->ExternalFormat = GL_RGB;
	TextureSSAO->DataType = GL_UNSIGNED_BYTE;
	TextureSSAO->Type4MinFilter = GL_NEAREST;
	TextureSSAO->Type4MagFilter = GL_NEAREST;
	genTexture(TextureSSAO);
	
	ssaoFBO = genFBO({ TextureSSAO });
	ElayGraphics::ResourceManager::registerSharedData("SSAOTexture", TextureSSAO);

	depthShader = std::make_shared<CShader>("Depth_VS.glsl", "Depth_FS.glsl");
	ssaoShader = std::make_shared<CShader>("SSAO_VS.glsl", "SSAO_FS.glsl");



}

void CSSAORenderPass::updateV()
{

	auto depthmap = ElayGraphics::ResourceManager::getSharedDataByName<std::shared_ptr<ElayGraphics::STexture>>("SSAODepthTexture");

	glViewport(0, 0, 1280, 760);
	glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	depthShader->activeShader();
	depthShader->setMat4UniformValue("u_ModelMatrix", glm::value_ptr(glm::mat4(1)));

	auto m_pMonkey = std::dynamic_pointer_cast<CModelLoad>(ElayGraphics::ResourceManager::getGameObjectByName("Monkey"));
	m_pMonkey->updateModel(*depthShader);
	genGenerateMipmap(depthmap);



	float sampleCount{};
	float spiralTurns{};
	float standardDeviation{};
	sampleCount = 16.0f;
	spiralTurns = 7.0f;
	standardDeviation = 6.0;


	//float kernelSize = 23;
	//float standardDeviation = standardDeviation;
	//float scale = 1.0f;

	glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	ssaoShader->activeShader();
	ssaoShader->setTextureUniformValue("materialParams_depth", depthmap);
	
	double width = 1280;
	double height = 760;
	int levelCount = 5;
	AmbientOcclusionOptions options;

	glm::mat4 projection = ElayGraphics::Camera::getMainCameraProjectionMatrix();
	float zfar = ElayGraphics::Camera::getMainCameraFar();
	float znear = ElayGraphics::Camera::getMainCameraNear();

	const float projectionScale = std::min(
		0.5f * projection[0].x * width,
		0.5f * projection[1].y * height);

	//glm::mat4 transprojection = glm::transpose(projection);

	const float peak = 0.1f * options.radius;
	const float intensity = (glm::TAU * peak) * options.intensity;
	// always square AO result, as it looks much better
	const float power = options.power * 2.0f;

	const auto invProjection = glm::inverse(projection);
	const float inc = (1.0f / (sampleCount - 0.5f)) * spiralTurns * glm::TAU;

	float x = invProjection[0][0];

	ssaoShader->setFloatUniformValue("materialParams_resolution", width, height, 1.0f / width, 1.0f / height );
	ssaoShader->setFloatUniformValue("materialParams_invRadiusSquared", 1.0f / (options.radius * options.radius));
	ssaoShader->setFloatUniformValue("materialParams_minHorizonAngleSineSquared", std::pow(std::sin(options.minHorizonAngleRad), 2.0f));
	ssaoShader->setFloatUniformValue("materialParams_projectionScale", projectionScale);
	ssaoShader->setFloatUniformValue("materialParams_projectionScaleRadius", projectionScale * options.radius);
	ssaoShader->setFloatUniformValue("materialParams_positionParams", invProjection[0][0] *2.0f, invProjection[1][1]  *2.0f);
	ssaoShader->setFloatUniformValue("materialParams_peak2", peak * peak);
	ssaoShader->setFloatUniformValue("materialParams_bias", options.bias);
	ssaoShader->setFloatUniformValue("materialParams_power", power);
	ssaoShader->setFloatUniformValue("materialParams_intensity", intensity / sampleCount);
	ssaoShader->setFloatUniformValue("materialParams_maxLevel", uint32_t(levelCount - 1));
	ssaoShader->setFloatUniformValue("materialParams_sampleCount",  sampleCount, 1.0f / (sampleCount - 0.5f) );
	ssaoShader->setFloatUniformValue("materialParams_spiralTurns", spiralTurns);
	ssaoShader->setFloatUniformValue("materialParams_angleIncCosSin", std::cos(inc), std::sin(inc));
	ssaoShader->setFloatUniformValue("materialParams_invFarPlane", 1.0f / -zfar);
	ssaoShader->setFloatUniformValue("far_plane", zfar);
	ssaoShader->setFloatUniformValue("near_plane", znear);
	drawQuad();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);




}
