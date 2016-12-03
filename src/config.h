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
    Application configuration header.
*/

#ifndef STACKISTRY_CONFIGURATION_HEADER
#define STACKISTRY_CONFIGURATION_HEADER

#include <climits>
#include <string>

#include <gdkmm/rectangle.h>
#include <gtkmm/enums.h>

#include "utils.h"


using Utils::Types::c_Property;

/** Access to persistent configuration data stored in a file
    in user's home directory (e.g. on Linux at ~/.stackistry). */
namespace Configuration
{
    const int DEFAULT_SIZE = -1;
    const int UNDEFINED = -1;
    const Gdk::Rectangle UNDEFINED_RECT = Gdk::Rectangle(-1, -1, -1, -1);

    /** Returns 'false' if the configuration file could not be loaded
        (default settings will be used). */
    bool Initialize();

    /// Saves settings in the configuration file
    void Store();

    inline bool IsUndefined(const Gdk::Rectangle &r)
    {
        return r.get_x() == UNDEFINED_RECT.get_x() &&
               r.get_y() == UNDEFINED_RECT.get_y() &&
               r.get_width() == UNDEFINED_RECT.get_width() &&
               r.get_height() == UNDEFINED_RECT.get_height();
    }

    Gtk::IconSize GetToolIconSize();
    void SetToolIconSize(int value);

    extern c_Property<Gdk::Rectangle> MainWndPosSize;
    extern c_Property<Gdk::Rectangle> FrameSelectDlgPosSize;
    extern c_Property<Gdk::Rectangle> AnchorSelectDlgPosSize;
    extern c_Property<Gdk::Rectangle> SelectRefPointsDlgPosSize;
    extern c_Property<Gdk::Rectangle> PreferencesDlgPosSize;
    extern c_Property<Gdk::Rectangle> SettingsDlgPosSize;
    extern c_Property<Gdk::Rectangle> QualityWndPosSize;

    extern c_Property<int> JobColWidth;
    extern c_Property<int> StateColWidth;
    extern c_Property<int> ProgressColWidth;
    extern c_Property<int> MainWndPanedPos;
    extern c_Property<bool> MainWndMaximized;
    extern c_Property<std::string> LastOpenDir;
    extern c_Property<bool> ExportInactiveFramesQuality;
    extern c_Property<size_t> NumQualityHistogramBins;

    /// format: <language>_<country>, e.g. "pl_PL"; empty = system default language
    extern c_Property<std::string> UILanguage;
}

#endif // STACKISTRY_CONFIGURATION_HEADER
