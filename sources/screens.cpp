#include "includes.h"
#include "screens.h"

windowEventListeners winEvtLists;

namespace
{
	eventListener<bool(uint32)> guiListener;

	bool buttonBack(uint32 en)
	{
		if (en != 501)
			return false;
		setScreenMainmenu();
		return true;
	}
}

void eraseGui()
{
	guiClass *guii = gui();
	guii->setFocus(0);
	guii->entities()->getAllEntities()->destroyAllEntities();
	guii->widgetEvent.detach();
	winEvtLists.attachAll(window());
}

void generateLogo()
{
	entityManagerClass *ents = gui()->entities();
	entityClass *logo = ents->newEntity(ents->generateUniqueName());
	GUI_GET_COMPONENT(label, label, logo);
	GUI_GET_COMPONENT(image, image, logo);
	image.textureName = hashString("grid/logo.gif");
	GUI_GET_COMPONENT(position, position, logo);
	position.anchor = vec2(1, 0);
	position.position.values[0] = 1;
	position.position.units[0] = unitEnum::ScreenWidth;
	position.position.values[1] = 0;
	position.position.units[1] = unitEnum::ScreenHeight;
}

void generateButtonBack()
{
	entityManagerClass *ents = gui()->entities();
	entityClass *but = ents->newEntity(501);
	GUI_GET_COMPONENT(button, button, but);
	GUI_GET_COMPONENT(text, txt, but);
	txt.assetName = hashString("grid/languages/internationalized.textpack");
	txt.textName = hashString("gui/mainmenu/back");
	GUI_GET_COMPONENT(position, position, but);
	position.anchor = vec2(1, 1);
	position.position.values[0] = 1;
	position.position.units[0] = unitEnum::ScreenWidth;
	position.position.values[1] = 1;
	position.position.units[1] = unitEnum::ScreenHeight;
	gui()->widgetEvent.attach(guiListener);
	guiListener.bind<&buttonBack>();
}
