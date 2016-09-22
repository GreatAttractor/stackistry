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

#include <glibmm/i18n.h>
#include <gtkmm/box.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/label.h>
#include <gtkmm/layout.h>

#include "img_viewer.h"
#include "utils.h"


c_ImageViewer::c_ImageViewer()
{
    m_ApplyZoom = true;

    m_DrawArea.signal_draw().connect(sigc::mem_fun(*this, &c_ImageViewer::OnDraw), false);
    m_DrawArea.show();

    Gtk::Box *contents = Gtk::manage(new Gtk::VBox());
    contents->show();

    m_Zoom.set_adjustment(Gtk::Adjustment::create(100, 1, 100, 1, 10));
    m_Zoom.set_digits(0);
    m_Zoom.set_value_pos(Gtk::PositionType::POS_LEFT);
    m_Zoom.signal_value_changed().connect(sigc::slot<void>(
        [this]()
        {
            m_FitInWindow.set_active(false);
            OnChangeZoom();
        }
    ));
    m_Zoom.signal_format_value().connect(sigc::slot<Glib::ustring, double>(
        [this](double)
            {
                return Glib::ustring::compose("%1%%", GetZoomPercentVal());
            }
    ));
    m_Zoom.show();

    for (int i = 1; i <= 10; i++)
    {
        m_ZoomRange.append(Glib::ustring::compose("\u2264%100%%", i)); // u2664 = less-than or equal to
    }
    m_ZoomRange.set_active(0);
    m_ZoomRange.signal_changed().connect(sigc::slot<void>(
        [this]()
            {
                m_FitInWindow.set_active(false);
                m_Zoom.queue_draw(); // need to refresh the slider's displayed value
                OnChangeZoom();
            }
    ));
    m_ZoomRange.show();

    m_InterpolationMethod.append(_("fast"));
    m_InterpolationMethod.append(_("good"));
    m_InterpolationMethod.append(_("best"));
    m_InterpolationMethod.set_active((int)Utils::Const::Defaults::interpolation);

    // u201C, u201D: left and right double quotation marks
    Glib::ustring interpolationTooltip = _("Image interpolation method used when zoom level is not 100%;\n"
                                           "\u201Cfast\u201D has the lowest quality, \u201Cbest\u201D is the slowest");

    m_InterpolationMethod.set_tooltip_text(interpolationTooltip);
    m_InterpolationMethod.signal_changed().connect(sigc::mem_fun(*this, &c_ImageViewer::OnChangeZoom));
    m_InterpolationMethod.show();

    Gtk::Label *interpolationLabel = Gtk::manage(new Gtk::Label(_("quality:")));
    interpolationLabel->set_tooltip_text(interpolationTooltip);
    interpolationLabel->show();

    auto zoombox = Gtk::manage(new Gtk::HBox());
    auto zoomLabel = Gtk::manage(new Gtk::Label(_("Zoom:")));
    zoomLabel->show();

    m_FitInWindow.set_label(_("Fit"));
    m_FitInWindow.set_tooltip_text(_("Fit in window"));
    m_FitInWindow.signal_toggled().connect(sigc::mem_fun(*this, &c_ImageViewer::OnChangeZoom));
    m_FitInWindow.show();

    zoombox->pack_start(*zoomLabel,    Gtk::PackOptions::PACK_SHRINK,         Utils::Const::widgetPaddingInPixels);
    zoombox->pack_start(m_Zoom,        Gtk::PackOptions::PACK_EXPAND_WIDGET,  Utils::Const::widgetPaddingInPixels);
    Gtk::Widget *widgets[] = { &m_ZoomRange,
                               interpolationLabel,
                               &m_InterpolationMethod,
                               &m_FitInWindow };
    for (auto *w: widgets)
    {
        zoombox->pack_start(*w, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);
    }
    zoombox->show();
    contents->pack_start(*zoombox, Gtk::PackOptions::PACK_SHRINK);

    Gtk::EventBox *evtBox = Gtk::manage(new Gtk::EventBox());
    evtBox->add(m_DrawArea);
    evtBox->signal_button_press_event().connect(sigc::slot<bool, GdkEventButton*>(
        [this](GdkEventButton *event)
        {
            m_ImageAreaBtnPressSignal.emit(event);
            return false;
        }
    ));
    evtBox->show();

    m_ScrWin.get_size_request(m_PrevScrWinWidth, m_PrevScrWinHeight);
    m_ScrWin.add(*evtBox);
    m_ScrWin.signal_draw().connect(sigc::slot<bool, const Cairo::RefPtr<Cairo::Context>&>(
        [this](const Cairo::RefPtr<Cairo::Context>&)
        {
            /* Use this signal to react to m_ScrWin's resize. Do nothing if the size
               has not changed (e.g. when the mouse cursor has been moved over the widget
               and the scrollbars are fading in/out of view; this process generates a lot
               of 'draw' events). */
            if (m_Img && m_FitInWindow.get_active() &&
                   (m_ScrWin.get_allocated_width() != m_PrevScrWinWidth ||
                    m_ScrWin.get_allocated_height() != m_PrevScrWinHeight))
            {
                OnChangeZoom();

                m_PrevScrWinWidth = m_ScrWin.get_allocated_width();
                m_PrevScrWinHeight = m_ScrWin.get_allocated_height();
            }

            return false; // continue with normal processing of the signal
        }
    ));
    m_ScrWin.show();

    contents->pack_start(m_ScrWin, Gtk::PackOptions::PACK_EXPAND_WIDGET);

    pack_start(*contents);
}

