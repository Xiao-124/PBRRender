#include "Interface.h"
#include "ModelLoad.h"
#include "ModelRenderPass.h"
#include "Color.h"
#include "ColorSpaceUtils.h"
#include "ColorGradingPass.h"
#include "IBL.h"
#include "CustomGUI.h"
#include "SkyBoxPass.h"
#include "ShadowMapPass.h"
#include "GroundObject.h"
int main()
{

	ElayGraphics::WINDOW_KEYWORD::setWindowSize(1280, 760);
	ElayGraphics::WINDOW_KEYWORD::setSampleNum(0);
	//ElayGraphics::WINDOW_KEYWORD::setIsCursorDisable(true);
	ElayGraphics::WINDOW_KEYWORD::setIsCursorDisable(false);
	ElayGraphics::COMPONENT_CONFIG::setIsEnableGUI(true);

	ElayGraphics::ResourceManager::registerGameObject(std::make_shared<CModelLoad>("Monkey", 1));
	ElayGraphics::ResourceManager::registerGameObject(std::make_shared<CGroundObject>("GroundObject", 2));

	ElayGraphics::ResourceManager::registerRenderPass(std::make_shared<IBLLigthPass>("IBLLightPass", 0));
	ElayGraphics::ResourceManager::registerRenderPass(std::make_shared<CShadowMapPass>("CShadowMapPass", 1));
	ElayGraphics::ResourceManager::registerRenderPass(std::make_shared<CSkyboxPass>("SkyboxPass", 2));
	ElayGraphics::ResourceManager::registerRenderPass(std::make_shared<CModelRenderPass>("MonkeyRenderPass", 3));
	ElayGraphics::ResourceManager::registerRenderPass(std::make_shared<CColorGradingPass>("ColorGradingPass", 4));
	

	ElayGraphics::ResourceManager::registerSubGUI(std::make_shared<CCustomGUI>("CustomGUI", 1));

	ElayGraphics::App::initApp();
	ElayGraphics::App::updateApp();

	return 0;
}