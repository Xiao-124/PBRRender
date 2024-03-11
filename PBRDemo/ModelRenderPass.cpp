#include "ModelRenderPass.h"
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
#include "GroundObject.h"
CModelRenderPass::CModelRenderPass(const std::string& vPassName, int vExcutionOrder) : IRenderPass(vPassName, vExcutionOrder)
{

}

CModelRenderPass::~CModelRenderPass()
{

}


void CModelRenderPass::initV()
{
	m_FBO = ElayGraphics::ResourceManager::getSharedDataByName<GLuint>("mFBO");
	m_pShader = std::make_shared<CShader>("ModelRender_VS.glsl", "ModelRender_FS.glsl");
	m_pMonkey = std::dynamic_pointer_cast<CModelLoad>(ElayGraphics::ResourceManager::getGameObjectByName("Monkey"));

	m_pShader->activeShader();
	//app.transform = mat4f{ mat3f(1), float3(0, 0, -4) } *tcm.getWorldTransform(ti);

	glm::mat4 modelMatrix(1);
	//modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, -4.0f));
	m_pShader->setMat4UniformValue("u_ModelMatrix", glm::value_ptr(modelMatrix));
	m_pMonkey->initModel(*m_pShader);
	glm::vec3 center = m_pMonkey->getAABB()->getCentre();
	ElayGraphics::Camera::setMainCameraFocalPos(glm::dvec3(center.x, center.y, center.z));

	
	auto groundShader = std::make_shared<CShader>("GroundShadow_VS.glsl", "GroundShadow_FS.glsl");
	ElayGraphics::ResourceManager::registerSharedData("GroundShader", groundShader);

}

