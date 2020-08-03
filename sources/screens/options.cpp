#include <cage-core/config.h>

#include "screens.h"
#include "../game.h"

ConfigUint32 confControlMovement("degrid/control/movement", 0);
ConfigUint32 confControlFiring("degrid/control/firing", 4);
ConfigUint32 confControlBomb("degrid/control/bomb", 4 + 4);
ConfigUint32 confControlTurret("degrid/control/turret", 2 + 4);
ConfigUint32 confControlDecoy("degrid/control/decoy", 0 + 4);
ConfigString confPlayerName("degrid/player/name", "player name");
ConfigFloat confPlayerShotColorR("degrid/player/shotColorR", 1);
ConfigFloat confPlayerShotColorG("degrid/player/shotColorG", 1);
ConfigFloat confPlayerShotColorB("degrid/player/shotColorB", 1);
ConfigFloat confVolumeMusic("degrid/volume/music", 0.5f);
ConfigFloat confVolumeEffects("degrid/volume/effects", 0.5f);
ConfigFloat confVolumeSpeech("degrid/volume/speech", 0.7f);

namespace
{
	void addOptionsMovementFiring(uint32 ctr)
	{
		EntityManager *ents = engineGui()->entities();
		{
			Entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(Parent, parent, opt);
			parent.parent = ctr;
			parent.order = 0;
			CAGE_COMPONENT_GUI(Text, txt, opt);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/arrowsAbsolute");
		}
		{
			Entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(Parent, parent, opt);
			parent.parent = ctr;
			parent.order = 1;
			CAGE_COMPONENT_GUI(Text, txt, opt);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/arrowsRelative");
		}
		{
			Entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(Parent, parent, opt);
			parent.parent = ctr;
			parent.order = 2;
			CAGE_COMPONENT_GUI(Text, txt, opt);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/lmb");
		}
		{
			Entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(Parent, parent, opt);
			parent.parent = ctr;
			parent.order = 3;
			CAGE_COMPONENT_GUI(Text, txt, opt);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/rmb");
		}
		{
			Entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(Parent, parent, opt);
			parent.parent = ctr;
			parent.order = 4;
			CAGE_COMPONENT_GUI(Text, txt, opt);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/cursor");
		}
	}

	void addOptionsPowers(uint32 ctr)
	{
		EntityManager *ents = engineGui()->entities();
		{
			Entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(Parent, parent, opt);
			parent.parent = ctr;
			parent.order = -4;
			CAGE_COMPONENT_GUI(Text, txt, opt);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/lmb");
		}
		{
			Entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(Parent, parent, opt);
			parent.parent = ctr;
			parent.order = -3;
			CAGE_COMPONENT_GUI(Text, txt, opt);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/mmb");
		}
		{
			Entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(Parent, parent, opt);
			parent.parent = ctr;
			parent.order = -2;
			CAGE_COMPONENT_GUI(Text, txt, opt);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/rmb");
		}
		{
			Entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(Parent, parent, opt);
			parent.parent = ctr;
			parent.order = -1;
			CAGE_COMPONENT_GUI(Text, txt, opt);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/space");
		}
		for (sint32 i = 0; i < sizeof(Letters); i++)
		{
			Entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(Parent, parent, opt);
			parent.parent = ctr;
			parent.order = Letters[i];
			CAGE_COMPONENT_GUI(Text, txt, opt);
			txt.value = string(&Letters[i], 1);
		}
	}

	EventListener<bool(uint32)> guiEvent;

	bool guiFunction(uint32 en)
	{
		switch (en)
		{
		case 31:
		{
			CAGE_COMPONENT_GUI(ComboBox, control, engineGui()->entities()->get(en));
			confControlMovement = control.selected;
			return true;
		}
		case 32:
		{
			CAGE_COMPONENT_GUI(ComboBox, control, engineGui()->entities()->get(en));
			confControlFiring = control.selected;
			return true;
		}
		case 33:
		{
			CAGE_COMPONENT_GUI(ComboBox, control, engineGui()->entities()->get(en));
			confControlBomb = control.selected;
			return true;
		}
		case 34:
		{
			CAGE_COMPONENT_GUI(ComboBox, control, engineGui()->entities()->get(en));
			confControlTurret = control.selected;
			return true;
		}
		case 35:
		{
			CAGE_COMPONENT_GUI(ComboBox, control, engineGui()->entities()->get(en));
			confControlDecoy = control.selected;
			return true;
		}
		case 36:
		{
			CAGE_COMPONENT_GUI(SliderBar, control, engineGui()->entities()->get(en));
			confVolumeMusic = control.value.value;
			return true;
		}
		case 37:
		{
			CAGE_COMPONENT_GUI(SliderBar, control, engineGui()->entities()->get(en));
			confVolumeEffects = control.value.value;
			return true;
		}
		case 38:
		{
			CAGE_COMPONENT_GUI(SliderBar, control, engineGui()->entities()->get(en));
			confVolumeSpeech = control.value.value;
			return true;
		}
		}
		return false;
	}
}

