#include "TerrainRenderPass.h"
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
TerrainRenderPass::TerrainRenderPass(const std::string& vPassName, int vExcutionOrder) :IRenderPass(vPassName, vExcutionOrder)
{


}

TerrainRenderPass::~TerrainRenderPass()
{

}

void TerrainRenderPass::initV()
{
	m_pShader = std::make_shared<CShader>("TerrainShadowMap_VS.glsl", "TerrainShadowMap_FS.glsl");
}

void TerrainRenderPass::updateV()
{



}
