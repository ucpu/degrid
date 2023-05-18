#include <cage-core/config.h>
#include <cage-engine/guiManager.h>

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
		EntityManager *ents = engineGuiEntities();
		{
			Entity *opt = ents->createUnique();
			GuiParentComponent &parent = opt->value<GuiParentComponent>();
			parent.parent = ctr;
			parent.order = 0;
			GuiTextComponent &txt = opt->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/arrowsAbsolute");
		}
		{
			Entity *opt = ents->createUnique();
			GuiParentComponent &parent = opt->value<GuiParentComponent>();
			parent.parent = ctr;
			parent.order = 1;
			GuiTextComponent &txt = opt->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/arrowsRelative");
		}
		{
			Entity *opt = ents->createUnique();
			GuiParentComponent &parent = opt->value<GuiParentComponent>();
			parent.parent = ctr;
			parent.order = 2;
			GuiTextComponent &txt = opt->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/lmb");
		}
		{
			Entity *opt = ents->createUnique();
			GuiParentComponent &parent = opt->value<GuiParentComponent>();
			parent.parent = ctr;
			parent.order = 3;
			GuiTextComponent &txt = opt->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/rmb");
		}
		{
			Entity *opt = ents->createUnique();
			GuiParentComponent &parent = opt->value<GuiParentComponent>();
			parent.parent = ctr;
			parent.order = 4;
			GuiTextComponent &txt = opt->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/cursor");
		}
		{
			Entity *opt = ents->createUnique();
			GuiParentComponent &parent = opt->value<GuiParentComponent>();
			parent.parent = ctr;
			parent.order = 5;
			GuiTextComponent &txt = opt->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/leftStick");
		}
		{
			Entity *opt = ents->createUnique();
			GuiParentComponent &parent = opt->value<GuiParentComponent>();
			parent.parent = ctr;
			parent.order = 6;
			GuiTextComponent &txt = opt->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/rightStick");
		}
	}

	void addOptionsPowers(uint32 ctr)
	{
		EntityManager *ents = engineGuiEntities();
		{
			Entity *opt = ents->createUnique();
			GuiParentComponent &parent = opt->value<GuiParentComponent>();
			parent.parent = ctr;
			parent.order = -4;
			GuiTextComponent &txt = opt->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/lmb");
		}
		{
			Entity *opt = ents->createUnique();
			GuiParentComponent &parent = opt->value<GuiParentComponent>();
			parent.parent = ctr;
			parent.order = -3;
			GuiTextComponent &txt = opt->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/mmb");
		}
		{
			Entity *opt = ents->createUnique();
			GuiParentComponent &parent = opt->value<GuiParentComponent>();
			parent.parent = ctr;
			parent.order = -2;
			GuiTextComponent &txt = opt->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/rmb");
		}
		{
			Entity *opt = ents->createUnique();
			GuiParentComponent &parent = opt->value<GuiParentComponent>();
			parent.parent = ctr;
			parent.order = -1;
			GuiTextComponent &txt = opt->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/space");
		}
		for (sint32 i = 0; i < sizeof(Letters); i++)
		{
			Entity *opt = ents->createUnique();
			GuiParentComponent &parent = opt->value<GuiParentComponent>();
			parent.parent = ctr;
			parent.order = Letters[i];
			opt->value<GuiTextComponent>().value = String(Letters[i]);
		}
		for (uint32 i = 0; i < 4; i++)
		{
			Entity *opt = ents->createUnique();
			GuiParentComponent &parent = opt->value<GuiParentComponent>();
			parent.parent = ctr;
			parent.order = 1000 + i;
			static constexpr const char *const names[] = { "Gamepad A", "Gamepad B", "Gamepad X", "Gamepad Y" };
			opt->value<GuiTextComponent>().value = names[i];
		}
	}

	InputListener<InputClassEnum::GuiWidget, InputGuiWidget, bool> guiEvent;

	bool guiFunction(InputGuiWidget in)
	{
		switch (in.widget)
		{
		case 31:
		{
			GuiComboBoxComponent &control = engineGuiEntities()->get(in.widget)->value<GuiComboBoxComponent>();
			confControlMovement = control.selected;
			return true;
		}
		case 32:
		{
			GuiComboBoxComponent &control = engineGuiEntities()->get(in.widget)->value<GuiComboBoxComponent>();
			confControlFiring = control.selected;
			return true;
		}
		case 33:
		{
			GuiComboBoxComponent &control = engineGuiEntities()->get(in.widget)->value<GuiComboBoxComponent>();
			confControlBomb = control.selected;
			return true;
		}
		case 34:
		{
			GuiComboBoxComponent &control = engineGuiEntities()->get(in.widget)->value<GuiComboBoxComponent>();
			confControlTurret = control.selected;
			return true;
		}
		case 35:
		{
			GuiComboBoxComponent &control = engineGuiEntities()->get(in.widget)->value<GuiComboBoxComponent>();
			confControlDecoy = control.selected;
			return true;
		}
		case 36:
		{
			GuiSliderBarComponent &control = engineGuiEntities()->get(in.widget)->value<GuiSliderBarComponent>();
			confVolumeMusic = control.value.value;
			return true;
		}
		case 37:
		{
			GuiSliderBarComponent &control = engineGuiEntities()->get(in.widget)->value<GuiSliderBarComponent>();
			confVolumeEffects = control.value.value;
			return true;
		}
		case 38:
		{
			GuiSliderBarComponent &control = engineGuiEntities()->get(in.widget)->value<GuiSliderBarComponent>();
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
	EntityManager *ents = engineGuiEntities();
	guiEvent.bind<&guiFunction>();
	guiEvent.attach(engineGuiManager()->widgetEvent);

	Entity *tabs = ents->createUnique();
	{
		tabs->value<GuiParentComponent>().parent = 12;
		tabs->value<GuiLayoutLineComponent>().vertical = false;
	}

	{ // controls
		Entity *panel = ents->createUnique();
		{
			panel->value<GuiPanelComponent>();
			GuiParentComponent &parent = panel->value<GuiParentComponent>();
			parent.parent = tabs->name();
			parent.order = 1;
			panel->value<GuiLayoutTableComponent>().sections = 2;
			GuiTextComponent &txt = panel->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/controls");
			panel->value<GuiLayoutScrollbarsComponent>();
		}
		uint32 index = 0;

		{ // movement
			{
				Entity *lbl = ents->createUnique();
				GuiParentComponent &parent = lbl->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = index++;
				lbl->value<GuiLabelComponent>();
				GuiTextComponent &txt = lbl->value<GuiTextComponent>();
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/options/movement");
			}

			{
				Entity *ctr = ents->create(31);
				GuiParentComponent &parent = ctr->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = index++;
				ctr->value<GuiComboBoxComponent>().selected = confControlMovement;
				addOptionsMovementFiring(ctr->name());
			}
		}

		{ // firing
			{
				Entity *lbl = ents->createUnique();
				GuiParentComponent &parent = lbl->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = index++;
				lbl->value<GuiLabelComponent>();
				GuiTextComponent &txt = lbl->value<GuiTextComponent>();
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/options/firing");
			}

			{
				Entity *ctr = ents->create(32);
				GuiParentComponent &parent = ctr->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = index++;
				ctr->value<GuiComboBoxComponent>().selected = confControlFiring;
				addOptionsMovementFiring(ctr->name());
			}
		}

		{ // bomb
			{
				Entity *lbl = ents->createUnique();
				GuiParentComponent &parent = lbl->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = index++;
				lbl->value<GuiLabelComponent>();
				GuiTextComponent &txt = lbl->value<GuiTextComponent>();
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/options/bomb");
			}

			{
				Entity *ctr = ents->create(33);
				GuiParentComponent &parent = ctr->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = index++;
				ctr->value<GuiComboBoxComponent>().selected = confControlBomb;
				addOptionsPowers(ctr->name());
			}
		}

		{ // turret
			{
				Entity *lbl = ents->createUnique();
				GuiParentComponent &parent = lbl->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = index++;
				lbl->value<GuiLabelComponent>();
				GuiTextComponent &txt = lbl->value<GuiTextComponent>();
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/options/turret");
			}

			{
				Entity *ctr = ents->create(34);
				GuiParentComponent &parent = ctr->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = index++;
				ctr->value<GuiComboBoxComponent>().selected = confControlTurret;
				addOptionsPowers(ctr->name());
			}
		}

		{ // decoy
			{
				Entity *lbl = ents->createUnique();
				GuiParentComponent &parent = lbl->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = index++;
				lbl->value<GuiLabelComponent>();
				GuiTextComponent &txt = lbl->value<GuiTextComponent>();
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/options/decoy");
			}

			{
				Entity *ctr = ents->create(35);
				GuiParentComponent &parent = ctr->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = index++;
				ctr->value<GuiComboBoxComponent>().selected = confControlDecoy;
				addOptionsPowers(ctr->name());
			}
		}
	}

	{
		Entity *panel = ents->createUnique();
		{
			panel->value<GuiPanelComponent>();
			GuiParentComponent &parent = panel->value<GuiParentComponent>();
			parent.parent = tabs->name();
			parent.order = 2;
			panel->value<GuiLayoutTableComponent>().sections = 2;
			GuiTextComponent &txt = panel->value<GuiTextComponent>();
			txt.assetName = HashString("degrid/languages/internationalized.textpack");
			txt.textName = HashString("gui/options/sounds");
			panel->value<GuiLayoutScrollbarsComponent>();
		}
		uint32 index = 0;

		{ // music volume
			{
				Entity *lbl = ents->createUnique();
				GuiParentComponent &parent = lbl->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = index++;
				lbl->value<GuiLabelComponent>();
				GuiTextComponent &txt = lbl->value<GuiTextComponent>();
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/options/musicVolume");
			}

			{
				Entity *ctr = ents->create(36);
				GuiParentComponent &parent = ctr->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = index++;
				ctr->value<GuiSliderBarComponent>().value = Real(confVolumeMusic);
			}
		}

		{ // effects volume
			{
				Entity *lbl = ents->createUnique();
				GuiParentComponent &parent = lbl->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = index++;
				lbl->value<GuiLabelComponent>();
				GuiTextComponent &txt = lbl->value<GuiTextComponent>();
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/options/effectsVolume");
			}

			{
				Entity *ctr = ents->create(37);
				GuiParentComponent &parent = ctr->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = index++;
				ctr->value<GuiSliderBarComponent>().value = Real(confVolumeEffects);
			}
		}

		{ // speech volume
			{
				Entity *lbl = ents->createUnique();
				GuiParentComponent &parent = lbl->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = index++;
				lbl->value<GuiLabelComponent>();
				GuiTextComponent &txt = lbl->value<GuiTextComponent>();
				txt.assetName = HashString("degrid/languages/internationalized.textpack");
				txt.textName = HashString("gui/options/speechVolume");
			}

			{
				Entity *ctr = ents->create(38);
				GuiParentComponent &parent = ctr->value<GuiParentComponent>();
				parent.parent = panel->name();
				parent.order = index++;
				ctr->value<GuiSliderBarComponent>().value = Real(confVolumeSpeech);
			}
		}
	}
}

