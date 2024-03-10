#pragma once
#include "RenderPass.h"
class IBLLigthPass : public IRenderPass
{
public:
	IBLLigthPass(const std::string& vPassName, int vExcutionOrder);
	~IBLLigthPass();

	void setIBLPath(const std::string& vIBLPath);
	virtual void initV();
	virtual void updateV();

private:
	std::string IBLPath;
};