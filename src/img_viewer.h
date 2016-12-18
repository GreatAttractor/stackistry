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
    Image viewer widget header.
*/

#ifndef STACKISTRY_IMAGE_VIEWER_WIDGET_HEADER
#define STACKISTRY_IMAGE_VIEWER_WIDGET_HEADER

#include <gtkmm/box.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/image.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/layout.h>
#include <gtkmm/scale.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/togglebutton.h>
#include <sigc++/sigc++.h>
#include <skry/skry_cpp.hpp>

#include "utils.h"


/// Displays an image in a scrollable window
/** Unlike Gtk::Image, this widget provides access
    to the underlying image for modification. */
class c_ImageViewer: public Gtk::VBox
{
public:

    /** The Cairo context passed to signal handler encompasses the whole
        image viewing area; the user can ignore scroll offset.
        However, the current zoom level has to be taken into account
        (see 'GetZoomPercentVal'). */
    typedef sigc::signal<bool, const Cairo::RefPtr<Cairo::Context>&> DrawImageAreaSignal_t;

    /** The event coordinates passed to signal handler are expressed within
        the whole image viewing area; the user can ignore the scroll offset.
        However, the current zoom level has to be taken into account
        (see 'GetZoomPercentVal').
        NOTE: the coordinates may be outside the image; it is up to
              the caller to verify this. */
    typedef sigc::signal<bool, GdkEventButton*> ImageAreaBtnPressSignal_t;

    /// Argument is the zoom percent value
    typedef sigc::signal<void, int> ZoomChangedSignal_t;


protected:

    Gtk::HBox m_ZoomBox;
    Gtk::DrawingArea m_DrawArea;
    Cairo::RefPtr<Cairo::ImageSurface> m_Img;

private:


    Gtk::ScrolledWindow m_ScrWin;
    Gtk::Scale m_Zoom;
    Gtk::ComboBoxText m_ZoomRange;
    Gtk::ToggleButton m_FitInWindow;
    Gtk::ComboBoxText m_InterpolationMethod;

    int m_PrevScrWinWidth, m_PrevScrWinHeight;

    /// If false, zoom controls do not change image scale
    bool m_ApplyZoom;

    bool m_FreezeZoomNotifications;

    // Emitted signals ----------------------
    DrawImageAreaSignal_t     m_DrawImageAreaSignal;
    ImageAreaBtnPressSignal_t m_ImageAreaBtnPressSignal;
    ZoomChangedSignal_t       m_ZoomChangedSignal;
    sigc::signal<void>        m_ImageSetSignal;
    //------------------------------

    int GetZoomPercentValIfEnabled() const;

    // Internal signal handlers -------------
    bool OnDraw(const Cairo::RefPtr<Cairo::Context>& cr);
    void OnChangeZoom();
    //------------------------------

public:
    c_ImageViewer();

    /** Creates and uses a copy of 'img' for display; it can be
        later accessed (and modified) via GetImage(). */
    void SetImage(const libskry::c_Image &img, bool refresh = true);

    /** Creates and uses a copy of 'img' for display; it can be
        later accessed (and modified) via GetImage(). */
    void SetImage(const Cairo::RefPtr<Cairo::ImageSurface> &img, bool refresh = true);

    void RemoveImage() { SetImage(Cairo::RefPtr<Cairo::ImageSurface>(nullptr)); }

    /// Changes to the returned surface will be visible after refresh
    Cairo::RefPtr<Cairo::ImageSurface> GetImage();

    /// Refresh on screen the whole image
    void Refresh();

    /// Refresh on screen the specified rectangle in the image
    void Refresh(const Cairo::Rectangle rect);

    void GetDisplayOffset(double &xofs, double &yofs);

    /** Returns current zoom factor as set with the controls,
        even if applying zoom has been disabled with SetApplyZoom(false). */
    unsigned GetZoomPercentVal() const;

    /// Disable or enable image scaling
    /** The user may want to disable in-widget zooming if the user is providing
        an already scaled image. If 'applyZoom' is false, the "fit in window"
        feature is not available. */
    void SetApplyZoom(bool applyZoom)
    {
        m_ApplyZoom = applyZoom;
        if (!applyZoom)
            m_FitInWindow.set_active(false);

        m_FitInWindow.set_visible(applyZoom);
        Refresh();
    }

    bool GetApplyZoom() const { return m_ApplyZoom; }

    /// Signal emitted on drawing the image viewing area
    /** See 'DrawImageAreaSignal_t' for details. */
    DrawImageAreaSignal_t signal_DrawImageArea()
    {
        return m_DrawImageAreaSignal;
    }

    /// See 'ImageAreaBtnPressSignal_t' for details
    ImageAreaBtnPressSignal_t signal_ImageAreaBtnPress()
    {
        return m_ImageAreaBtnPressSignal;
    }

    /// Also emitted when the user changes interpolation method
    ZoomChangedSignal_t signal_ZoomChanged()
    {
        return m_ZoomChangedSignal;
    }

    sigc::signal<void> signal_ImageSet()
    {
        return m_ImageSetSignal;
    }

    Utils::Const::InterpolationMethod GetInterpolationMethod() const
    {
        return (Utils::Const::InterpolationMethod)m_InterpolationMethod.get_active_row_number();
    }

    void SetInterpolationMethod(Utils::Const::InterpolationMethod method)
    {
        m_InterpolationMethod.set_active((int)method);
    }

    void SetZoomControlsEnabled(bool enabled)
    {
        for (Gtk::Widget *w: { (Gtk::Widget *)&m_InterpolationMethod,
                               (Gtk::Widget *)&m_Zoom,
                               (Gtk::Widget *)&m_ZoomRange,
                               (Gtk::Widget *)&m_FitInWindow })
        {
            w->set_sensitive(enabled);
        }
        if (!enabled)
            m_FitInWindow.set_active(false);
    }

    void SetZoom(double zoom, Utils::Const::InterpolationMethod interpMethod);
};

#endif // STACKISTRY_IMAGE_VIEWER_WIDGET_HEADER
