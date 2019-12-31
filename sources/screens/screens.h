#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/entities.h>
#include <cage-core/hashString.h>

#include <cage-engine/core.h>
#include <cage-engine/engine.h>
#include <cage-engine/window.h>
#include <cage-engine/gui.h>

using namespace cage;

struct GuiConfig
{
	bool backButton;
	bool logo;
	GuiConfig();
};

void regenerateGui(const GuiConfig &config);

void setScreenMainmenu();
void setScreenGame();
void setScreenGameover();
void setScreenOptions();
void setScreenScores();
void setScreenAbout();
void setScreenAchievements();
