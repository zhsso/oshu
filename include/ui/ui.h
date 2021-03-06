/**
 * \file ui/ui.h
 * \ingroup ui
 */

#pragma once

/**
 * \defgroup ui UI
 *
 * \brief
 * Collection of GUI elements.
 *
 * All the widgets share a common interface:
 *
 * - The widget's state is defined with a structure.
 *
 * - The widget is initialized and configured with a `create` function,
 *   returning an int. The first argument is an #oshu_display, and the last one
 *   a *widget*. All the other parameters are widget-specific.
 *
 * - The widget is destroyed with the symmetrical void `destroy` function. Note
 *   that the object itself isn't freed, because it may be allocated on the
 *   stack, or somewhere else.
 *
 * - You can render the widget with its void `show` function. They usually only
 *   take the widget object, but may receive additional parameters.
 *
 */

#include "ui/audio.h"
#include "ui/background.h"
#include "ui/cursor.h"
#include "ui/metadata.h"
#include "ui/score.h"
