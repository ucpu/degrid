#include "screens.h"

namespace
{
	eventListener<bool(uint32)> guiListener;
	eventListener<bool(uint32, uint32, modifiersFlags)> keyReleaseListener;

	bool buttonBack(uint32 en)
	{
		if (en != 20)
			return false;
		setScreenMainmenu();
		return true;
	}

	bool keyRelease(uint32 key, uint32, modifiersFlags modifiers)
	{
		if (key == 256) // esc
		{
			setScreenMainmenu();
			return true;
		}
		return false;
	}

	void eraseGui()
	{
		guiClass *guii = gui();
		guii->skipAllEventsUntilNextUpdate();
		guii->setFocus(0);
		guii->entities()->destroy();
		guii->widgetEvent.detach();
		keyReleaseListener.detach();
	}

	void generateLayout()
	{
		// splitter 0
		// +---------------------+--+
		// |S                    |M |
		// |                     |  |
		// |                     |  |
		// |                     |  |
		// |                     |  |
		// |                     |  |
		// +---------------------+--+

		// splitter 1
		// +---------------------+--+
		// |                     |S |
		// |                     |  |
		// |                     |  |
		// |                     |  |
		// |                     +--+
		// |                     |M |
		// +---------------------+--+

		// splitter 2
		// +--+------------------+--+
		// |M |S                 |  |
		// |  |                  |  |
		// |  |                  |  |
		// |  |                  |  |
		// |  |                  +--+
		// |  |                  |  |
		// +--+------------------+--+

		// splitter 3
		// +--+------------------+--+
		// |S |                  |  |
		// |  |                  |  |
		// |  |                  |  |
		// |  |                  |  |
		// +--+                  +--+
		// |M |                  |  |
		// +--+------------------+--+

		// panels
		// +--+------------------+--+
		// |10|12                |14|
		// |  |                  |  |
		// |  |                  |  |
		// |  |                  |  |
		// +--+                  +--+
		// |11|                  |15|
		// +--+------------------+--+
		// 14 - logo
		// 15 - back button

		entityManagerClass *ents = gui()->entities();

		uint32 splits[4];

		{ // splitter 0
			entityClass *e = ents->createUnique();
			splits[0] = e->name();
			GUI_GET_COMPONENT(layoutSplitter, ls, e);
			ls.inverse = true;
		}
		{ // splitter 1
			entityClass *e = ents->createUnique();
			splits[1] = e->name();
			GUI_GET_COMPONENT(parent, p, e);
			p.parent = splits[0];
			p.order = 2;
			GUI_GET_COMPONENT(layoutSplitter, ls, e);
			ls.vertical = true;
			ls.inverse = true;
		}
		{ // splitter 2
			entityClass *e = ents->createUnique();
			splits[2] = e->name();
			GUI_GET_COMPONENT(parent, p, e);
			p.parent = splits[0];
			p.order = 1;
			GUI_GET_COMPONENT(layoutSplitter, ls, e);
		}
		{ // splitter 3
			entityClass *e = ents->createUnique();
			splits[3] = e->name();
			GUI_GET_COMPONENT(parent, p, e);
			p.parent = splits[2];
			p.order = 1;
			GUI_GET_COMPONENT(layoutSplitter, ls, e);
			ls.vertical = true;
			ls.inverse = true;
		}

		{ // panel 10
			entityClass *e = ents->create(10);
			GUI_GET_COMPONENT(parent, p, e);
			p.parent = splits[3];
			p.order = 1;
			GUI_GET_COMPONENT(scrollbars, sc, e);
			sc.alignment = vec2(0, 0);
		}
		{ // panel 11
			entityClass *e = ents->create(11);
			GUI_GET_COMPONENT(parent, p, e);
			p.parent = splits[3];
			p.order = 2;
			GUI_GET_COMPONENT(scrollbars, sc, e);
			sc.alignment = vec2(0, 1);
		}
		{ // panel 12
			entityClass *e = ents->create(12);
			GUI_GET_COMPONENT(parent, p, e);
			p.parent = splits[2];
			p.order = 2;
			GUI_GET_COMPONENT(scrollbars, sc, e);
			sc.alignment = vec2(0.5, 0.5);
		}
		{ // panel 14
			entityClass *e = ents->create(14);
			GUI_GET_COMPONENT(parent, p, e);
			p.parent = splits[1];
			p.order = 1;
			GUI_GET_COMPONENT(scrollbars, sc, e);
			sc.alignment = vec2(1, 0);
		}
		{ // panel 15
			entityClass *e = ents->create(15);
			GUI_GET_COMPONENT(parent, p, e);
			p.parent = splits[1];
			p.order = 2;
			GUI_GET_COMPONENT(scrollbars, sc, e);
			sc.alignment = vec2(1, 1);
		}
	}

	void generateLogo()
	{
		entityManagerClass *ents = gui()->entities();
		entityClass *logo = ents->createUnique();
		GUI_GET_COMPONENT(label, label, logo);
		GUI_GET_COMPONENT(image, image, logo);
		image.textureName = hashString("grid/logo.gif");
		GUI_GET_COMPONENT(parent, parent, logo);
		parent.parent = 14;
	}

	void generateButtonBack()
	{
		entityManagerClass *ents = gui()->entities();
		entityClass *but = ents->create(20);
		GUI_GET_COMPONENT(button, button, but);
		GUI_GET_COMPONENT(text, txt, but);
		txt.assetName = hashString("grid/languages/internationalized.textpack");
		txt.textName = hashString("gui/mainmenu/back");
		GUI_GET_COMPONENT(parent, parent, but);
		parent.parent = 15;
		guiListener.attach(gui()->widgetEvent);
		guiListener.bind<&buttonBack>();
		keyReleaseListener.attach(window()->events.keyRelease);
		keyReleaseListener.bind<&keyRelease>();
	}

}

guiConfig::guiConfig() : backButton(true), logo(true)
{}

void regenerateGui(const guiConfig &config)
{
	eraseGui();
	generateLayout();
	if (config.logo)
		generateLogo();
	if (config.backButton)
		generateButtonBack();
}
