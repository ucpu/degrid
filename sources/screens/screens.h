#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/entities.h>
#include <cage-core/HashString.h>

#include <cage-Engine/core.h>
#include <cage-Engine/Engine.h>
#include <cage-Engine/window.h>
#include <cage-Engine/gui.h>

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
