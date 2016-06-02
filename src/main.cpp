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
    Main program file.
*/

#include <chrono>
#include <iostream>
#include <glibmm/i18n.h>
#include <gtkmm.h>
extern "C" {
#include <skry/skry.h>
}

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
    std::cout << "argv[0] is " << argv[0] << std::endl;
    Utils::SetAppLaunchPath(argv[0]);

    SKRY_initialize();
//    SKRY_set_logging(
//        SKRY_LOG_REF_PT_ALIGNMENT |
//        SKRY_LOG_QUALITY | SKRY_LOG_STACKING | SKRY_LOG_IMG_ALIGNMENT,
//        SkryLogCallback);
    SKRY_set_clock_func(ClockSec);

    Configuration::Initialize();

    Utils::EnumerateSupportedOutputFmts();

    textdomain("stackistry");
    bindtextdomain("stackistry", "./lang");

    auto app =
      Gtk::Application::create(argc, argv, "Stackistry-application");

    c_MainWindow window;
    window.set_default_size(200, 200);

    // Application icon rendered in different sizes
    std::vector<Glib::RefPtr<Gdk::Pixbuf>> appIcons;
    for (unsigned size: { 16, 24, 32, 40, 64 })
    {
        appIcons.push_back(Utils::LoadIcon("stackistry-app.svg", size, size));
    }
    window.set_default_icon_list(appIcons);

    auto appResult = app->run(window);

    SKRY_deinitialize();
    window.Finalize();
    Configuration::Store();
    return appResult;
}
