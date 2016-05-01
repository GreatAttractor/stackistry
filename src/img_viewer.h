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
    Image viewer widget header.
*/

#ifndef STACKISTRY_IMAGE_VIEWER_WIDGET_HEADER
#define STACKISTRY_IMAGE_VIEWER_WIDGET_HEADER

#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/layout.h>
#include <gtkmm/scrolledwindow.h>
#include <skry/skry_cpp.hpp>


/// Displays an image in a scrollable window
/** Unlike Gtk::Image, this widget provides access
    to the underlying image for modification. */
class c_ImageViewer: public Gtk::VBox
{
    Cairo::RefPtr<Cairo::ImageSurface> m_Img;
    Gtk::DrawingArea m_DrawArea;
    Gtk::ScrolledWindow m_ScrWin;

    // Signal handlers -------------
    bool OnBtnPress(GdkEventButton *event);
    bool OnDraw(const Cairo::RefPtr<Cairo::Context>& cr);
    //------------------------------

public:
    c_ImageViewer();

    /** Creates and uses a copy of 'img' for display; it can be
        later accessed (and modified) via GetImage(). */
    void SetImage(const libskry::c_Image &img, bool refresh = true);

    /** Creates and uses a copy of 'img' for display; it can be
        later accessed (and modified) via GetImage(). */
    void SetImage(const Cairo::RefPtr<Cairo::ImageSurface> &img, bool refresh = true);

    /// Changes to the returned surface will be visible after refresh
    Cairo::RefPtr<Cairo::ImageSurface> GetImage();

    /// Refresh on screen the whole image
    void Refresh();

    /// Refresh on screen the specified rectangle in the image
    void Refresh(const Cairo::Rectangle rect);

    void GetDisplayOffset(double &xofs, double &yofs);
};

#endif // STACKISTRY_IMAGE_VIEWER_WIDGET_HEADER
