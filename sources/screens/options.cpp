#include "screens.h"
#include "../game.h"

#include <cage-core/config.h>

configUint32 confControlMovement("grid.control.movement", 0);
configUint32 confControlFiring("grid.control.firing", 4);
configUint32 confControlBomb("grid.control.bomb", 4 + 4);
configUint32 confControlTurret("grid.control.turret", 2 + 4);
configUint32 confControlDecoy("grid.control.decoy", 0 + 4);
configString confPlayerName("grid.player.name", "player name");
configFloat confPlayerShotColorR("grid.player.shotColorR", 1);
configFloat confPlayerShotColorG("grid.player.shotColorG", 1);
configFloat confPlayerShotColorB("grid.player.shotColorB", 1);
configFloat confVolumeMusic("grid.volume.music", 0.5f);
configFloat confVolumeEffects("grid.volume.effects", 0.5f);
configFloat confVolumeSpeech("grid.volume.speech", 0.7f);

namespace
{
	void addOptionsMovementFiring(uint32 ctr)
	{
		entityManagerClass *ents = gui()->entities();
		{
			entityClass *opt = ents->createUnique();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = 0;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/arrowsAbsolute");
		}
		{
			entityClass *opt = ents->createUnique();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = 1;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/arrowsRelative");
		}
		{
			entityClass *opt = ents->createUnique();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = 2;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/lmb");
		}
		{
			entityClass *opt = ents->createUnique();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = 3;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/rmb");
		}
		{
			entityClass *opt = ents->createUnique();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = 4;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/cursor");
		}
	}

	void addOptionsPowers(uint32 ctr)
	{
		entityManagerClass *ents = gui()->entities();
		{
			entityClass *opt = ents->createUnique();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = -4;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/lmb");
		}
		{
			entityClass *opt = ents->createUnique();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = -3;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/mmb");
		}
		{
			entityClass *opt = ents->createUnique();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = -2;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/rmb");
		}
		{
			entityClass *opt = ents->createUnique();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = -1;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/space");
		}
		for (sint32 i = 0; i < sizeof(letters); i++)
		{
			entityClass *opt = ents->createUnique();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = letters[i];
			GUI_GET_COMPONENT(text, txt, opt);
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
			GUI_GET_COMPONENT(comboBox, control, gui()->entities()->get(en));
			confControlMovement = control.selected;
			return true;
		}
		case 32:
		{
			GUI_GET_COMPONENT(comboBox, control, gui()->entities()->get(en));
			confControlFiring = control.selected;
			return true;
		}
		case 33:
		{
			GUI_GET_COMPONENT(comboBox, control, gui()->entities()->get(en));
			confControlBomb = control.selected;
			return true;
		}
		case 34:
		{
			GUI_GET_COMPONENT(comboBox, control, gui()->entities()->get(en));
			confControlTurret = control.selected;
			return true;
		}
		case 35:
		{
			GUI_GET_COMPONENT(comboBox, control, gui()->entities()->get(en));
			confControlDecoy = control.selected;
			return true;
		}
		case 36:
		{
			GUI_GET_COMPONENT(sliderBar, control, gui()->entities()->get(en));
			confVolumeMusic = control.value.value;
			return true;
		}
		case 37:
		{
			GUI_GET_COMPONENT(sliderBar, control, gui()->entities()->get(en));
			confVolumeEffects = control.value.value;
			return true;
		}
		case 38:
		{
			GUI_GET_COMPONENT(sliderBar, control, gui()->entities()->get(en));
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
	entityManagerClass *ents = gui()->entities();
	guiEvent.bind<&guiFunction>();
	guiEvent.attach(gui()->widgetEvent);

	entityClass *tabs = ents->createUnique();
	{
		GUI_GET_COMPONENT(parent, parent, tabs);
		parent.parent = 12;
		GUI_GET_COMPONENT(layoutLine, layout, tabs);
		layout.vertical = false;
	}

	{ // controls
		entityClass *panel = ents->createUnique();
		{
			GUI_GET_COMPONENT(panel, control, panel);
			GUI_GET_COMPONENT(parent, parent, panel);
			parent.parent = tabs->name();
			parent.order = 1;
			GUI_GET_COMPONENT(layoutTable, layout, panel);
			layout.sections = 2;
			GUI_GET_COMPONENT(text, txt, panel);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/controls");
			GUI_GET_COMPONENT(scrollbars, sc, panel);
		}
		uint32 index = 0;

		{ // movement
			{
				entityClass *lbl = ents->createUnique();
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, lbl);
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/movement");
			}

			{
				entityClass *ctr = ents->create(31);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				GUI_GET_COMPONENT(comboBox, control, ctr);
				control.selected = confControlMovement;
				addOptionsMovementFiring(ctr->name());
			}
		}

		{ // firing
			{
				entityClass *lbl = ents->createUnique();
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, lbl);
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/firing");
			}

			{
				entityClass *ctr = ents->create(32);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				GUI_GET_COMPONENT(comboBox, control, ctr);
				control.selected = confControlFiring;
				addOptionsMovementFiring(ctr->name());
			}
		}

		{ // bomb
			{
				entityClass *lbl = ents->createUnique();
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, lbl);
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/bomb");
			}

			{
				entityClass *ctr = ents->create(33);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				GUI_GET_COMPONENT(comboBox, control, ctr);
				control.selected = confControlBomb;
				addOptionsPowers(ctr->name());
			}
		}

		{ // turret
			{
				entityClass *lbl = ents->createUnique();
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, lbl);
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/turret");
			}

			{
				entityClass *ctr = ents->create(34);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				GUI_GET_COMPONENT(comboBox, control, ctr);
				control.selected = confControlTurret;
				addOptionsPowers(ctr->name());
			}
		}

		{ // decoy
			{
				entityClass *lbl = ents->createUnique();
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, lbl);
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/decoy");
			}

			{
				entityClass *ctr = ents->create(35);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				GUI_GET_COMPONENT(comboBox, control, ctr);
				control.selected = confControlDecoy;
				addOptionsPowers(ctr->name());
			}
		}
	}

	{
		entityClass *panel = ents->createUnique();
		{
			GUI_GET_COMPONENT(panel, control, panel);
			GUI_GET_COMPONENT(parent, parent, panel);
			parent.parent = tabs->name();
			parent.order = 2;
			GUI_GET_COMPONENT(layoutTable, layout, panel);
			layout.sections = 2;
			GUI_GET_COMPONENT(text, txt, panel);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/sounds");
			GUI_GET_COMPONENT(scrollbars, sc, panel);
		}
		uint32 index = 0;

		{ // music volume
			{
				entityClass *lbl = ents->createUnique();
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, lbl);
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/musicVolume");
			}

			{
				entityClass *ctr = ents->create(36);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				GUI_GET_COMPONENT(sliderBar, control, ctr);
				control.value = real(confVolumeMusic);
			}
		}

		{ // effects volume
			{
				entityClass *lbl = ents->createUnique();
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, lbl);
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/effectsVolume");
			}

			{
				entityClass *ctr = ents->create(37);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				GUI_GET_COMPONENT(sliderBar, control, ctr);
				control.value = real(confVolumeEffects);
			}
		}

		{ // speech volume
			{
				entityClass *lbl = ents->createUnique();
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = panel->name();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, lbl);
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/speechVolume");
			}

			{
				entityClass *ctr = ents->create(38);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = panel->name();
				parent.order = index++;
				GUI_GET_COMPONENT(sliderBar, control, ctr);
				control.value = real(confVolumeSpeech);
			}
		}
	}
}

