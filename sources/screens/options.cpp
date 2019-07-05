#include "screens.h"
#include "../game.h"

#include <cage-core/config.h>

configUint32 confControlMovement("degrid.control.movement", 0);
configUint32 confControlFiring("degrid.control.firing", 4);
configUint32 confControlBomb("degrid.control.bomb", 4 + 4);
configUint32 confControlTurret("degrid.control.turret", 2 + 4);
configUint32 confControlDecoy("degrid.control.decoy", 0 + 4);
configString confPlayerName("degrid.player.name", "player name");
configFloat confPlayerShotColorR("degrid.player.shotColorR", 1);
configFloat confPlayerShotColorG("degrid.player.shotColorG", 1);
configFloat confPlayerShotColorB("degrid.player.shotColorB", 1);
configFloat confVolumeMusic("degrid.volume.music", 0.5f);
configFloat confVolumeEffects("degrid.volume.effects", 0.5f);
configFloat confVolumeSpeech("degrid.volume.speech", 0.7f);

namespace
{
	void addOptionsMovementFiring(uint32 ctr)
	{
		entityManager *ents = gui()->entities();
		{
			entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(parent, parent, opt);
			parent.parent = ctr;
			parent.order = 0;
			CAGE_COMPONENT_GUI(text, txt, opt);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/arrowsAbsolute");
		}
		{
			entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(parent, parent, opt);
			parent.parent = ctr;
			parent.order = 1;
			CAGE_COMPONENT_GUI(text, txt, opt);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/arrowsRelative");
		}
		{
			entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(parent, parent, opt);
			parent.parent = ctr;
			parent.order = 2;
			CAGE_COMPONENT_GUI(text, txt, opt);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/lmb");
		}
		{
			entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(parent, parent, opt);
			parent.parent = ctr;
			parent.order = 3;
			CAGE_COMPONENT_GUI(text, txt, opt);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/rmb");
		}
		{
			entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(parent, parent, opt);
			parent.parent = ctr;
			parent.order = 4;
			CAGE_COMPONENT_GUI(text, txt, opt);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/cursor");
		}
	}

	void addOptionsPowers(uint32 ctr)
	{
		entityManager *ents = gui()->entities();
		{
			entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(parent, parent, opt);
			parent.parent = ctr;
			parent.order = -4;
			CAGE_COMPONENT_GUI(text, txt, opt);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/lmb");
		}
		{
			entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(parent, parent, opt);
			parent.parent = ctr;
			parent.order = -3;
			CAGE_COMPONENT_GUI(text, txt, opt);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/mmb");
		}
		{
			entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(parent, parent, opt);
			parent.parent = ctr;
			parent.order = -2;
			CAGE_COMPONENT_GUI(text, txt, opt);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/rmb");
		}
		{
			entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(parent, parent, opt);
			parent.parent = ctr;
			parent.order = -1;
			CAGE_COMPONENT_GUI(text, txt, opt);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/space");
		}
		for (sint32 i = 0; i < sizeof(letters); i++)
		{
			entity *opt = ents->createUnique();
			CAGE_COMPONENT_GUI(parent, parent, opt);
			parent.parent = ctr;
			parent.order = letters[i];
			CAGE_COMPONENT_GUI(text, txt, opt);
			txt.value = string(&letters[i], 1);
		}
	}

	eventListener<bool(uint32)> guiEvent;

	bool guiFunction(uint32 en)
	{
		switch (en)
		{
		case 31:
		{
			CAGE_COMPONENT_GUI(comboBox, control, gui()->entities()->get(en));
			confControlMovement = control.selected;
			return true;
		}
		case 32:
		{
			CAGE_COMPONENT_GUI(comboBox, control, gui()->entities()->get(en));
			confControlFiring = control.selected;
			return true;
		}
		case 33:
		{
			CAGE_COMPONENT_GUI(comboBox, control, gui()->entities()->get(en));
			confControlBomb = control.selected;
			return true;
		}
		case 34:
		{
			CAGE_COMPONENT_GUI(comboBox, control, gui()->entities()->get(en));
			confControlTurret = control.selected;
			return true;
		}
		case 35:
		{
			CAGE_COMPONENT_GUI(comboBox, control, gui()->entities()->get(en));
			confControlDecoy = control.selected;
			return true;
		}
		case 36:
		{
			CAGE_COMPONENT_GUI(sliderBar, control, gui()->entities()->get(en));
			confVolumeMusic = control.value.value;
			return true;
		}
		case 37:
		{
			CAGE_COMPONENT_GUI(sliderBar, control, gui()->entities()->get(en));
			confVolumeEffects = control.value.value;
			return true;
		}
		case 38:
		{
			CAGE_COMPONENT_GUI(sliderBar, control, gui()->entities()->get(en));
			confVolumeSpeech = control.value.value;
			return true;
		}
		}
		return false;
	}
}

