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
    Utility functions header.
*/

#ifndef STACKISTRY_UTILS_HEADER
#define STACKISTRY_UTILS_HEADER

#include <string>
#include <vector>

#include <cairomm/context.h>
#include <gdkmm/pixbuf.h>
#include <gdkmm/rectangle.h>
#include <gtkmm/window.h>
#include <skry/skry_cpp.hpp>


namespace Utils
{

namespace Types
{

    /// Encapsulates a setter/getter of type T
    template <typename T>
    class c_Property
    {
        T (*m_Getter)();
        void (*m_Setter)(const T&);

    public:
        c_Property(T getter(), void setter(const T&))
          : m_Getter(getter), m_Setter(setter)
        { }

        void operator =(const T &value) { m_Setter(value); }
        operator T() const { return m_Getter(); }
    };

    class IValidatedInput
    {
    public:
        virtual bool HasCorrectInput() = 0;
    };
}

namespace Const
{
    const int refPtDrawRadius = 10;
    const guint widgetPaddingInPixels = 5;
    const unsigned imgAlignmentRefBlockSize = 32;

    enum MouseButtons { left = 1, MIDDLE = 2, RIGHT = 3 };

    enum OutputSaveMode { NONE, SOURCE_PATH, SPECIFIED_PATH };

    namespace Defaults
    {
        const OutputSaveMode saveMode = SOURCE_PATH;
        const enum SKRY_output_format outputFmt = SKRY_TIFF_16;
        const int referencePointSpacing = 40; ///< Value in pixels
        const float placementBrightnessThreshold = 0.33f;
        const enum SKRY_quality_criterion qualityCriterion = SKRY_quality_criterion::SKRY_PERCENTAGE_BEST;
        const unsigned qualityThreshold = 30;
    }
}

namespace Vars
{
    typedef struct
    {
        enum SKRY_output_format skryOutpFmt;
        Glib::ustring name;
        std::vector<Glib::ustring> patterns;
        std::string defaultExtension;
    } OutputFormatDescr_t;

    extern std::vector<OutputFormatDescr_t> outputFormatDescription;

    extern std::string appLaunchPath; ///< Value of argv[0]
}

Cairo::RefPtr<Cairo::ImageSurface> ConvertImgToSurface(const libskry::c_Image &img);

/// Returns the affected area of 'cr' (can be used for e.g. selective refresh on screen)
Cairo::Rectangle DrawAnchorPoint(const Cairo::RefPtr<Cairo::Context> &cr, int x, int y);

void EnumerateSupportedOutputFmts();

void SavePosSize(const Gtk::Window &wnd, Types::c_Property<Gdk::Rectangle> &destination);
void RestorePosSize(const Gdk::Rectangle &posSize, Gtk::Window &wnd);

enum SKRY_pixel_format FindMatchingFormat(enum SKRY_output_format outputFmt, size_t numChannels);

const Vars::OutputFormatDescr_t &GetOutputFormatDescr(enum SKRY_output_format);

/// Loads specified file from the 'icons' subdirectory
Glib::RefPtr<Gdk::Pixbuf> LoadIcon(const char *fileName, ///< Just the filename+extension
                                   int width, int height);

void SetAppLaunchPath(const char *appLaunchPath);

/// Returns a localized error message
std::string GetErrorMsg(enum SKRY_result errorCode);

}

#endif // STACKISTRY_UTILS_HEADER
