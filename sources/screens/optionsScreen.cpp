#include "../includes.h"
#include "../screens.h"
#include "../game.h"

namespace
{
	void addOptionsMovementFiring(uint32 ctr)
	{
		entityManagerClass *ents = gui()->entities();
		{
			entityClass *opt = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.ordering = 0;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/arrowsAbsolute");
		}
		{
			entityClass *opt = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.ordering = 1;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/arrowsRelative");
		}
		{
			entityClass *opt = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.ordering = 2;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/lmb");
		}
		{
			entityClass *opt = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.ordering = 3;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/rmb");
		}
		{
			entityClass *opt = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.ordering = 4;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/cursor");
		}
	}

	void addOptionsPowers(uint32 ctr)
	{
		entityManagerClass *ents = gui()->entities();
		{
			entityClass *opt = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.ordering = -4;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/lmb");
		}
		{
			entityClass *opt = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.ordering = -3;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/mmb");
		}
		{
			entityClass *opt = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.ordering = -2;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/rmb");
		}
		{
			entityClass *opt = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.ordering = -1;
			GUI_GET_COMPONENT(text, txt, opt);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/space");
		}
		for (sint32 i = 0; i < sizeof(grid::letters); i++)
		{
			entityClass *opt = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, parent, opt);
			parent.parent = ctr;
			parent.ordering = grid::letters[i];
			GUI_GET_COMPONENT(text, txt, opt);
			txt.value = string(&grid::letters[i], 1);
		}
	}

	eventListener<bool(uint32)> guiEvent;

	bool guiFunction(uint32 en)
	{
		GUI_GET_COMPONENT(control, control, gui()->entities()->getEntity(en));
		switch (en)
		{
		case 31:
			grid::confControlMovement = control.ival;
			return true;
		case 32:
			grid::confControlFiring = control.ival;
			return true;
		case 33:
			grid::confControlBomb = control.ival;
			return true;
		case 34:
			grid::confControlTurret = control.ival;
			return true;
		case 35:
			grid::confControlDecoy = control.ival;
			return true;
		case 36:
			grid::confVolumeMusic = control.fval;
			return true;
		case 37:
			grid::confVolumeEffects = control.fval;
			return true;
		case 38:
			grid::confVolumeSpeech = control.fval;
			return true;
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
	guiEvent.attach(gui()->genericEvent);
	real txtSize = gui()->elements[(int)elementTypeEnum::ButtonNormal].defaultSize[0];

	entityClass *tabs = ents->newEntity(ents->generateUniqueName());
	{
		GUI_GET_COMPONENT(layout, layout, tabs);
		layout.layoutMode = layoutModeEnum::Column;
		GUI_GET_COMPONENT(position, position, tabs);
		position.x = 0.005;
		position.y = 0.005;
		position.xUnit = unitsModeEnum::ScreenHeight;
		position.yUnit = unitsModeEnum::ScreenHeight;
	}

	{ // controls
		entityClass *panel = ents->newEntity(ents->generateUniqueName());
		{
			GUI_GET_COMPONENT(control, control, panel);
			control.controlType = controlTypeEnum::Panel;
			GUI_GET_COMPONENT(parent, parent, panel);
			parent.parent = tabs->getName();
		}

		entityClass *column = ents->newEntity(ents->generateUniqueName());
		{
			GUI_GET_COMPONENT(parent, parent, column);
			parent.parent = panel->getName();
			GUI_GET_COMPONENT(layout, layout, column);
			layout.layoutMode = layoutModeEnum::Column;
		}

		{ // controls label
			entityClass *lbl = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, parent, lbl);
			parent.parent = column->getName();
			parent.ordering = 0;
			GUI_GET_COMPONENT(control, control, lbl);
			control.controlType = controlTypeEnum::Empty;
			GUI_GET_COMPONENT(position, pos, lbl);
			pos.w = txtSize;
			pos.wUnit = unitsModeEnum::Pixels;
			GUI_GET_COMPONENT(text, txt, lbl);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/controls");
		}

		{ // movement
			entityClass *row = ents->newEntity(ents->generateUniqueName());
			{
				GUI_GET_COMPONENT(parent, parent, row);
				parent.parent = column->getName();
				parent.ordering = 1;
				GUI_GET_COMPONENT(layout, layout, row);
				layout.layoutMode = layoutModeEnum::Row;
			}

			{
				entityClass *lbl = ents->newEntity(ents->generateUniqueName());
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = row->getName();
				parent.ordering = 0;
				GUI_GET_COMPONENT(control, control, lbl);
				control.controlType = controlTypeEnum::Empty;
				GUI_GET_COMPONENT(position, pos, lbl);
				pos.w = txtSize;
				pos.wUnit = unitsModeEnum::Pixels;
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/movement");
			}

			{
				entityClass *ctr = ents->newEntity(31);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = row->getName();
				parent.ordering = 1;
				GUI_GET_COMPONENT(control, control, ctr);
				control.controlType = controlTypeEnum::Combobox;
				control.ival = grid::confControlMovement;

				addOptionsMovementFiring(ctr->getName());
			}
		}

		{ // firing
			entityClass *row = ents->newEntity(ents->generateUniqueName());
			{
				GUI_GET_COMPONENT(parent, parent, row);
				parent.parent = column->getName();
				parent.ordering = 2;
				GUI_GET_COMPONENT(layout, layout, row);
				layout.layoutMode = layoutModeEnum::Row;
			}

			{
				entityClass *lbl = ents->newEntity(ents->generateUniqueName());
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = row->getName();
				parent.ordering = 0;
				GUI_GET_COMPONENT(control, control, lbl);
				control.controlType = controlTypeEnum::Empty;
				GUI_GET_COMPONENT(position, pos, lbl);
				pos.w = txtSize;
				pos.wUnit = unitsModeEnum::Pixels;
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/firing");
			}

			{
				entityClass *ctr = ents->newEntity(32);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = row->getName();
				parent.ordering = 1;
				GUI_GET_COMPONENT(control, control, ctr);
				control.controlType = controlTypeEnum::Combobox;
				control.ival = grid::confControlFiring;

				addOptionsMovementFiring(ctr->getName());
			}
		}

		{ // bomb
			entityClass *row = ents->newEntity(ents->generateUniqueName());
			{
				GUI_GET_COMPONENT(parent, parent, row);
				parent.parent = column->getName();
				parent.ordering = 3;
				GUI_GET_COMPONENT(layout, layout, row);
				layout.layoutMode = layoutModeEnum::Row;
			}

			{
				entityClass *lbl = ents->newEntity(ents->generateUniqueName());
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = row->getName();
				parent.ordering = 0;
				GUI_GET_COMPONENT(control, control, lbl);
				control.controlType = controlTypeEnum::Empty;
				GUI_GET_COMPONENT(position, pos, lbl);
				pos.w = txtSize;
				pos.wUnit = unitsModeEnum::Pixels;
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/bomb");
			}

			{
				entityClass *ctr = ents->newEntity(33);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = row->getName();
				parent.ordering = 1;
				GUI_GET_COMPONENT(control, control, ctr);
				control.controlType = controlTypeEnum::Combobox;
				control.ival = grid::confControlBomb;

				addOptionsPowers(ctr->getName());
			}
		}

		{ // turret
			entityClass *row = ents->newEntity(ents->generateUniqueName());
			{
				GUI_GET_COMPONENT(parent, parent, row);
				parent.parent = column->getName();
				parent.ordering = 4;
				GUI_GET_COMPONENT(layout, layout, row);
				layout.layoutMode = layoutModeEnum::Row;
			}

			{
				entityClass *lbl = ents->newEntity(ents->generateUniqueName());
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = row->getName();
				parent.ordering = 0;
				GUI_GET_COMPONENT(control, control, lbl);
				control.controlType = controlTypeEnum::Empty;
				GUI_GET_COMPONENT(position, pos, lbl);
				pos.w = txtSize;
				pos.wUnit = unitsModeEnum::Pixels;
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/turret");
			}

			{
				entityClass *ctr = ents->newEntity(34);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = row->getName();
				parent.ordering = 1;
				GUI_GET_COMPONENT(control, control, ctr);
				control.controlType = controlTypeEnum::Combobox;
				control.ival = grid::confControlTurret;

				addOptionsPowers(ctr->getName());
			}
		}

		{ // decoy
			entityClass *row = ents->newEntity(ents->generateUniqueName());
			{
				GUI_GET_COMPONENT(parent, parent, row);
				parent.parent = column->getName();
				parent.ordering = 5;
				GUI_GET_COMPONENT(layout, layout, row);
				layout.layoutMode = layoutModeEnum::Row;
			}

			{
				entityClass *lbl = ents->newEntity(ents->generateUniqueName());
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = row->getName();
				parent.ordering = 0;
				GUI_GET_COMPONENT(control, control, lbl);
				control.controlType = controlTypeEnum::Empty;
				GUI_GET_COMPONENT(position, pos, lbl);
				pos.w = txtSize;
				pos.wUnit = unitsModeEnum::Pixels;
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/decoy");
			}

			{
				entityClass *ctr = ents->newEntity(35);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = row->getName();
				parent.ordering = 1;
				GUI_GET_COMPONENT(control, control, ctr);
				control.controlType = controlTypeEnum::Combobox;
				control.ival = grid::confControlDecoy;

				addOptionsPowers(ctr->getName());
			}
		}

		{ // sound label
			entityClass *lbl = ents->newEntity(ents->generateUniqueName());
			GUI_GET_COMPONENT(parent, parent, lbl);
			parent.parent = column->getName();
			parent.ordering = 10;
			GUI_GET_COMPONENT(control, control, lbl);
			control.controlType = controlTypeEnum::Empty;
			GUI_GET_COMPONENT(position, pos, lbl);
			pos.w = txtSize;
			pos.wUnit = unitsModeEnum::Pixels;
			GUI_GET_COMPONENT(text, txt, lbl);
			txt.assetName = hashString("grid/languages/internationalized.textpack");
			txt.textName = hashString("gui/options/sounds");
		}

		{ // music volume
			entityClass *row = ents->newEntity(ents->generateUniqueName());
			{
				GUI_GET_COMPONENT(parent, parent, row);
				parent.parent = column->getName();
				parent.ordering = 11;
				GUI_GET_COMPONENT(layout, layout, row);
				layout.layoutMode = layoutModeEnum::Row;
			}

			{
				entityClass *lbl = ents->newEntity(ents->generateUniqueName());
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = row->getName();
				parent.ordering = 0;
				GUI_GET_COMPONENT(control, control, lbl);
				control.controlType = controlTypeEnum::Empty;
				GUI_GET_COMPONENT(position, pos, lbl);
				pos.w = txtSize;
				pos.wUnit = unitsModeEnum::Pixels;
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/musicVolume");
			}

			{
				entityClass *ctr = ents->newEntity(36);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = row->getName();
				parent.ordering = 1;
				GUI_GET_COMPONENT(control, control, ctr);
				control.controlType = controlTypeEnum::Slider;
				control.fval = grid::confVolumeMusic;
			}
		}

		{ // effects volume
			entityClass *row = ents->newEntity(ents->generateUniqueName());
			{
				GUI_GET_COMPONENT(parent, parent, row);
				parent.parent = column->getName();
				parent.ordering = 12;
				GUI_GET_COMPONENT(layout, layout, row);
				layout.layoutMode = layoutModeEnum::Row;
			}

			{
				entityClass *lbl = ents->newEntity(ents->generateUniqueName());
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = row->getName();
				parent.ordering = 0;
				GUI_GET_COMPONENT(control, control, lbl);
				control.controlType = controlTypeEnum::Empty;
				GUI_GET_COMPONENT(position, pos, lbl);
				pos.w = txtSize;
				pos.wUnit = unitsModeEnum::Pixels;
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/effectsVolume");
			}

			{
				entityClass *ctr = ents->newEntity(37);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = row->getName();
				parent.ordering = 1;
				GUI_GET_COMPONENT(control, control, ctr);
				control.controlType = controlTypeEnum::Slider;
				control.fval = grid::confVolumeEffects;
			}
		}

		{ // speech volume
			entityClass *row = ents->newEntity(ents->generateUniqueName());
			{
				GUI_GET_COMPONENT(parent, parent, row);
				parent.parent = column->getName();
				parent.ordering = 13;
				GUI_GET_COMPONENT(layout, layout, row);
				layout.layoutMode = layoutModeEnum::Row;
			}

			{
				entityClass *lbl = ents->newEntity(ents->generateUniqueName());
				GUI_GET_COMPONENT(parent, parent, lbl);
				parent.parent = row->getName();
				parent.ordering = 0;
				GUI_GET_COMPONENT(control, control, lbl);
				control.controlType = controlTypeEnum::Empty;
				GUI_GET_COMPONENT(position, pos, lbl);
				pos.w = txtSize;
				pos.wUnit = unitsModeEnum::Pixels;
				GUI_GET_COMPONENT(text, txt, lbl);
				txt.assetName = hashString("grid/languages/internationalized.textpack");
				txt.textName = hashString("gui/options/speechVolume");
			}

			{
				entityClass *ctr = ents->newEntity(38);
				GUI_GET_COMPONENT(parent, parent, ctr);
				parent.parent = row->getName();
				parent.ordering = 1;
				GUI_GET_COMPONENT(control, control, ctr);
				control.controlType = controlTypeEnum::Slider;
				control.fval = grid::confVolumeSpeech;
			}
		}
	}
}