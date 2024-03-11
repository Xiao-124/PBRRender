#pragma once
#include "RenderPass.h"

class CSSAORenderPass : public IRenderPass
{
public:
	CSSAORenderPass(const std::string& vPassName, int vExcutionOrder);
	virtual ~CSSAORenderPass();
	virtual void initV();
	virtual void updateV();
private:
	std::shared_ptr<CShader> depthShader;
	std::shared_ptr<CShader> ssaoShader;
	GLuint depthFBO;
	GLuint ssaoFBO;

};