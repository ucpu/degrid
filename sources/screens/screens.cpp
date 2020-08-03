#include "screens.h"

namespace
{
	EventListener<bool(uint32)> guiListener;
	EventListener<bool(uint32, uint32, ModifiersFlags)> keyReleaseListener;

	bool buttonBack(uint32 en)
	{
		if (en != 20)
			return false;
		setScreenMainmenu();
		return true;
	}

	bool keyRelease(uint32 key, uint32, ModifiersFlags modifiers)
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
		Gui *guii = engineGui();
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

		EntityManager *ents = engineGui()->entities();

		uint32 splits[4];

		{ // splitter 0
			Entity *e = ents->create(4);
			splits[0] = e->name();
			CAGE_COMPONENT_GUI(LayoutSplitter, ls, e);
			ls.inverse = true;
		}
		{ // splitter 1
			Entity *e = ents->create(5);
			splits[1] = e->name();
			CAGE_COMPONENT_GUI(Parent, p, e);
			p.parent = splits[0];
			p.order = 2;
			CAGE_COMPONENT_GUI(LayoutSplitter, ls, e);
			ls.vertical = true;
			ls.inverse = true;
		}
		{ // splitter 2
			Entity *e = ents->create(6);
			splits[2] = e->name();
			CAGE_COMPONENT_GUI(Parent, p, e);
			p.parent = splits[0];
			p.order = 1;
			CAGE_COMPONENT_GUI(LayoutSplitter, ls, e);
		}
		{ // splitter 3
			Entity *e = ents->create(7);
			splits[3] = e->name();
			CAGE_COMPONENT_GUI(Parent, p, e);
			p.parent = splits[2];
			p.order = 1;
			CAGE_COMPONENT_GUI(LayoutSplitter, ls, e);
			ls.vertical = true;
			ls.inverse = true;
		}

		{ // panel 10
			Entity *e = ents->create(10);
			CAGE_COMPONENT_GUI(Parent, p, e);
			p.parent = splits[3];
			p.order = 1;
			CAGE_COMPONENT_GUI(Scrollbars, sc, e);
			sc.alignment = vec2(0, 0);
		}
		{ // panel 11
			Entity *e = ents->create(11);
			CAGE_COMPONENT_GUI(Parent, p, e);
			p.parent = splits[3];
			p.order = 2;
			CAGE_COMPONENT_GUI(Scrollbars, sc, e);
			sc.alignment = vec2(0, 1);
		}
		{ // panel 12
			Entity *e = ents->create(12);
			CAGE_COMPONENT_GUI(Parent, p, e);
			p.parent = splits[2];
			p.order = 2;
			CAGE_COMPONENT_GUI(Scrollbars, sc, e);
			sc.alignment = vec2(0.5, 0.5);
		}
		{ // panel 14
			Entity *e = ents->create(14);
			CAGE_COMPONENT_GUI(Parent, p, e);
			p.parent = splits[1];
			p.order = 1;
			CAGE_COMPONENT_GUI(Scrollbars, sc, e);
			sc.alignment = vec2(1, 0);
		}
		{ // panel 15
			Entity *e = ents->create(15);
			CAGE_COMPONENT_GUI(Parent, p, e);
			p.parent = splits[1];
			p.order = 2;
			CAGE_COMPONENT_GUI(Scrollbars, sc, e);
			sc.alignment = vec2(1, 1);
		}
	}

	void generateLogo()
	{
		EntityManager *ents = engineGui()->entities();
		Entity *logo = ents->createUnique();
		CAGE_COMPONENT_GUI(Label, label, logo);
		CAGE_COMPONENT_GUI(Image, image, logo);
		image.textureName = HashString("degrid/logo.png");
		CAGE_COMPONENT_GUI(Parent, parent, logo);
		parent.parent = 14;
	}

	void generateButtonBack()
	{
		EntityManager *ents = engineGui()->entities();
		Entity *but = ents->create(20);
		CAGE_COMPONENT_GUI(Button, button, but);
		CAGE_COMPONENT_GUI(Text, txt, but);
		txt.assetName = HashString("degrid/languages/internationalized.textpack");
		txt.textName = HashString("gui/mainmenu/back");
		CAGE_COMPONENT_GUI(Parent, parent, but);
		parent.parent = 15;
		guiListener.attach(engineGui()->widgetEvent);
		guiListener.bind<&buttonBack>();
		keyReleaseListener.attach(engineWindow()->events.keyRelease);
		keyReleaseListener.bind<&keyRelease>();
	}

}

void regenerateGui(const GuiConfig &config)
{
	eraseGui();
	generateLayout();
	if (config.logo)
		generateLogo();
	if (config.backButton)
		generateButtonBack();
}
