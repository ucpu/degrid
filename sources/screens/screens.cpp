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
		guiManager *guii = gui();
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

		entityManager *ents = gui()->entities();

		uint32 splits[4];

		{ // splitter 0
			entity *e = ents->create(4);
			splits[0] = e->name();
			CAGE_COMPONENT_GUI(layoutSplitter, ls, e);
			ls.inverse = true;
		}
		{ // splitter 1
			entity *e = ents->create(5);
			splits[1] = e->name();
			CAGE_COMPONENT_GUI(parent, p, e);
			p.parent = splits[0];
			p.order = 2;
			CAGE_COMPONENT_GUI(layoutSplitter, ls, e);
			ls.vertical = true;
			ls.inverse = true;
		}
		{ // splitter 2
			entity *e = ents->create(6);
			splits[2] = e->name();
			CAGE_COMPONENT_GUI(parent, p, e);
			p.parent = splits[0];
			p.order = 1;
			CAGE_COMPONENT_GUI(layoutSplitter, ls, e);
		}
		{ // splitter 3
			entity *e = ents->create(7);
			splits[3] = e->name();
			CAGE_COMPONENT_GUI(parent, p, e);
			p.parent = splits[2];
			p.order = 1;
			CAGE_COMPONENT_GUI(layoutSplitter, ls, e);
			ls.vertical = true;
			ls.inverse = true;
		}

		{ // panel 10
			entity *e = ents->create(10);
			CAGE_COMPONENT_GUI(parent, p, e);
			p.parent = splits[3];
			p.order = 1;
			CAGE_COMPONENT_GUI(scrollbars, sc, e);
			sc.alignment = vec2(0, 0);
		}
		{ // panel 11
			entity *e = ents->create(11);
			CAGE_COMPONENT_GUI(parent, p, e);
			p.parent = splits[3];
			p.order = 2;
			CAGE_COMPONENT_GUI(scrollbars, sc, e);
			sc.alignment = vec2(0, 1);
		}
		{ // panel 12
			entity *e = ents->create(12);
			CAGE_COMPONENT_GUI(parent, p, e);
			p.parent = splits[2];
			p.order = 2;
			CAGE_COMPONENT_GUI(scrollbars, sc, e);
			sc.alignment = vec2(0.5, 0.5);
		}
		{ // panel 14
			entity *e = ents->create(14);
			CAGE_COMPONENT_GUI(parent, p, e);
			p.parent = splits[1];
			p.order = 1;
			CAGE_COMPONENT_GUI(scrollbars, sc, e);
			sc.alignment = vec2(1, 0);
		}
		{ // panel 15
			entity *e = ents->create(15);
			CAGE_COMPONENT_GUI(parent, p, e);
			p.parent = splits[1];
			p.order = 2;
			CAGE_COMPONENT_GUI(scrollbars, sc, e);
			sc.alignment = vec2(1, 1);
		}
	}

	void generateLogo()
	{
		entityManager *ents = gui()->entities();
		entity *logo = ents->createUnique();
		CAGE_COMPONENT_GUI(label, label, logo);
		CAGE_COMPONENT_GUI(image, image, logo);
		image.textureName = hashString("degrid/logo.png");
		CAGE_COMPONENT_GUI(parent, parent, logo);
		parent.parent = 14;
	}

	void generateButtonBack()
	{
		entityManager *ents = gui()->entities();
		entity *but = ents->create(20);
		CAGE_COMPONENT_GUI(button, button, but);
		CAGE_COMPONENT_GUI(text, txt, but);
		txt.assetName = hashString("degrid/languages/internationalized.textpack");
		txt.textName = hashString("gui/mainmenu/back");
		CAGE_COMPONENT_GUI(parent, parent, but);
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
