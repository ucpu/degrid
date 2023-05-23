#include <cage-core/entities.h>
#include <cage-core/hashString.h>
#include <cage-engine/window.h>
#include <cage-engine/guiBuilder.h>
#include <cage-engine/guiManager.h>
#include <cage-simple/engine.h>

using namespace cage;

struct GuiConfig
{
	bool backButton = true;
	bool logo = true;
};

void regenerateGui(const GuiConfig &config);

void setScreenMainmenu();
void setScreenGame();
void setScreenGameover();
void setScreenOptions();
void setScreenScores();
void setScreenAbout();
void setScreenAchievements();
