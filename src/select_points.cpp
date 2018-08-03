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
    Point selection dialog implementation.
*/

#include <cairomm/surface.h>
#include <glibmm/i18n.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/separator.h>

#include "select_points.h"
#include "config.h"
#include "sig_functors.h"
#include "utils.h"


c_SelectPointsDlg::c_SelectPointsDlg(const libskry::c_Image &img,
                                     const std::vector<struct SKRY_point> &points,
                                     const std::vector<struct SKRY_point> &automaticPoints)
{
    m_Img = img;
    m_Points = points;
    m_AutomaticPoints = automaticPoints;
    InitControls(!automaticPoints.empty());

    m_ImgView.SetImage(m_Img);

    set_response_sensitive(Gtk::ResponseType::RESPONSE_OK, !m_Points.empty());
}

void c_SelectPointsDlg::OnRemoveClick()
{
    m_Points.clear();
    m_ImgView.SetImage(m_Img);
    set_response_sensitive(Gtk::ResponseType::RESPONSE_OK, false);
}

void c_SelectPointsDlg::OnAutoClick()
{
    m_Points.clear();
    m_ImgView.SetImage(m_Img);

    for (auto &autoPt: m_AutomaticPoints)
        m_Points.push_back(autoPt);

    set_response_sensitive(Gtk::ResponseType::RESPONSE_OK, true);
}

void c_SelectPointsDlg::InitControls(bool hasAutoBtn)
{
    m_ImgView.signal_ImageAreaBtnPress().connect(sigc::mem_fun(*this, &c_SelectPointsDlg::OnImageBtnPress));

    m_ImgView.signal_DrawImageArea().connect(sigc::slot<bool, const Cairo::RefPtr<Cairo::Context>&>(
        [this](const Cairo::RefPtr<Cairo::Context> &cr)
        {
            for (auto &pt: m_Points)
                Utils::DrawAnchorPoint(cr,
                                       m_ImgView.GetZoomPercentVal() * pt.x / 100,
                                       m_ImgView.GetZoomPercentVal() * pt.y / 100);

            return false;
        }
    ));
    m_ImgView.show();

    Gtk::Button *btnRemovePts = Gtk::manage(new Gtk::Button(_("Remove points")));
    btnRemovePts->signal_clicked().connect(sigc::mem_fun(*this, &c_SelectPointsDlg::OnRemoveClick));
    btnRemovePts->show();

    Gtk::Button *btnAuto = 0;
    if (hasAutoBtn)
    {
        btnAuto = Gtk::manage(new Gtk::Button(_("Automatic")));
        btnAuto->signal_clicked().connect(sigc::mem_fun(*this, &c_SelectPointsDlg::OnAutoClick));
        btnAuto->show();
    }

    Gtk::HButtonBox *btnBox = Gtk::manage(new Gtk::HButtonBox());
    btnBox->set_layout(Gtk::ButtonBoxStyle::BUTTONBOX_START);
    btnBox->pack_start(*btnRemovePts, Gtk::PackOptions::PACK_SHRINK);
    if (hasAutoBtn)
    {
        btnBox->pack_start(*btnAuto, Gtk::PackOptions::PACK_SHRINK);
    }
    btnBox->show();
    get_content_area()->pack_start(*btnBox, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    get_content_area()->pack_start(m_ImgView, Gtk::PackOptions::PACK_EXPAND_WIDGET, Utils::Const::widgetPaddingInPixels);

    m_InfoText.set_alignment(Gtk::ALIGN_START, Gtk::ALIGN_CENTER);
    m_InfoText.set_ellipsize(Pango::EllipsizeMode::ELLIPSIZE_END);
    m_InfoText.set_lines(2);
    get_content_area()->pack_start(m_InfoText, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    auto separator = Gtk::manage(new Gtk::Separator());
    separator->show();
    get_content_area()->pack_end(*separator, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    add_button(_("OK"), Gtk::RESPONSE_OK);
    add_button(_("Cancel"), Gtk::RESPONSE_CANCEL);
}

bool c_SelectPointsDlg::OnImageBtnPress(GdkEventButton *event)
{
    if (event->type == GDK_BUTTON_PRESS && event->button == Utils::Const::MouseButtons::left)
    {
        unsigned zoom = m_ImgView.GetZoomPercentVal();

        int imgX = (int)(100 * event->x / zoom),
            imgY = (int)(100 * event->y / zoom);

        if (imgX >= 0 && imgX < m_ImgView.GetImage()->get_width() &&
            imgY >= 0 && imgY < m_ImgView.GetImage()->get_height())
        {
            m_Points.push_back({ imgX, imgY });
            m_ImgView.Refresh();

            set_response_sensitive(Gtk::ResponseType::RESPONSE_OK, true);
        }
    }
    return true;
}

/// Existing contents of 'anchors' are removed
void c_SelectPointsDlg::GetPoints(std::vector<struct SKRY_point> &points) const
{
    points = m_Points;
}

void c_SelectPointsDlg::SetInfoText(std::string text)
{
    // Add a newline to prevent text overlapping the separator under
    // 'm_InfoText' when 'm_InfoText' is squeezed and the text wraps
    m_InfoText.set_text(text + "\n");
    if (text.empty())
        m_InfoText.hide();
    else
        m_InfoText.show();
}
