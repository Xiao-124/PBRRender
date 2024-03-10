#include "ShadowMapPass.h"
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
CShadowMapPass::CShadowMapPass(const std::string& vPassName, int vExcutionOrder):IRenderPass(vPassName, vExcutionOrder)
{
}

CShadowMapPass::~CShadowMapPass()
{

}

void CShadowMapPass::initV()
{
	m_pShader = std::make_shared<CShader>("ShadowMap_VS.glsl", "ShadowMap_FS.glsl");
	auto  Monkey = std::dynamic_pointer_cast<CModelLoad>(ElayGraphics::ResourceManager::getGameObjectByName("Monkey"));
	

	//glGenTextures(1, &m_FBO);
	//auto TextureConfig4Depth = std::make_shared<ElayGraphics::STexture>();
	//TextureConfig4Depth->isMipmap = true;
	//TextureConfig4Depth->InternalFormat = GL_DEPTH_COMPONENT;
	//TextureConfig4Depth->ExternalFormat = GL_DEPTH_COMPONENT;
	//TextureConfig4Depth->DataType = GL_FLOAT;
	//
	//TextureConfig4Depth->Width = 1024;
	//TextureConfig4Depth->Height = 1024;
	//TextureConfig4Depth->Type4WrapS = GL_CLAMP_TO_BORDER;
	//TextureConfig4Depth->Type4WrapT = GL_CLAMP_TO_BORDER;
	//TextureConfig4Depth->BorderColor = { 1.0, 1.0, 1.0, 1.0 };
	//genTexture(TextureConfig4Depth);
	//
	//
	////m_FBO = genFBO({ TextureConfig4Depth });
	//int id = TextureConfig4Depth->TextureID;
	//glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, TextureConfig4Depth->TextureID, 0);
	//if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	//{
	//	std::cerr << "Error::FBO:: Framebuffer Is Not Complete." << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
	//}
	////glDrawBuffer(GL_NONE);
	////glReadBuffer(GL_NONE);
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	auto TextureConfig4Depth = std::make_shared<ElayGraphics::STexture>();
	//TextureConfig4Depth->isMipmap = true;
	//TextureConfig4Depth->InternalFormat = GL_RG32F;
	//TextureConfig4Depth->ExternalFormat = GL_RG;
	//TextureConfig4Depth->DataType = GL_FLOAT;
	TextureConfig4Depth->Width = 1024;
	TextureConfig4Depth->Height = 1024;
	TextureConfig4Depth->Type4WrapS = GL_CLAMP_TO_BORDER;
	TextureConfig4Depth->Type4WrapT = GL_CLAMP_TO_BORDER;
	TextureConfig4Depth->BorderColor = { 1.0, 1.0, 1.0, 1.0 };


	TextureConfig4Depth->InternalFormat = GL_DEPTH_COMPONENT32F;
	TextureConfig4Depth->ExternalFormat = GL_DEPTH_COMPONENT;
	TextureConfig4Depth->DataType = GL_FLOAT;
	TextureConfig4Depth->Type4MinFilter = GL_NEAREST;
	TextureConfig4Depth->Type4MagFilter = GL_NEAREST;
	TextureConfig4Depth->TextureAttachmentType = ElayGraphics::STexture::ETextureAttachmentType::DepthTexture;
	//genTexture(TextureConfig4Depth);

	genTexture(TextureConfig4Depth);
	m_FBO = genFBO({ TextureConfig4Depth });
	
	ElayGraphics::ResourceManager::registerSharedData("LightDepthTexture", TextureConfig4Depth);
	//m_pShader->activeShader();
	//m_pShader->setMat4UniformValue("u_ModelMatrix", glm::value_ptr(Monkey->getModelMatrix()));
	
	GLfloat near_plane = 1.0f, far_plane = 7.5f;
	glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
	auto light = ElayGraphics::ResourceManager::getSharedDataByName<LightSettings>("LightSettings");
	glm::vec3 lightPosition = -100.0f * light.sunLight.sunlightDirection;
	glm::mat4 lightView = glm::lookAt(lightPosition, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ElayGraphics::ResourceManager::registerSharedData("u_LightVPMatrix", lightProjection * lightView);
	ElayGraphics::ResourceManager::registerSharedData("u_LightPos", lightPosition);
	
	m_pShader->setMat4UniformValue("u_LightVPMatrix", glm::value_ptr(lightProjection * lightView));
	Monkey->initModel(*m_pShader);


}

void CShadowMapPass::updateV()
{

	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear( GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glViewport(0, 0, 1024, 1024);
	
	m_pShader->activeShader();
	GLfloat near_plane = 1.0f, far_plane = 7.5f;
	
	glm::mat4 lightProjection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
	auto light = ElayGraphics::ResourceManager::getSharedDataByName<LightSettings>("LightSettings");
	
	auto  Monkey = std::dynamic_pointer_cast<CModelLoad>(ElayGraphics::ResourceManager::getGameObjectByName("Monkey"));
	glm::vec3 center = Monkey->getAABB()->getCentre();
	glm::vec3 lightPosition = center + light.sunLight.sunlightDirection * (-20.0f);
	glm::mat4 lightView = glm::lookAt(lightPosition, lightPosition + light.sunLight.sunlightDirection, glm::vec3(0.0f, 1.0f, 0.0f));
	m_pShader->setMat4UniformValue("u_LightVPMatrix", glm::value_ptr(lightProjection * lightView));
	
	ElayGraphics::ResourceManager::updateSharedDataByName("u_LightVPMatrix", lightProjection * lightView);
	ElayGraphics::ResourceManager::updateSharedDataByName("u_LightPos", lightPosition);
	
	Monkey->updateModel(*m_pShader);
	
	
	glViewport(0, 0, ElayGraphics::WINDOW_KEYWORD::getWindowWidth(), ElayGraphics::WINDOW_KEYWORD::getWindowHeight());
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

