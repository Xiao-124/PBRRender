#pragma once
#include "RenderPass.h"
class TerrainRenderPass: public IRenderPass
{
public:
    TerrainRenderPass(const std::string& vPassName, int vExcutionOrder);
    ~TerrainRenderPass();

    virtual void initV();
    virtual void updateV();

protected:


private:
    GLuint m_FBO;
    GLuint m_sUBO;
    std::shared_ptr<CShader> m_SunShader;

};
