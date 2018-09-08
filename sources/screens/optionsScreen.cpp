#include "../includes.h"
#include "../screens.h"
#include "../game.h"

namespace
{
	void addOptionsMovementFiring(uint32 ctr)
	{
		entityManagerClass *ents = gui()->entities();
		{
			entityClass *opt = ents->newUniqueEntity();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = 0;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/arrowsAbsolute");
		}
		{
			entityClass *opt = ents->newUniqueEntity();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = 1;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/arrowsRelative");
		}
		{
			entityClass *opt = ents->newUniqueEntity();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = 2;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/lmb");
		}
		{
			entityClass *opt = ents->newUniqueEntity();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = 3;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/rmb");
		}
		{
			entityClass *opt = ents->newUniqueEntity();
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
			entityClass *opt = ents->newUniqueEntity();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = -4;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/lmb");
		}
		{
			entityClass *opt = ents->newUniqueEntity();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = -3;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/mmb");
		}
		{
			entityClass *opt = ents->newUniqueEntity();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = -2;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/rmb");
		}
		{
			entityClass *opt = ents->newUniqueEntity();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = -1;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/space");
		}
		for (sint32 i = 0; i < sizeof(grid::letters); i++)
		{
			entityClass *opt = ents->newUniqueEntity();
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.order = grid::letters[i];
			GUI_GET_COMPONENT(text, txt, opt);
			txt.value = string(&grid::letters[i], 1);
		}
	}

	eventListener<bool(uint32)> guiEvent;

	bool guiFunction(uint32 en)
	{
		switch (en)
		{
		case 31:
		{
			GUI_GET_COMPONENT(comboBox, control, gui()->entities()->getEntity(en));
			grid::confControlMovement = control.selected;
			return true;
		}
		case 32:
		{
			GUI_GET_COMPONENT(comboBox, control, gui()->entities()->getEntity(en));
			grid::confControlFiring = control.selected;
			return true;
		}
		case 33:
		{
			GUI_GET_COMPONENT(comboBox, control, gui()->entities()->getEntity(en));
			grid::confControlBomb = control.selected;
			return true;
		}
		case 34:
		{
			GUI_GET_COMPONENT(comboBox, control, gui()->entities()->getEntity(en));
			grid::confControlTurret = control.selected;
			return true;
		}
		case 35:
		{
			GUI_GET_COMPONENT(comboBox, control, gui()->entities()->getEntity(en));
			grid::confControlDecoy = control.selected;
			return true;
		}
		case 36:
		{
			GUI_GET_COMPONENT(sliderBar, control, gui()->entities()->getEntity(en));
			grid::confVolumeMusic = control.value.value;
			return true;
		}
		case 37:
		{
			GUI_GET_COMPONENT(sliderBar, control, gui()->entities()->getEntity(en));
			grid::confVolumeEffects = control.value.value;
			return true;
		}
		case 38:
		{
			GUI_GET_COMPONENT(sliderBar, control, gui()->entities()->getEntity(en));
			grid::confVolumeSpeech = control.value.value;
			return true;
		}
		}
		return false;
	}
}

void setScreenOptions()
{
	eraseGui();
	generateLogo();
	generateButtonBack();
	entityManagerClass *ents = gui()->entities();
	guiEvent.bind<&guiFunction>();
	guiEvent.attach(gui()->widgetEvent);

	entityClass *tabs = ents->newUniqueEntity();
	{
		GUI_GET_COMPONENT(position, position, tabs);
		position.size.values[1] = 1;
		position.size.units[1] = unitEnum::ScreenHeight;
		GUI_GET_COMPONENT(layoutLine, layout, tabs);
		layout.vertical = true;
		layout.expandToSameWidth = true;
	}

	{ // controls
		entityClass *panel = ents->newUniqueEntity();
		{
			GUI_GET_COMPONENT(groupBox, control, panel);
			GUI_GET_COMPONENT(parent, parent, panel);
			parent.parent = tabs->getName();
			parent.order = 1;
			GUI_GET_COMPONENT(layoutTable, layout, panel);
			layout.sections = 2;
			GUI_GET_COMPONENT(text, txt, panel);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/controls");
		}
		uint32 index = 0;

		{ // movement
			{
				entityClass *lbl = ents->newUniqueEntity();
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, lbl);
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/movement");
			}

			{
				entityClass *ctr = ents->newEntity(31);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(comboBox, control, ctr);
				control.selected = grid::confControlMovement;
				addOptionsMovementFiring(ctr->getName());
			}
		}

		{ // firing
			{
				entityClass *lbl = ents->newUniqueEntity();
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, lbl);
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/firing");
			}

			{
				entityClass *ctr = ents->newEntity(32);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(comboBox, control, ctr);
				control.selected = grid::confControlFiring;
				addOptionsMovementFiring(ctr->getName());
			}
		}

		{ // bomb
			{
				entityClass *lbl = ents->newUniqueEntity();
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, lbl);
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/bomb");
			}

			{
				entityClass *ctr = ents->newEntity(33);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(comboBox, control, ctr);
				control.selected = grid::confControlBomb;
				addOptionsPowers(ctr->getName());
			}
		}

		{ // turret
			{
				entityClass *lbl = ents->newUniqueEntity();
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, lbl);
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/turret");
			}

			{
				entityClass *ctr = ents->newEntity(34);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(comboBox, control, ctr);
				control.selected = grid::confControlTurret;
				addOptionsPowers(ctr->getName());
			}
		}

		{ // decoy
			{
				entityClass *lbl = ents->newUniqueEntity();
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, lbl);
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/decoy");
			}

			{
				entityClass *ctr = ents->newEntity(35);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(comboBox, control, ctr);
				control.selected = grid::confControlDecoy;
				addOptionsPowers(ctr->getName());
			}
		}
	}

	{
		entityClass *panel = ents->newUniqueEntity();
		{
			GUI_GET_COMPONENT(groupBox, control, panel);
			GUI_GET_COMPONENT(parent, parent, panel);
			parent.parent = tabs->getName();
			parent.order = 2;
			GUI_GET_COMPONENT(layoutTable, layout, panel);
			layout.sections = 2;
			GUI_GET_COMPONENT(text, txt, panel);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/sounds");
		}
		uint32 index = 0;

		{ // music volume
			{
				entityClass *lbl = ents->newUniqueEntity();
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, lbl);
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/musicVolume");
			}

			{
				entityClass *ctr = ents->newEntity(36);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(sliderBar, control, ctr);
				control.value = real(grid::confVolumeMusic);
			}
		}

		{ // effects volume
			{
				entityClass *lbl = ents->newUniqueEntity();
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, lbl);
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/effectsVolume");
			}

			{
				entityClass *ctr = ents->newEntity(37);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(sliderBar, control, ctr);
				control.value = real(grid::confVolumeEffects);
			}
		}

		{ // speech volume
			{
				entityClass *lbl = ents->newUniqueEntity();
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(label, control, lbl);
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/speechVolume");
			}

			{
				entityClass *ctr = ents->newEntity(38);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = panel->getName();
				parent.order = index++;
				GUI_GET_COMPONENT(sliderBar, control, ctr);
				control.value = real(grid::confVolumeSpeech);
			}
		}
	}

	gui()->skipAllEventsUntilNextUpdate();
}

