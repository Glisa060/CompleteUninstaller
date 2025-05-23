#pragma once
#include <wx/defs.h>

enum
{
    Minimal_Quit = wxID_EXIT,
    Minimal_About = wxID_ABOUT,
    Minimal_Open = wxID_OPEN,
    Run_Selected = wxID_ANY + 1,
    Minimal_Analyse = wxID_ANY + 2,
    RestartAsAdmin_ID = wxID_ANY + 3,
    Theme_Light = wxID_ANY + 4,
    Theme_Dark = wxID_ANY + 5,
};

enum class Theme { Light, Dark };