void setScreenOptions()
{
	regenerateGui(guiConfig());
	entityManager *ents = gui()->entities();
	guiEvent.bind<&guiFunction>();
	guiEvent.attach(gui()->widgetEvent);

	entity *tabs = ents->createUnique();
	{
		CAGE_COMPONENT_GUI(parent, parent, tabs);
		parent.parent = 12;
		CAGE_COMPONENT_GUI(layoutLine, layout, tabs);
		layout.vertical = false;
	}

	{ // controls
		entity *panel = ents->createUnique();
		{
			CAGE_COMPONENT_GUI(panel, control, panel);
			CAGE_COMPONENT_GUI(parent, parent, panel);
			parent.parent = tabs->name();
			parent.order = 1;
			CAGE_COMPONENT_GUI(layoutTable, layout, panel);
			layout.sections = 2;
			CAGE_COMPONENT_GUI(text, txt, panel);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/controls");
			CAGE_COMPONENT_GUI(scrollbars, sc, panel);
		}
		uint32 index = 0;

		{ // movement
			{
				entity *lbl = ents->createUnique();
				CAGE_COMPONENT_GUI(parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(label, control, lbl);
				CAGE_COMPONENT_GUI(text, txt, lbl);
				txt.assetName = hashString("degrid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/movement");
			}

			{
				entity *ctr = ents->create(31);
				CAGE_COMPONENT_GUI(parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(comboBox, control, ctr);
				control.selected = confControlMovement;
				addOptionsMovementFiring(ctr->name());
			}
		}

		{ // firing
			{
				entity *lbl = ents->createUnique();
				CAGE_COMPONENT_GUI(parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(label, control, lbl);
				CAGE_COMPONENT_GUI(text, txt, lbl);
				txt.assetName = hashString("degrid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/firing");
			}

			{
				entity *ctr = ents->create(32);
				CAGE_COMPONENT_GUI(parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(comboBox, control, ctr);
				control.selected = confControlFiring;
				addOptionsMovementFiring(ctr->name());
			}
		}

		{ // bomb
			{
				entity *lbl = ents->createUnique();
				CAGE_COMPONENT_GUI(parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(label, control, lbl);
				CAGE_COMPONENT_GUI(text, txt, lbl);
				txt.assetName = hashString("degrid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/bomb");
			}

			{
				entity *ctr = ents->create(33);
				CAGE_COMPONENT_GUI(parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(comboBox, control, ctr);
				control.selected = confControlBomb;
				addOptionsPowers(ctr->name());
			}
		}

		{ // turret
			{
				entity *lbl = ents->createUnique();
				CAGE_COMPONENT_GUI(parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(label, control, lbl);
				CAGE_COMPONENT_GUI(text, txt, lbl);
				txt.assetName = hashString("degrid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/turret");
			}

			{
				entity *ctr = ents->create(34);
				CAGE_COMPONENT_GUI(parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(comboBox, control, ctr);
				control.selected = confControlTurret;
				addOptionsPowers(ctr->name());
			}
		}

		{ // decoy
			{
				entity *lbl = ents->createUnique();
				CAGE_COMPONENT_GUI(parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(label, control, lbl);
				CAGE_COMPONENT_GUI(text, txt, lbl);
				txt.assetName = hashString("degrid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/decoy");
			}

			{
				entity *ctr = ents->create(35);
				CAGE_COMPONENT_GUI(parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(comboBox, control, ctr);
				control.selected = confControlDecoy;
				addOptionsPowers(ctr->name());
			}
		}
	}

	{
		entity *panel = ents->createUnique();
		{
			CAGE_COMPONENT_GUI(panel, control, panel);
			CAGE_COMPONENT_GUI(parent, parent, panel);
			parent.parent = tabs->name();
			parent.order = 2;
			CAGE_COMPONENT_GUI(layoutTable, layout, panel);
			layout.sections = 2;
			CAGE_COMPONENT_GUI(text, txt, panel);
			txt.assetName = hashString("degrid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/sounds");
			CAGE_COMPONENT_GUI(scrollbars, sc, panel);
		}
		uint32 index = 0;

		{ // music volume
			{
				entity *lbl = ents->createUnique();
				CAGE_COMPONENT_GUI(parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(label, control, lbl);
				CAGE_COMPONENT_GUI(text, txt, lbl);
				txt.assetName = hashString("degrid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/musicVolume");
			}

			{
				entity *ctr = ents->create(36);
				CAGE_COMPONENT_GUI(parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(sliderBar, control, ctr);
				control.value = real(confVolumeMusic);
			}
		}

		{ // effects volume
			{
				entity *lbl = ents->createUnique();
				CAGE_COMPONENT_GUI(parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(label, control, lbl);
				CAGE_COMPONENT_GUI(text, txt, lbl);
				txt.assetName = hashString("degrid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/effectsVolume");
			}

			{
				entity *ctr = ents->create(37);
				CAGE_COMPONENT_GUI(parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(sliderBar, control, ctr);
				control.value = real(confVolumeEffects);
			}
		}

		{ // speech volume
			{
				entity *lbl = ents->createUnique();
				CAGE_COMPONENT_GUI(parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(label, control, lbl);
				CAGE_COMPONENT_GUI(text, txt, lbl);
				txt.assetName = hashString("degrid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/speechVolume");
			}

			{
				entity *ctr = ents->create(38);
				CAGE_COMPONENT_GUI(parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				CAGE_COMPONENT_GUI(sliderBar, control, ctr);
				control.value = real(confVolumeSpeech);
			}
		}
	}
}

