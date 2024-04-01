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
	//m_pShader = std::make_shared<CShader>("ModelRender_VS.glsl", "ModelRender_FS.glsl");

	standardModelShader = std::make_shared<CShader>("ShadingModelStandard_VS.glsl", "ShadingModelStandard_FS.glsl");
	clothModelShader = std::make_shared<CShader>("ShadingModelCloth_VS.glsl", "ShadingModelCloth_FS.glsl");
	subsurafceModelShader = std::make_shared<CShader>("ShadingModelSubSurface_VS.glsl", "ShadingModelSubSurface_FS.glsl");

	m_pMonkey = std::dynamic_pointer_cast<CModelLoad>(ElayGraphics::ResourceManager::getGameObjectByName("Monkey"));
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

	glm::dvec3 cameraPos = ElayGraphics::Camera::getMainCameraPos();
	float iblIntensity = light.iblIntensity;
	iblIntensity *= exposure;

	if (material.materialType == 0)
	{
		m_pShader = standardModelShader;
	}
	else if (material.materialType == 1)
	{
		m_pShader = clothModelShader;
	}
	else if (material.materialType == 2)
	{
		m_pShader = subsurafceModelShader;
	}

	m_pShader->activeShader();	
	m_pShader->setFloatUniformValue("cameraPos",  cameraPos.x, cameraPos.y, cameraPos.z);
	m_pShader->setMat4UniformValue("u_ModelMatrix", glm::value_ptr(glm::mat4(1.0f)));

	
	m_pShader->setFloatUniformValue("directLight.lightDirection", lightDir.x, lightDir.y, lightDir.z);
	m_pShader->setFloatUniformValue("directLight.sun", sun.x, sun.y, sun.z, sun.w);
	m_pShader->setFloatUniformValue("directLight.lightColor", lightColor.x, lightColor.y, lightColor.z, lightIdentity);
	

	for (int i = 0; i < light.pointLights.size(); i++)
	{
		m_pShader->setFloatUniformValue("punctualLight[" + std::to_string(i) + "].positionFalloff", 
			light.pointLights[i].pointLightPosition.x, light.pointLights[i].pointLightPosition.y, 
			light.pointLights[i].pointLightPosition.z);

		m_pShader->setFloatUniformValue("punctualLight[" + std::to_string(i) + "].color", light.pointLights[i].pointlightColor.x, 
			light.pointLights[i].pointlightColor.y, 
			light.pointLights[i].pointlightColor.z, light.pointLights[i].pointlightIntensity);
		m_pShader->setIntUniformValue("punctualLight[" + std::to_string(i) + "].type", 0);
	}
	m_pShader->setIntUniformValue("punctualLight_Num", light.pointLights.size());
	
	m_pShader->setFloatUniformValue("iblLuminance", iblIntensity);

	LinearColorA mcolor = Color::toLinear<ACCURATE>(sRGBColorA(material.baseColor.r, material.baseColor.g, material.baseColor.b, material.baseColor.a));
	m_pShader->setFloatUniformValue("material_baseColor", mcolor.r, mcolor.g, mcolor.b, mcolor.a);
	m_pShader->setFloatUniformValue("material_metallic", material.metallic);
	m_pShader->setFloatUniformValue("material_roughness", material.roughness);
	m_pShader->setFloatUniformValue("material_reflectance", material.reflectance);
	m_pShader->setFloatUniformValue("material_emissive", material.emissive.r, material.emissive.g, material.emissive.b, material.emissive.a);
	m_pShader->setFloatUniformValue("material_ambientOcclusion", material.ambientOcclusion);
	
	m_pShader->setFloatUniformValue("material_sheenColor", material.sheenColor.r, material.sheenColor.g, material.sheenColor.b);
	m_pShader->setFloatUniformValue("material_sheenRoughness", material.sheenRoughness);
	m_pShader->setFloatUniformValue("material_clearCoat", material.clearCoat);
	m_pShader->setFloatUniformValue("material_clearCoatRoughness", material.clearCoatRoughness);
	m_pShader->setFloatUniformValue("material_anisotropyDirection", material.anisotropyDirection.x, material.anisotropyDirection.y, material.anisotropyDirection.z);
	m_pShader->setFloatUniformValue("material_anisotropy", material.anisotropy);
	m_pShader->setFloatUniformValue("material_subsurfaceColor", material.subSurfaceColor.r, material.subSurfaceColor.g, material.subSurfaceColor.b);
	
	m_pShader->setFloatUniformValue("material_thickness", material.thickness);
	m_pShader->setFloatUniformValue("material_subsurfacePower", material.subsurfacePower);


	glm::mat4 u_ProjectionMatrix = ElayGraphics::Camera::getMainCameraProjectionMatrix();
	glm::mat4 u_ViewMatrix  = ElayGraphics::Camera::getMainCameraViewMatrix();
	static int pre_frameId = -1;
	int frameId = (pre_frameId + 1) % 8;
	pre_frameId++;
	const glm::vec2 Halton_2_3[8] =
	{
		glm::vec2(0.0f, -1.0f / 3.0f),
		glm::vec2(-1.0f / 2.0f, 1.0f / 3.0f),
		glm::vec2(1.0f / 2.0f, -7.0f / 9.0f),
		glm::vec2(-3.0f / 4.0f, -1.0f / 9.0f),
		glm::vec2(1.0f / 4.0f, 5.0f / 9.0f),
		glm::vec2(-1.0f / 4.0f, -5.0f / 9.0f),
		glm::vec2(3.0f / 4.0f, 1.0f / 9.0f),
		glm::vec2(-7.0f / 8.0f, 7.0f / 9.0f)
	};


	float deltaWidth = 1.0 / albeo->Width, deltaHeight = 1.0 / albeo->Height;
	glm::vec2 jitter = glm::vec2(
		Halton_2_3[frameId].x * deltaWidth,
		Halton_2_3[frameId].y * deltaHeight
	);

	//jitter * (2.0f / vec2{ svp.width, svp.height }
	glm::mat4 jitterMat = u_ProjectionMatrix;
	jitterMat[2][0] += jitter.x;
	jitterMat[2][1] += jitter.y;


	m_pShader->setMat4UniformValue("u_ProjectionMatrix", glm::value_ptr(jitterMat));
	m_pShader->setMat4UniformValue("u_ViewMatrix", glm::value_ptr(u_ViewMatrix));
	
	auto irradianceMap = ElayGraphics::ResourceManager::getSharedDataByName<std::shared_ptr<ElayGraphics::STexture>>("irradianceMap");
	auto prefilterMap = ElayGraphics::ResourceManager::getSharedDataByName<std::shared_ptr<ElayGraphics::STexture>>("prefilterMap");
	auto brdfLUT = ElayGraphics::ResourceManager::getSharedDataByName<std::shared_ptr<ElayGraphics::STexture>>("brdfLUTTexture");
	auto envCubemap = ElayGraphics::ResourceManager::getSharedDataByName<std::shared_ptr<ElayGraphics::STexture>>("envCubemap");
	m_pShader->setTextureUniformValue("irradianceMap", irradianceMap);
	m_pShader->setTextureUniformValue("prefilterMap", prefilterMap);
	m_pShader->setTextureUniformValue("brdfLUT", brdfLUT);
	m_pShader->setTextureUniformValue("environmentCubeMap", envCubemap);

	std::vector<glm::vec3> frame_iblSH = ElayGraphics::ResourceManager::getSharedDataByName<std::vector<glm::vec3>>("iblSH");
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