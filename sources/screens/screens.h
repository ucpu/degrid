#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/entities.h>
#include <cage-core/hashString.h>

#include <cage-engine/core.h>
#include <cage-engine/engine.h>
#include <cage-engine/window.h>
#include <cage-engine/gui.h>

using namespace cage;

struct guiConfig
{
	bool backButton;
	bool logo;
	guiConfig();
};

void regenerateGui(const guiConfig &config);

void setScreenMainmenu();
void setScreenGame();
void setScreenGameover();
void setScreenOptions();
void setScreenScores();
void setScreenCredits();
void setScreenAchievements();
