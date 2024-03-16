#pragma once
#include "RenderPass.h"

class CModelLoad;
class CModelRenderPass : public IRenderPass
{
public:
	CModelRenderPass(const std::string& vPassName, int vExcutionOrder);
	virtual ~CModelRenderPass();

	virtual void initV();
	virtual void updateV();

private:
	std::shared_ptr<CModelLoad> m_pMonkey;
	int m_FBO = 0;
	glm::mat4 modelMatrix = glm::mat4(1.0);

	std::shared_ptr<CShader> standardModelShader;
	std::shared_ptr<CShader> clothModelShader;
	std::shared_ptr<CShader> subsurafceModelShader;
};