void CModelRenderPass::updateV()
{

	auto albeo = ElayGraphics::ResourceManager::getSharedDataByName<std::shared_ptr<ElayGraphics::STexture>>("TextureConfig4Albedo");
	glViewport( 0, 0,  albeo->Width, albeo->Height);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	//glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	m_pShader->activeShader();
	glm::dvec3 cameraPos = ElayGraphics::Camera::getMainCameraPos();
	m_pShader->setFloatUniformValue("cameraPos",  cameraPos.x, cameraPos.y, cameraPos.z);

	
	modelMatrix *= ElayGraphics::Camera::getMainCameraModelMatrix();
	m_pShader->setMat4UniformValue("u_ModelMatrix", glm::value_ptr(modelMatrix));

	auto material = ElayGraphics::ResourceManager::getSharedDataByName<MaterialSettings>("MaterialSettings");
	auto light = ElayGraphics::ResourceManager::getSharedDataByName<LightSettings>("LightSettings");
	auto cameraSet = ElayGraphics::ResourceManager::getSharedDataByName<CameraSetting>("CameraSetting");

	float mAperture = cameraSet.cameraAperture;
	float mShutterSpeed = 1.0f / cameraSet.cameraSpeed;
	float mSensitivity = cameraSet.cameraISO;
	float exposure = Exposure::exposure(mAperture, mShutterSpeed, mSensitivity);



	glm::vec3 lightDir = light.sunLight.sunlightDirection;
	lightDir = glm::normalize(lightDir);
	lightDir = -lightDir;

	float sunAngularRadius = light.sunLight.sunlightAngularRadius * glm::DEG_TO_RAD;
	float radius = sunAngularRadius;
	float haloSize = light.sunLight.sunlightHaloSize;
	float haloFalloff = light.sunLight.sunlightHaloFalloff;

	glm::vec4 sun;
	sun.x = std::cos(radius);
	sun.y = std::sin(radius);
	sun.z = 1.0f / (std::cos(radius * haloSize) - sun.x);
	sun.w = haloFalloff;

	glm::vec3 lightColor = light.sunLight.sunlightColor;
	float lightIdentity = light.sunLight.sunlightIntensity * exposure;
	m_pShader->setFloatUniformValue("lightDirection", lightDir.x, lightDir.y, lightDir.z);
	m_pShader->setFloatUniformValue("sun", sun.x, sun.y, sun.z, sun.w);
	m_pShader->setFloatUniformValue("lightColor", lightColor.x, lightColor.y, lightColor.z, lightIdentity);
	
	float iblIntensity = light.iblIntensity;
	iblIntensity *= exposure;
	m_pShader->setFloatUniformValue("iblLuminance", iblIntensity);

	LinearColorA mcolor = Color::toLinear<ACCURATE>(sRGBColorA(material.baseColor.r, material.baseColor.g, material.baseColor.b, material.baseColor.a));
	m_pShader->setFloatUniformValue("baseColor", mcolor.r, mcolor.g, mcolor.b, mcolor.a);
	m_pShader->setFloatUniformValue("metallic", material.metallic);
	m_pShader->setFloatUniformValue("roughness", material.roughness);
	m_pShader->setFloatUniformValue("reflectance", material.reflectance);
	m_pShader->setFloatUniformValue("emissive", material.emissive.r, material.emissive.g, material.emissive.b, material.emissive.a);
	m_pShader->setFloatUniformValue("ambientOcclusion", material.ambientOcclusion);
	
	
	auto irradianceMap = ElayGraphics::ResourceManager::getSharedDataByName<std::shared_ptr<ElayGraphics::STexture>>("irradianceMap");
	auto prefilterMap = ElayGraphics::ResourceManager::getSharedDataByName<std::shared_ptr<ElayGraphics::STexture>>("prefilterMap");
	auto brdfLUT = ElayGraphics::ResourceManager::getSharedDataByName<std::shared_ptr<ElayGraphics::STexture>>("brdfLUTTexture");
	auto envCubemap = ElayGraphics::ResourceManager::getSharedDataByName<std::shared_ptr<ElayGraphics::STexture>>("envCubemap");
	m_pShader->setTextureUniformValue("irradianceMap", irradianceMap);
	m_pShader->setTextureUniformValue("prefilterMap", prefilterMap);
	m_pShader->setTextureUniformValue("brdfLUT", brdfLUT);
	m_pShader->setTextureUniformValue("environmentCubeMap", envCubemap);

	std::vector<glm::vec3> frame_iblSH = ElayGraphics::ResourceManager::getSharedDataByName<std::vector<glm::vec3>>("iblSH");
	//frame_iblSH[0] = glm::vec3(0.7857);
	//frame_iblSH[1] = glm::vec3(0.4025);
	//frame_iblSH[2] = glm::vec3(0.4605);
	//frame_iblSH[3] = glm::vec3(0.08418);
	//frame_iblSH[4] = glm::vec3(0.05834);
	//frame_iblSH[5] = glm::vec3(0.2049);
	//frame_iblSH[6] = glm::vec3(0.09273);
	//frame_iblSH[7] = glm::vec3(-0.0918);
	//frame_iblSH[8] = glm::vec3(-0.0067);	
	for (int i = 0; i < 9; i++)
	{
		std::string name = "frame_iblSH[" + std::to_string(i) + "]";
		m_pShader->setFloatUniformValue(name.c_str(), frame_iblSH[i].x, frame_iblSH[i].y, frame_iblSH[i].z);
		//m_pShader->setFloatUniformValue(name.c_str(), 0, 0, 0);
	}
	m_pMonkey->updateModel(*m_pShader);
	

	glDisable(GL_CULL_FACE);
	glm::vec3 center = m_pMonkey->getAABB()->getCentre();
	auto lightPosition = ElayGraphics::ResourceManager::getSharedDataByName<glm::vec3>("u_LightPos");

	//auto groundShader = std::make_shared<CShader>("GroundShadow_VS.glsl", "GroundShadow_FS.glsl");
	auto groundShader = ElayGraphics::ResourceManager::getSharedDataByName<std::shared_ptr<CShader>>("GroundShader");
	auto TextureConfig4Depth = ElayGraphics::ResourceManager::getSharedDataByName<std::shared_ptr<ElayGraphics::STexture>>("LightDepthTexture");
	groundShader->activeShader();
	groundShader->setTextureUniformValue("shadowMap", TextureConfig4Depth);
	groundShader->setFloatUniformValue("lightPos", lightPosition.x, lightPosition.y, lightPosition.z);
	groundShader->setFloatUniformValue("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
	glm::mat4 modelMatrix(1);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0, -0.5, 0));
	//modelMatrix = glm::scale(modelMatrix, glm::vec3(100, 0, 100));

	groundShader->setMat4UniformValue("u_ModelMatrix", glm::value_ptr(modelMatrix));
	auto u_LightVPMatrix = ElayGraphics::ResourceManager::getSharedDataByName<glm::mat4>("u_LightVPMatrix");
	groundShader->setMat4UniformValue("u_LightVPMatrix", glm::value_ptr(u_LightVPMatrix));

	auto m_GroundObject = std::dynamic_pointer_cast<CGroundObject>(ElayGraphics::ResourceManager::getGameObjectByName("GroundObject"));
	glBindVertexArray(m_GroundObject->getVAO());
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);


	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}