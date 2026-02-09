#pragma once

#include <finescript/script_engine.h>

namespace finegui {

/// Register all ui.* and gui.* native functions in the script engine.
///
/// Builder functions (pure map construction):
///   ui.window, ui.text, ui.button, ui.checkbox, ui.slider, ui.slider_int,
///   ui.input, ui.input_int, ui.input_float, ui.combo, ui.separator,
///   ui.group, ui.columns
///
/// Action functions (require ScriptGui context via ctx.userData()):
///   ui.show, ui.update, ui.hide, gui.on_message
void registerGuiBindings(finescript::ScriptEngine& engine);

} // namespace finegui
