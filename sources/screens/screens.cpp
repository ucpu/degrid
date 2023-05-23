#include "screens.h"

namespace
{
	EventListener<bool(const GenericInput &)> keyReleaseListener;

	void eraseGui()
	{
		GuiManager *guii = engineGuiManager();
		guii->invalidateInputs();
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

		EntityManager *ents = engineGuiEntities();

		uint32 splits[4];

		{ // splitter 0
			Entity *e = ents->create(4);
			splits[0] = e->name();
			e->value<GuiLayoutLineComponent>().begin = LineEdgeModeEnum::Flexible;
			e->value<GuiLayoutLineComponent>().end = LineEdgeModeEnum::Flexible;
		}
		{ // splitter 1
			Entity *e = ents->create(5);
			splits[1] = e->name();
			GuiParentComponent &p = e->value<GuiParentComponent>();
			p.parent = splits[0];
			p.order = 2;
			e->value<GuiLayoutLineComponent>().begin = LineEdgeModeEnum::Flexible;
			e->value<GuiLayoutLineComponent>().end = LineEdgeModeEnum::Flexible;
			e->value<GuiLayoutLineComponent>().vertical = true;
		}
		{ // splitter 2
			Entity *e = ents->create(6);
			splits[2] = e->name();
			GuiParentComponent &p = e->value<GuiParentComponent>();
			p.parent = splits[0];
			p.order = 1;
			e->value<GuiLayoutLineComponent>().begin = LineEdgeModeEnum::Flexible;
			e->value<GuiLayoutLineComponent>().end = LineEdgeModeEnum::Flexible;
		}
		{ // splitter 3
			Entity *e = ents->create(7);
			splits[3] = e->name();
			GuiParentComponent &p = e->value<GuiParentComponent>();
			p.parent = splits[2];
			p.order = 1;
			e->value<GuiLayoutLineComponent>().begin = LineEdgeModeEnum::Flexible;
			e->value<GuiLayoutLineComponent>().end = LineEdgeModeEnum::Flexible;
			e->value<GuiLayoutLineComponent>().vertical = true;
		}

		{ // panel 10
			Entity *e = ents->create(10);
			GuiParentComponent &p = e->value<GuiParentComponent>();
			p.parent = splits[3];
			p.order = 1;
			e->value<GuiLayoutAlignmentComponent>().alignment = Vec2(0, 0);
		}
		{ // panel 11
			Entity *e = ents->create(11);
			GuiParentComponent &p = e->value<GuiParentComponent>();
			p.parent = splits[3];
			p.order = 2;
			e->value<GuiLayoutAlignmentComponent>().alignment = Vec2(0, 1);
		}
		{ // panel 12
			Entity *e = ents->create(12);
			GuiParentComponent &p = e->value<GuiParentComponent>();
			p.parent = splits[2];
			p.order = 2;
			e->value<GuiLayoutAlignmentComponent>().alignment = Vec2(0.5, 0.5);
		}
		{ // panel 14
			Entity *e = ents->create(14);
			GuiParentComponent &p = e->value<GuiParentComponent>();
			p.parent = splits[1];
			p.order = 1;
			e->value<GuiLayoutAlignmentComponent>().alignment = Vec2(1, 0);
		}
		{ // panel 15
			Entity *e = ents->create(15);
			GuiParentComponent &p = e->value<GuiParentComponent>();
			p.parent = splits[1];
			p.order = 2;
			e->value<GuiLayoutAlignmentComponent>().alignment = Vec2(1, 1);
		}
	}

	void generateLogo()
	{
		EntityManager *ents = engineGuiEntities();
		Entity *logo = ents->createUnique();
		logo->value<GuiLabelComponent>();
		logo->value<GuiImageComponent>().textureName = HashString("degrid/logo.png");
		logo->value<GuiParentComponent>().parent = 14;
	}

	void generateButtonBack()
	{
		EntityManager *ents = engineGuiEntities();
		Entity *but = ents->create(20);
		but->value<GuiButtonComponent>();
		GuiTextComponent &txt = but->value<GuiTextComponent>();
		txt.assetName = HashString("degrid/languages/internationalized.textpack");
		txt.textName = HashString("gui/mainmenu/back");
		but->value<GuiParentComponent>().parent = 15;
		but->value<GuiEventComponent>().event.bind([](Entity *) { setScreenMainmenu(); return true; });
		keyReleaseListener.attach(engineWindow()->events);
		keyReleaseListener.bind(inputListener<InputClassEnum::KeyRelease, InputKey>([](InputKey in) {
			if (in.key == 256) // esc
			{
				setScreenMainmenu();
				return true;
			}
			return false;
		}));
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