void setScreenOptions()
{
	regenerateGui(GuiConfig());
	EntityManager *ents = engineGui()->entities();
	guiEvent.bind<&guiFunction>();
	guiEvent.attach(engineGui()->widgetEvent);

	Entity *tabs = ents->createUnique();
	{
		CAGE_COMPONENT_GUI(Parent, parent, tabs);
		parent.parent = 12;
		CAGE_COMPONENT_GUI(LayoutLine, layout, tabs);
		layout.vertical = false;
	}

	{ // controls
		Entity *panel = ents->createUnique();
		{
			CAGE_COMPONENT_GUI(Panel, control, panel);
			CAGE_COMPONENT_GUI(Parent, parent, panel);
			parent.parent = tabs->name();
			parent.order = 1;
			CAGE_COMPONENT_GUI(LayoutTable, layout, panel);
			layout.sections = 2;
			CAGE_COMPONENT_GUI(Text, txt, panel);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/controls");
			CAGE_COMPONENT_GUI(Scrollbars, sc, panel);
		}
		uint32 index = 0;

		{ // movement
			{
				Entity *lbl = ents->createUnique();
				CAGE_COMPONENT_GUI(Parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(Label, control, lbl);
				CAGE_COMPONENT_GUI(Text, txt, lbl);
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/options/movement");
			}

			{
				Entity *ctr = ents->create(31);
				CAGE_COMPONENT_GUI(Parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(ComboBox, control, ctr);
				control.selected = confControlMovement;
				addOptionsMovementFiring(ctr->name());
			}
		}

		{ // firing
			{
				Entity *lbl = ents->createUnique();
				CAGE_COMPONENT_GUI(Parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(Label, control, lbl);
				CAGE_COMPONENT_GUI(Text, txt, lbl);
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/options/firing");
			}

			{
				Entity *ctr = ents->create(32);
				CAGE_COMPONENT_GUI(Parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(ComboBox, control, ctr);
				control.selected = confControlFiring;
				addOptionsMovementFiring(ctr->name());
			}
		}

		{ // bomb
			{
				Entity *lbl = ents->createUnique();
				CAGE_COMPONENT_GUI(Parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(Label, control, lbl);
				CAGE_COMPONENT_GUI(Text, txt, lbl);
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/options/bomb");
			}

			{
				Entity *ctr = ents->create(33);
				CAGE_COMPONENT_GUI(Parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(ComboBox, control, ctr);
				control.selected = confControlBomb;
				addOptionsPowers(ctr->name());
			}
		}

		{ // turret
			{
				Entity *lbl = ents->createUnique();
				CAGE_COMPONENT_GUI(Parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(Label, control, lbl);
				CAGE_COMPONENT_GUI(Text, txt, lbl);
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/options/turret");
			}

			{
				Entity *ctr = ents->create(34);
				CAGE_COMPONENT_GUI(Parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(ComboBox, control, ctr);
				control.selected = confControlTurret;
				addOptionsPowers(ctr->name());
			}
		}

		{ // decoy
			{
				Entity *lbl = ents->createUnique();
				CAGE_COMPONENT_GUI(Parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(Label, control, lbl);
				CAGE_COMPONENT_GUI(Text, txt, lbl);
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/options/decoy");
			}

			{
				Entity *ctr = ents->create(35);
				CAGE_COMPONENT_GUI(Parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(ComboBox, control, ctr);
				control.selected = confControlDecoy;
				addOptionsPowers(ctr->name());
			}
		}
	}

	{
		Entity *panel = ents->createUnique();
		{
			CAGE_COMPONENT_GUI(Panel, control, panel);
			CAGE_COMPONENT_GUI(Parent, parent, panel);
			parent.parent = tabs->name();
			parent.order = 2;
			CAGE_COMPONENT_GUI(LayoutTable, layout, panel);
			layout.sections = 2;
			CAGE_COMPONENT_GUI(Text, txt, panel);
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/sounds");
			CAGE_COMPONENT_GUI(Scrollbars, sc, panel);
		}
		uint32 index = 0;

		{ // music volume
			{
				Entity *lbl = ents->createUnique();
				CAGE_COMPONENT_GUI(Parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(Label, control, lbl);
				CAGE_COMPONENT_GUI(Text, txt, lbl);
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/options/musicVolume");
			}

			{
				Entity *ctr = ents->create(36);
				CAGE_COMPONENT_GUI(Parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(SliderBar, control, ctr);
				control.value = real(confVolumeMusic);
			}
		}

		{ // effects volume
			{
				Entity *lbl = ents->createUnique();
				CAGE_COMPONENT_GUI(Parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(Label, control, lbl);
				CAGE_COMPONENT_GUI(Text, txt, lbl);
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/options/effectsVolume");
			}

			{
				Entity *ctr = ents->create(37);
				CAGE_COMPONENT_GUI(Parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(SliderBar, control, ctr);
				control.value = real(confVolumeEffects);
			}
		}

		{ // speech volume
			{
				Entity *lbl = ents->createUnique();
				CAGE_COMPONENT_GUI(Parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(Label, control, lbl);
				CAGE_COMPONENT_GUI(Text, txt, lbl);
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/options/speechVolume");
			}

			{
				Entity *ctr = ents->create(38);
				CAGE_COMPONENT_GUI(Parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(SliderBar, control, ctr);
				control.value = real(confVolumeSpeech);
			}
		}
	}
}

