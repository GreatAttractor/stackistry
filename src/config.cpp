/*
Stackistry - astronomical image stacking
Copyright (C) 2016, 2017 Filip Szczerek <ga.software@yahoo.com>

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
    Application configuration implementation.
*/

#include <iostream>
#include <sstream>

#include <glibmm/keyfile.h>
#include <glibmm/miscutils.h>
#include <gtkmm/enums.h>

#include "config.h"

namespace Configuration
{

namespace Group
{
    const char *UI = "UI";
    const char *Output = "Output";
}

namespace Key
{
    const char *toolIconSize = "ToolIconSize";

    const char *MainWndPosSize = "MainWndPosSize";
    const char *FrameSelectDlgPosSize = "FrameSelectDlgPosSize";
    const char *AnchorSelectDlgPosSize = "AnchorSelectDlgPosSize";
    const char *SelectRefPointsDlgPosSize = "SelectRefPointsDlgPosSize";
    const char *PreferencesDlgPosSize = "PreferencesDlgPosSize";
    const char *SettingsDlgPosSize = "SettingsDlgPosSize";
    const char *MainWndMaximized = "MainWndMaximized";
    const char *QualityWndPosSize = "QualityWndPosSize";

    const char *JobColWidth = "JobColWidth";
    const char *StateColWidth = "StateColWidth";
    const char *ProgressColWidth = "ProgressColWidth";
    const char *LastOpenDir = "LastOpenDir";
    const char *mainWndPanedPos = "MainWndPanedPos";
    const char *numQualityHistBins = "NumQualityHistogramBins";

    const char *exportInactiveFramesQuality = "ExportInactiveFramesQuality";

