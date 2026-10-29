// hexic - ReXGlue Recompiled Project

#include "generated/default/hexic_init.h"

#include "hexic_app.h"

REXCVAR_DEFINE_STRING(input_script, "", "Hexic", "Replay timed controller presses from this file (testing)");

REX_DEFINE_APP(hexic, HexicApp::Create)
