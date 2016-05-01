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
    Image viewer widget implementation.
*/

#include <vector>

#include <gtkmm/eventbox.h>
#include <gtkmm/layout.h>

#include "img_viewer.h"
#include "utils.h"


c_ImageViewer::c_ImageViewer()
{
    m_DrawArea.signal_draw().connect(sigc::mem_fun(*this, &c_ImageViewer::OnDraw), false);
    m_DrawArea.show();

    Gtk::EventBox *evtBox = Gtk::manage(new Gtk::EventBox());
    evtBox->add(m_DrawArea);
    evtBox->signal_button_press_event().connect(sigc::mem_fun(*this, &c_ImageViewer::OnBtnPress), false);
    evtBox->show();

    m_ScrWin.add(*evtBox);
    m_ScrWin.show();

    pack_start(m_ScrWin);
}

/** Creates and uses a copy of 'img' for display; it can be
    later accessed (and modified) via GetImage(). */
void c_ImageViewer::SetImage(const libskry::c_Image &img, bool refresh)
{
    m_Img = Utils::ConvertImgToSurface(img);
    m_DrawArea.set_size_request(img.GetWidth(), img.GetHeight());
    if (refresh)
        m_DrawArea.queue_draw();
}

/** Creates and uses a copy of 'img' for display; it can be
    later accessed (and modified) via GetImage(). */
void c_ImageViewer::SetImage(const Cairo::RefPtr<Cairo::ImageSurface> &img, bool refresh)
{
    m_Img = img;
    m_DrawArea.set_size_request(img->get_width(), img->get_height());
    if (refresh)
        m_DrawArea.queue_draw();
}

/// Changes to the returned surface will be visible
Cairo::RefPtr<Cairo::ImageSurface> c_ImageViewer::GetImage()
{
    return m_Img;
}

bool c_ImageViewer::OnBtnPress(GdkEventButton *event)
{
    // Propagate the event outside, so that owners of c_ImageViewer
    // can connect to its 'signal_button_press_event'
    return on_button_press_event(event);
}

bool c_ImageViewer::OnDraw(const Cairo::RefPtr<Cairo::Context>& cr)
{
    if (!m_Img)
        return false;

    cr->set_source(m_Img, 0, 0);

    std::vector<Cairo::Rectangle> clipRects;

    try
    {
        cr->copy_clip_rectangle_list(clipRects);
    }
    catch (...)
    {
        clipRects.clear();
        double x1, y1, x2, y2;
        cr->get_clip_extents(x1, y1, x2, y2);

        Cairo::Rectangle fullRect = { x1, y1, x2-x1+1, y2-y1+1 };
        clipRects.push_back(fullRect);
    }

    for (Cairo::Rectangle &rect: clipRects)
        cr->rectangle(rect.x, rect.y, rect.width, rect.height);

    cr->fill();

    // Propagate the event outside, so that owners of c_ImageViewer
    // can connect to its 'signal_button_press_event'
    return on_draw(cr);
}

/// Refresh on screen the whole image
void c_ImageViewer::Refresh()
{
    m_DrawArea.queue_draw();
}

/// Refresh on screen the specified rectangle in the image
void c_ImageViewer::Refresh(const Cairo::Rectangle rect)
{
    m_DrawArea.queue_draw_area(rect.x, rect.y, rect.width, rect.height);
}

void c_ImageViewer::GetDisplayOffset(double &xofs, double &yofs)
{
    xofs = m_ScrWin.get_hadjustment()->get_value();
    yofs = m_ScrWin.get_vadjustment()->get_value();
}
