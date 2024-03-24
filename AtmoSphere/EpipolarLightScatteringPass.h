#pragma once
#include "RenderPass.h"
class EpipolarLightScattering : public IRenderPass
{
public:
    EpipolarLightScattering(const std::string& vPassName, int vExcutionOrder);
    ~EpipolarLightScattering();

    virtual void initV();
    virtual void updateV();

protected:
    void PrecomputeScatteringLUT();


private:
    GLuint m_FBO;
    GLuint m_sUBO;
};