void c_ImageViewer::OnChangeZoom()
{
    if (m_Img)
    {
        m_DrawArea.set_size_request(GetZoomPercentValIfEnabled() * m_Img->get_width() / 100,
                                    GetZoomPercentValIfEnabled() * m_Img->get_height() / 100);
        m_DrawArea.queue_draw();
    }

    m_ZoomChangedSignal.emit(GetZoomPercentVal());
}

int c_ImageViewer::GetZoomPercentValIfEnabled() const
{
    if (m_ApplyZoom)
        return GetZoomPercentVal();
    else
        return 100;
}

/** Returns current zoom factor as set with the controls,
    even if applying zoom has been disabled with SetApplyZoom(false). */
unsigned c_ImageViewer::GetZoomPercentVal() const
{
    if (m_FitInWindow.get_active() && m_Img)
    {
        // Return the zoom factor appropriate for a "touch from inside" fit

        int viewW = m_ScrWin.get_allocated_width(),
            viewH = m_ScrWin.get_allocated_height();

        if ((double)viewW/viewH > (double)m_Img->get_width()/m_Img->get_height())
        {
            return viewH * 100 / m_Img->get_height();
        }
        else
        {
            return viewW * 100 / m_Img->get_width();
        }
    }
    else
    {
        int scale = m_ZoomRange.get_active_row_number();
        return m_Zoom.get_value() + 100*scale;
    }
}

/** Creates and uses a copy of 'img' for display; it can be
    later accessed (and modified) via GetImage(). */
void c_ImageViewer::SetImage(const libskry::c_Image &img, bool refresh)
{
    m_Img = Utils::ConvertImgToSurface(img);
    m_DrawArea.set_size_request(GetZoomPercentValIfEnabled() * m_Img->get_width() / 100,
                                GetZoomPercentValIfEnabled() * m_Img->get_height() / 100);

    if (refresh)
        m_DrawArea.queue_draw();
}

/** Creates and uses a copy of 'img' for display; it can be
    later accessed (and modified) via GetImage(). */
void c_ImageViewer::SetImage(const Cairo::RefPtr<Cairo::ImageSurface> &img, bool refresh)
{
    m_Img = img;
    m_DrawArea.set_size_request(GetZoomPercentValIfEnabled() * m_Img->get_width() / 100,
                                GetZoomPercentValIfEnabled() * m_Img->get_height() / 100);

    if (refresh)
        m_DrawArea.queue_draw();
}

/// Changes to the returned surface will be visible
Cairo::RefPtr<Cairo::ImageSurface> c_ImageViewer::GetImage()
{
    return m_Img;
}

bool c_ImageViewer::OnDraw(const Cairo::RefPtr<Cairo::Context>& cr)
{
    if (!m_Img)
        return false;

    auto src = Cairo::SurfacePattern::create(m_Img);
    src->set_matrix(Cairo::scaling_matrix(
                        100.0/GetZoomPercentValIfEnabled(),
                        100.0/GetZoomPercentValIfEnabled()));
    src->set_filter(Utils::GetFilter((Utils::Const::InterpolationMethod)m_InterpolationMethod.get_active_row_number()));
    cr->set_source(src);

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

    // Call the external signal handler (if any)
    m_DrawImageAreaSignal.emit(cr);

    return true;
}

/// Refresh on screen the whole image
void c_ImageViewer::Refresh()
{
    m_DrawArea.queue_draw();
}

/// Refresh on screen the specified rectangle in the image
void c_ImageViewer::Refresh(const Cairo::Rectangle rect)
{
    unsigned zoom = GetZoomPercentValIfEnabled();
    m_DrawArea.queue_draw_area(rect.x * zoom / 100,
                               rect.y * zoom / 100,
                               rect.width *  zoom / 100,
                               rect.height * zoom / 100);
}

void c_ImageViewer::GetDisplayOffset(double &xofs, double &yofs)
{
    xofs = m_ScrWin.get_hadjustment()->get_value();
    yofs = m_ScrWin.get_vadjustment()->get_value();
}
