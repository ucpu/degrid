#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/entities.h>
#include <cage-core/hashString.h>

#include <cage-client/core.h>
#include <cage-client/engine.h>
#include <cage-client/window.h>
#include <cage-client/gui.h>

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
