#include "CinderConsole.h"

#include <Windows.h>
#include <string>

using namespace std;

void CinderConsole::create() {
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
}