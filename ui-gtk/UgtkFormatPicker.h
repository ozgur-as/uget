/*
 *
 *   Copyright (C) 2026 by Ozgur As
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

// Modal-ish dialog that displays UgetFormat entries and lets the user pick
// one. The result is delivered to a callback so the dialog stays compatible
// with GTK4's async pattern (no gtk_dialog_run).

#ifndef UGTK_FORMAT_PICKER_H
#define UGTK_FORMAT_PICKER_H

#include <gtk/gtk.h>
#include <UgetResolver.h>

#ifdef __cplusplus
extern "C" {
#endif

// Called once with the picked format (still owned by the result), or NULL
// if the user cancelled. The result must remain valid until after the
// callback runs; the picker does NOT take ownership of either the result
// or the format.
typedef void (*UgtkFormatPickerCallback)(UgetFormat* picked, void* user_data);

// Show the picker. The result is read-only — caller retains ownership and
// must keep it alive until callback fires.
void  ugtk_format_picker_show (GtkWindow*               parent,
                               UgetResolverResult*      result,
                               UgtkFormatPickerCallback callback,
                               void*                    user_data);

#ifdef __cplusplus
}
#endif

#endif  // UGTK_FORMAT_PICKER_H
