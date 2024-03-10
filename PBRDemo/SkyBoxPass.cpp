#include "Shader.h"
#include "Interface.h"
#include "Common.h"
#include "Utils.h"
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "SkyboxPass.h"


CSkyboxPass::CSkyboxPass(const std::string& vPassName, int vExcutionOrder) :IRenderPass(vPassName, vExcutionOrder)
{
}

CSkyboxPass::~CSkyboxPass()
{
}

//************************************************************************************
//Function:
void CSkyboxPass::initV()
{
	auto TextureConfig4Albedo = std::make_shared<ElayGraphics::STexture>();
	auto TextureConfig4Depth = std::make_shared<ElayGraphics::STexture>();
	TextureConfig4Albedo->InternalFormat = GL_RGBA32F;
	TextureConfig4Albedo->ExternalFormat = GL_RGBA;
	TextureConfig4Albedo->DataType = GL_FLOAT;
	genTexture(TextureConfig4Albedo);

	TextureConfig4Depth->InternalFormat = GL_DEPTH_COMPONENT32F;
	TextureConfig4Depth->ExternalFormat = GL_DEPTH_COMPONENT;
	TextureConfig4Depth->DataType = GL_FLOAT;
	TextureConfig4Depth->Type4MinFilter = GL_LINEAR;
	TextureConfig4Depth->Type4MagFilter = GL_LINEAR;
	TextureConfig4Depth->TextureAttachmentType = ElayGraphics::STexture::ETextureAttachmentType::DepthTexture;
	genTexture(TextureConfig4Depth);

	m_FBO = genFBO({ TextureConfig4Albedo,TextureConfig4Depth });
	ElayGraphics::ResourceManager::registerSharedData("TextureConfig4Albedo", TextureConfig4Albedo);
	ElayGraphics::ResourceManager::registerSharedData("DepthTexture", TextureConfig4Depth);
	ElayGraphics::ResourceManager::registerSharedData("mFBO", m_FBO);


	m_pShader = std::make_shared<CShader>("Skybox_VS.glsl", "Skybox_FS.glsl");
	auto envCubemap = ElayGraphics::ResourceManager::getSharedDataByName<std::shared_ptr<ElayGraphics::STexture>>("envCubemap");
	m_pShader->activeShader();
	m_pShader->setTextureUniformValue("u_Skybox", envCubemap);
}


//************************************************************************************
//Function:
void CSkyboxPass::updateV()
{
	auto albeo = ElayGraphics::ResourceManager::getSharedDataByName<std::shared_ptr<ElayGraphics::STexture>>("TextureConfig4Albedo");
	glViewport(0, 0, albeo->Width, albeo->Height);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	m_pShader->activeShader();
	drawCube();
	glDepthFunc(GL_LESS);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}