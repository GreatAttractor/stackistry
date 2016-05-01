/*
Stackistry - astronomical image stacking
Copyright (C) 2016 Filip Szczerek <ga.software@yahoo.com>

This file is part of Stackistry.

Stackistry is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Stackistry is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stackistry.  If not, see <http://www.gnu.org/licenses/>.

File description:
    Preferences dialog header.
*/

#ifndef STACKISTRY_PREFERENCES_DIALOG_HEADER
#define STACKISTRY_PREFERENCES_DIALOG_HEADER

#include <gtkmm/dialog.h>
#include <gtkmm/scale.h>

#include "utils.h"


class c_PreferencesDlg: public Gtk::Dialog, public Utils::Types::IValidatedInput
{
    Gtk::Scale m_ToolIconSize;

    void InitControls();

    void OnResponse(int responseId);

public:
    c_PreferencesDlg();
    virtual bool HasCorrectInput();
    int GetToolIconSize();
};

#endif
