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
    Utility functions implementation.
*/

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>

#include <cairomm/context.h>
#include <cairomm/surface.h>
#include <glibmm/i18n.h>
#include <glibmm/miscutils.h>

#include "config.h"
#include "utils.h"


namespace Utils
{

namespace Vars
{
    std::vector<OutputFormatDescr_t> outputFormatDescription;
    std::string appLaunchPath; ///< Value of argv[0]
}

Cairo::RefPtr<Cairo::ImageSurface> ConvertImgToSurface(const libskry::c_Image &img)
{
    libskry::c_Image imgBgra;

    const libskry::c_Image *pRgba;
    if (img.GetPixelFormat() != SKRY_PIX_BGRA8)
    {
        imgBgra = libskry::c_Image::ConvertPixelFormat(img, SKRY_PIX_BGRA8);
        if (!imgBgra)
            return Cairo::RefPtr<Cairo::ImageSurface>(nullptr);
        else
            pRgba = &imgBgra;
    }
    else
        pRgba = &img;

    Cairo::RefPtr<Cairo::ImageSurface> surface = Cairo::ImageSurface::create(Cairo::Format::FORMAT_RGB24, img.GetWidth(), img.GetHeight());
    for (unsigned row = 0; row < pRgba->GetHeight(); row++)
        std::memcpy(surface->get_data() + row * surface->get_stride(), pRgba->GetLine(row), surface->get_stride());

    return surface;
}

Cairo::Rectangle DrawAnchorPoint(const Cairo::RefPtr<Cairo::Context> &cr, int x, int y)
{
    cr->set_source_rgb(0.5, 0.2, 1);
    cr->move_to(x - Const::refPtDrawRadius, y);
    cr->line_to(x + Const::refPtDrawRadius, y);
    cr->move_to(x, y - Const::refPtDrawRadius);
    cr->line_to(x, y + Const::refPtDrawRadius);
    cr->stroke();


    return Cairo::Rectangle({
      (double)x - Utils::Const::refPtDrawRadius,
      (double)y - Utils::Const::refPtDrawRadius,
      2*Utils::Const::refPtDrawRadius + 2*cr->get_line_width(),
      2*Utils::Const::refPtDrawRadius + 2*cr->get_line_width() });
}

void EnumerateSupportedOutputFmts()
{
    size_t numFmts;
    const unsigned *supportedFmts = SKRY_get_supported_output_formats(&numFmts);

    for (size_t i = 0; i < numFmts; i++)
    {
        Vars::outputFormatDescription.push_back(Vars::OutputFormatDescr_t());
        Vars::OutputFormatDescr_t &descr = Vars::outputFormatDescription.back();
        descr.skryOutpFmt = (enum SKRY_output_format)supportedFmts[i];

        switch (supportedFmts[i])
        {
        case SKRY_BMP_8:
            descr.name = _("BMP 8-bit");
            descr.patterns = { "*.bmp" };
            descr.defaultExtension = ".bmp";
            break;

        case SKRY_TIFF_16:
            descr.name = _("TIFF 16-bit (uncompressed)");
            descr.patterns = { "*.tif", "*.tiff" };
            descr.defaultExtension = ".tif";
            break;

        case SKRY_PNG_8:
            descr.name = _("PNG 8-bit");
            descr.patterns = { "*.png" };
            descr.defaultExtension = ".png";
            break;
        }
    }
}

void RestorePosSize(const Gdk::Rectangle &posSize, Gtk::Window &wnd)
{
    //TODO: check if visible on any screen
    if (!Configuration::IsUndefined(posSize))
    {
        wnd.move(posSize.get_x(), posSize.get_y());
        wnd.resize(posSize.get_width(), posSize.get_height());
    }
}

void SavePosSize(const Gtk::Window &wnd, c_Property<Gdk::Rectangle> &destination)
{
    Gdk::Rectangle posSize;
    wnd.get_position(posSize.gobj()->x, posSize.gobj()->y);
    wnd.get_size(posSize.gobj()->width, posSize.gobj()->height);
    destination = posSize;
}

enum SKRY_pixel_format FindMatchingFormat(enum SKRY_output_format outputFmt, size_t numChannels)
{
    for (int i = SKRY_PIX_INVALID+1; i < SKRY_NUM_PIX_FORMATS; i++)
        if (i != SKRY_PIX_PAL8
            && NUM_CHANNELS[i] == numChannels
            && OUTPUT_FMT_BITS_PER_CHANNEL[outputFmt] == BITS_PER_CHANNEL[i])
        {
            return (SKRY_pixel_format)i;
        }

    return SKRY_PIX_INVALID;
}

const Vars::OutputFormatDescr_t &GetOutputFormatDescr(enum SKRY_output_format outpFmt)
{
    for (auto &descr: Vars::outputFormatDescription)
    {
        if (descr.skryOutpFmt == outpFmt)
            return descr;
    }

    assert(0);
}

Glib::RefPtr<Gdk::Pixbuf> LoadIcon(const char *fileName, int width, int height)
{
    try
    {
        std::string fullPath = Glib::build_filename(
            Glib::path_get_dirname(Vars::appLaunchPath), "..", "icons", fileName);

        return Gdk::Pixbuf::create_from_file(fullPath, width, height);
    }
    catch (Glib::Exception &ex)
    {
        std::cerr << ex.what() << std::endl;
    }
    return Glib::RefPtr<Gdk::Pixbuf>(nullptr);
}

void SetAppLaunchPath(const char *appLaunchPath)
{
    Vars::appLaunchPath = appLaunchPath;
}

}
