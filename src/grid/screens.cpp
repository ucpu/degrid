#include "includes.h"
#include "screens.h"

windowEventListeners winEvtLists;

namespace
{
	eventListener<void(uint32)> guiListener;

	const bool buttonBack(uint32 en)
	{
		if (en != 501) return false;
		setScreenMainmenu();
		return true;
	}
}

void eraseGui()
{
	guiClass *guii = gui();
	guii->setFocus(0);
	guii->entities()->getAllEntities()->destroyAllEntities();
	guii->genericEvent.detach();
	winEvtLists.attachAll(window());
}

void generateLogo()
{
	entityManagerClass *ents = gui()->entities();
	entityClass *logo = ents->newEntity(ents->generateUniqueName());
	GUI_GET_COMPONENT(control, control, logo);
	control.controlType = controlTypeEnum::Empty;
	GUI_GET_COMPONENT(image, image, logo);
	image.textureName = hashString("grid/logo.gif");
	GUI_GET_COMPONENT(position, position, logo);
	position.x = 1.0;
	position.y = 0.0;
	position.w = 753.f / 2000.f;
	position.h = 201.f / 2000.f;
	position.xUnit = unitsModeEnum::ScreenWidth;
	position.yUnit = position.wUnit = position.hUnit = unitsModeEnum::ScreenHeight;
	position.anchorX = 1.0;
}

void generateButtonBack()
{
	entityManagerClass *ents = gui()->entities();
	entityClass *but = ents->newEntity(501);
	GUI_GET_COMPONENT(control, control, but);
	control.controlType = controlTypeEnum::Button;
	GUI_GET_COMPONENT(text, txt, but);
	txt.assetName = hashString("grid/languages/internationalized.textpack");
	txt.textName = hashString("gui/mainmenu/back");
	GUI_GET_COMPONENT(position, position, but);
	position.x = 1.0;
	position.y = 1.0;
	position.xUnit = unitsModeEnum::ScreenWidth;
	position.yUnit = unitsModeEnum::ScreenHeight;
	position.anchorX = 1.0;
	position.anchorY = 1.0;
	GUI_GET_COMPONENT(format, format, but);
	format.align = textAlignEnum::Center;
	gui()->genericEvent.add(guiListener);
	guiListener.bind<&buttonBack>();
}
