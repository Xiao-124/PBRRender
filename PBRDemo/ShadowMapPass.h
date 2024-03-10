#pragma once
#include "RenderPass.h"

class CModelLoad;
class CShadowMapPass : public IRenderPass
{
public:
	CShadowMapPass(const std::string& vPassName, int vExcutionOrder);
	virtual ~CShadowMapPass();

	virtual void initV();
	virtual void updateV();

private:
	GLuint m_FBO = 0;
	glm::mat4 modelMatrix = glm::mat4(1.0);
};