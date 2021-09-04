#include "screens.h"

namespace
{
	EventListener<bool(uint32)> guiListener;
	EventListener<bool(uint32, ModifiersFlags)> keyReleaseListener;

	bool buttonBack(uint32 en)
	{
		if (en != 20)
			return false;
		setScreenMainmenu();
		return true;
	}

	bool keyRelease(uint32 key, ModifiersFlags modifiers)
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
		guii->focus(0);
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
			GuiLayoutSplitterComponent &ls = e->value<GuiLayoutSplitterComponent>();
			ls.inverse = true;
		}
		{ // splitter 1
			Entity *e = ents->create(5);
			splits[1] = e->name();
			GuiParentComponent &p = e->value<GuiParentComponent>();
			p.parent = splits[0];
			p.order = 2;
			GuiLayoutSplitterComponent &ls = e->value<GuiLayoutSplitterComponent>();
			ls.vertical = true;
			ls.inverse = true;
		}
		{ // splitter 2
			Entity *e = ents->create(6);
			splits[2] = e->name();
			GuiParentComponent &p = e->value<GuiParentComponent>();
			p.parent = splits[0];
			p.order = 1;
			GuiLayoutSplitterComponent &ls = e->value<GuiLayoutSplitterComponent>();
		}
		{ // splitter 3
			Entity *e = ents->create(7);
			splits[3] = e->name();
			GuiParentComponent &p = e->value<GuiParentComponent>();
			p.parent = splits[2];
			p.order = 1;
			GuiLayoutSplitterComponent &ls = e->value<GuiLayoutSplitterComponent>();
			ls.vertical = true;
			ls.inverse = true;
		}

		{ // panel 10
			Entity *e = ents->create(10);
			GuiParentComponent &p = e->value<GuiParentComponent>();
			p.parent = splits[3];
			p.order = 1;
			GuiScrollbarsComponent &sc = e->value<GuiScrollbarsComponent>();
			sc.alignment = Vec2(0, 0);
		}
		{ // panel 11
			Entity *e = ents->create(11);
			GuiParentComponent &p = e->value<GuiParentComponent>();
			p.parent = splits[3];
			p.order = 2;
			GuiScrollbarsComponent &sc = e->value<GuiScrollbarsComponent>();
			sc.alignment = Vec2(0, 1);
		}
		{ // panel 12
			Entity *e = ents->create(12);
			GuiParentComponent &p = e->value<GuiParentComponent>();
			p.parent = splits[2];
			p.order = 2;
			GuiScrollbarsComponent &sc = e->value<GuiScrollbarsComponent>();
			sc.alignment = Vec2(0.5, 0.5);
		}
		{ // panel 14
			Entity *e = ents->create(14);
			GuiParentComponent &p = e->value<GuiParentComponent>();
			p.parent = splits[1];
			p.order = 1;
			GuiScrollbarsComponent &sc = e->value<GuiScrollbarsComponent>();
			sc.alignment = Vec2(1, 0);
		}
		{ // panel 15
			Entity *e = ents->create(15);
			GuiParentComponent &p = e->value<GuiParentComponent>();
			p.parent = splits[1];
			p.order = 2;
			GuiScrollbarsComponent &sc = e->value<GuiScrollbarsComponent>();
			sc.alignment = Vec2(1, 1);
		}
	}

	void generateLogo()
	{
		EntityManager *ents = engineGui()->entities();
		Entity *logo = ents->createUnique();
		GuiLabelComponent &label = logo->value<GuiLabelComponent>();
		GuiImageComponent &image = logo->value<GuiImageComponent>();
		image.textureName = HashString("degrid/logo.png");
		GuiParentComponent &parent = logo->value<GuiParentComponent>();
		parent.parent = 14;
	}

	void generateButtonBack()
	{
		EntityManager *ents = engineGui()->entities();
		Entity *but = ents->create(20);
		GuiButtonComponent &button = but->value<GuiButtonComponent>();
		GuiTextComponent &txt = but->value<GuiTextComponent>();
		txt.assetName = HashString("degrid/languages/internationalized.textpack");
		txt.textName = HashString("gui/mainmenu/back");
		GuiParentComponent &parent = but->value<GuiParentComponent>();
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
