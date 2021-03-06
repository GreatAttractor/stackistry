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
    Main program file.
*/

#include <chrono>
#include <iostream>
#if _WIN32
#include <windows.h> // For locale functions
#endif

#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <skry/skry.h>

#include "config.h"
#include "main_window.h"
#include "utils.h"


void SkryLogCallback(unsigned logEventType, const char *msg)
{
    switch (logEventType)
    {
    case SKRY_LOG_IMAGE:            std::cout << "(img) "; break;
    case SKRY_LOG_REF_PT_ALIGNMENT: std::cout << "(refpt) "; break;
    case SKRY_LOG_STACKING:         std::cout << "(stack) "; break;
    case SKRY_LOG_TRIANGULATION:    std::cout << "(tri) "; break;
    case SKRY_LOG_QUALITY:          std::cout << "(qual) "; break;
    case SKRY_LOG_AVI:              std::cout << "(avi) "; break;
    case SKRY_LOG_IMG_ALIGNMENT:    std::cout << "(stab) "; break;
    }
    std::cout << msg << std::endl;
}

double ClockSec()
{
    return 1.0/1000000 * std::chrono::duration_cast<std::chrono::microseconds>
               (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

int main(int argc, char *argv[])
{
    Utils::SetAppLaunchPath(argv[0]);
    Configuration::Initialize();

    if (std::string(Configuration::UILanguage) != Utils::Const::SYSTEM_DEFAULT_LANG)
    {
        // The "LANGUAGE" env. variable is later used by gettext() (the _() macro)
        // to determine which language to use
        Glib::setenv("LANGUAGE", (std::string(Configuration::UILanguage) + ".UTF-8").c_str(), 1);
    }
#if _WIN32
    else
    {
        // "LANG" and "LANGUAGE" are not defined under Windows.
        // In order to make gettext() use the system language,
        // get the locale info via WinAPI.
        LCID loc = GetSystemDefaultLangID();

        int numChars = GetLocaleInfo(loc, LOCALE_SISO639LANGNAME, NULL, 0);
        std::unique_ptr<TCHAR[]> langCode(new TCHAR[numChars]);
        GetLocaleInfo(loc, LOCALE_SISO639LANGNAME, langCode.get(), numChars);

        numChars = GetLocaleInfo(loc, LOCALE_SISO3166CTRYNAME, NULL, 0);
        std::unique_ptr<TCHAR[]> langCountry(new TCHAR[numChars]);
        GetLocaleInfo(loc, LOCALE_SISO3166CTRYNAME, langCountry.get(), numChars);

        Glib::setenv("LANGUAGE", (std::string(langCode.get()) + "_" + langCountry.get() + ".UTF-8").c_str(), 1);
    }
#endif

    const char *langFileName = "stackistry"; // name of Stackistry's *.mo file
    textdomain(langFileName);
    bindtextdomain(
        langFileName,
        Glib::build_filename(Glib::path_get_dirname(argv[0]), "..", "lang").c_str());
    bind_textdomain_codeset(langFileName, "UTF-8");

    SKRY_initialize();
//    SKRY_set_logging(
//        SKRY_LOG_REF_PT_ALIGNMENT |
//        SKRY_LOG_QUALITY | SKRY_LOG_STACKING | SKRY_LOG_IMG_ALIGNMENT,
//        SkryLogCallback);
    SKRY_set_clock_func(ClockSec);

    auto app =
      Gtk::Application::create(argc, argv, "Stackistry-application");

    // Needs to be called after creating 'app', otherwise _() (called internally) does not work (why?)
    Utils::EnumerateSupportedOutputFmts();

    c_MainWindow window;
    window.set_default_size(200, 200);

    // Application icon rendered in different sizes
    std::vector<Glib::RefPtr<Gdk::Pixbuf>> appIcons;
    for (unsigned size: { 16, 24, 32, 40, 64 })
    {
        appIcons.push_back(Utils::LoadIconFromFile("stackistry-app.svg", size, size));
    }
    window.set_default_icon_list(appIcons);

    auto appResult = app->run(window);

    SKRY_deinitialize();
    window.Finalize();
    Configuration::Store();
    return appResult;
}