    const char *UIlanguage = "UILanguage";
}

const char *CONFIG_FILE_NAME = ".stackistry";

const char *CUSTOM_ICON_SIZE_NAME = "stackistry_custom";

static Glib::KeyFile configFile;

static Gtk::IconSize customIconSize = Gtk::BuiltinIconSize::ICON_SIZE_LARGE_TOOLBAR;

static Gdk::Rectangle RectFromString(const Glib::ustring &s)
{
    Gdk::Rectangle rect;
    std::stringstream parser;

    char sep; // separator
    parser << s;
    parser >> (rect.gobj()->x) >> sep >>
              (rect.gobj()->y) >> sep >>
              (rect.gobj()->width) >> sep >>
              (rect.gobj()->height) >> sep;

    if (parser.fail())
        rect = UNDEFINED_RECT;

    return rect;
}

static Glib::ustring RectToString(const Gdk::Rectangle &r)
{
    return Glib::ustring::format(r.get_x(), ';', r.get_y(), ';',
                                 r.get_width(), ';', r.get_height(), ';');
}

static int GetIntVal(const char *group, const char *key, int defaultVal)
{
    int result;
    try
    {
        result = configFile.get_integer(group, key);
    }
    catch (Glib::KeyFileError &exc)
    {
        result = defaultVal;
    }
    return result;
}

static unsigned GetUnsignedVal(const char *group, const char *key, unsigned defaultVal)
{
    unsigned result;
    try
    {
        result = (unsigned)configFile.get_uint64(group, key);
        if (result == 0)
            result = defaultVal;
    }
    catch (Glib::KeyFileError &exc)
    {
        result = defaultVal;
    }
    return result;
}


Gdk::Rectangle GetRect(const char *group, const char *key)
{
    try
    {
        return RectFromString(configFile.get_string(group, key));
    }
    catch (Glib::KeyFileError &exc)
    {
        return UNDEFINED_RECT;
    }
}

int GetInt(const char *group, const char *key)
{
    try
    {
        return configFile.get_integer(group, key);
    }
    catch (Glib::KeyFileError &exc)
    {
        return UNDEFINED;
    }
}

std::string GetString(const char *group, const char *key)
{
    try
    {
        return configFile.get_string(group, key);
    }
    catch (Glib::KeyFileError &exc)
    {
        return "";
    }
}

Gtk::IconSize GetToolIconSize()
{
    return customIconSize;
}

void SetToolIconSize(int value)
{
    if (value != GetIntVal(Group::UI, Key::toolIconSize, DEFAULT_SIZE))
    {
        Glib::ustring name = Glib::ustring::format(CUSTOM_ICON_SIZE_NAME, value);

        // Check if we have not registered this size already
        customIconSize = Gtk::IconSize::from_name(name);
        if ((int)customIconSize == 0)
            customIconSize = Gtk::IconSize::register_new(name, value, value);

        configFile.set_integer(Group::UI, Key::toolIconSize, value);
    }

    if (value == DEFAULT_SIZE)
    {
        customIconSize = Gtk::BuiltinIconSize::ICON_SIZE_LARGE_TOOLBAR;
    }
}

#define WND_POS_SIZE_PROPERTY(WindowName)                          \
c_Property<Gdk::Rectangle> WindowName##PosSize(                    \
    []() { return GetRect(Group::UI, Key::WindowName##PosSize); }, \
    [](const Gdk::Rectangle &r) { configFile.set_string(Group::UI, Key::WindowName##PosSize, RectToString(r)); })

WND_POS_SIZE_PROPERTY(MainWnd);
WND_POS_SIZE_PROPERTY(FrameSelectDlg);
WND_POS_SIZE_PROPERTY(AnchorSelectDlg);
WND_POS_SIZE_PROPERTY(SelectRefPointsDlg);
WND_POS_SIZE_PROPERTY(PreferencesDlg);
WND_POS_SIZE_PROPERTY(SettingsDlg);
WND_POS_SIZE_PROPERTY(QualityWnd);

#define COL_WIDTH_PROPERTY(Column)                                                 \
    c_Property<int> Column(                                                   \
        []() { return GetInt(Group::UI, Key::Column); }, \
        [](const int &width) { configFile.set_integer(Group::UI, Key::Column, width); })

COL_WIDTH_PROPERTY(JobColWidth);
COL_WIDTH_PROPERTY(StateColWidth);
COL_WIDTH_PROPERTY(ProgressColWidth);

c_Property<int> MainWndPanedPos(
    []() { return GetInt(Group::UI, Key::mainWndPanedPos); },
    [](const int &pos) { configFile.set_integer(Group::UI, Key::mainWndPanedPos, pos); });

c_Property<bool> MainWndMaximized(
    []() { return 1 == GetInt(Group::UI, Key::MainWndMaximized); },
    [](const bool &maximized) { configFile.set_integer(Group::UI, Key::MainWndMaximized, (int)maximized); });

c_Property<std::string> LastOpenDir(
    []() { return GetString(Group::UI, Key::LastOpenDir); },
    [](const std::string &dir) { configFile.set_string(Group::UI, Key::LastOpenDir, dir); });

c_Property<std::string> UILanguage(
    []() { return GetString(Group::UI, Key::UIlanguage); },
    [](const std::string &lang) { configFile.set_string(Group::UI, Key::UIlanguage, lang); });

c_Property<bool> ExportInactiveFramesQuality(
    []() { return 1 == GetInt(Group::Output, Key::exportInactiveFramesQuality); },
    [](const bool &b) { configFile.set_integer(Group::Output, Key::exportInactiveFramesQuality, (int)b); });

c_Property<size_t> NumQualityHistogramBins(
    []() { return (size_t)GetUnsignedVal(Group::UI, Key::numQualityHistBins, Utils::Const::Defaults::NumQualityHistogramBins); },
    [](const size_t &n) { configFile.set_integer(Group::UI, Key::numQualityHistBins, n); });


bool Initialize()
{
    try
    {
        bool loadResult = configFile.load_from_file(Glib::build_filename(Glib::get_home_dir(), CONFIG_FILE_NAME));
        if (loadResult)
        {
            int tiSize = configFile.get_integer(Group::UI, Key::toolIconSize);
            if (tiSize != DEFAULT_SIZE)
                customIconSize = Gtk::IconSize::register_new(Glib::ustring::format(CUSTOM_ICON_SIZE_NAME, tiSize),
                                                             tiSize, tiSize);
        }
        return loadResult;
    }
    catch (...)
    {
        return false;
    }
}

void Store()
{
    configFile.save_to_file(Glib::build_filename(Glib::get_home_dir(), CONFIG_FILE_NAME));
}

}
