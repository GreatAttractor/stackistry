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
    Frame selection dialog implementation.
*/

#include <iostream>

#include <gtkmm/button.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/separator.h>
#include <gtkmm/scrolledwindow.h>

#include "config.h"
#include "frame_select.h"
#include "utils.h"


const guint KEY_DEACTIVATE_FRAMES = GDK_KEY_Delete;
const guint KEY_TOGGLE_FRAMES = GDK_KEY_space;

Gtk::Box *c_FrameSelectDlg::CreateVisualizationBox()
{
    auto box = Gtk::manage(new Gtk::VBox());
    box->pack_start(m_ImgView, Gtk::PackOptions::PACK_EXPAND_WIDGET, Utils::Const::widgetPaddingInPixels);
    box->pack_start(m_VideoPos, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);
    box->show();
    return box;
}

void c_FrameSelectDlg::OnSyncToggled()
{
    Gtk::TreeModel::Path path;
    Gtk::TreeViewColumn *focusCol;
    m_FrameList.view.get_cursor(path, focusCol);

    if (m_SyncListWSlider.get_active() && !path.empty()) // 'path' will be always filled
        m_VideoPos.set_value((*m_FrameList.data->get_iter(path))[m_FrameList.columns.index]);
}

Gtk::Box *c_FrameSelectDlg::CreateFrameListBox()
{
    get_action_area()->set_border_width(10);

    m_FrameList.data = Gtk::ListStore::create(m_FrameList.columns);
    m_FrameList.view.set_model(m_FrameList.data);
    m_FrameList.view.set_activate_on_single_click(false);
    m_FrameList.view.append_column_editable("Active", m_FrameList.columns.active);
    m_FrameList.view.append_column("Index", m_FrameList.columns.index);

    m_FrameList.view.show();
    const uint8_t *activeFlags = m_ImgSeq.GetImgActiveFlags();
    for (size_t i = 0; i < m_ImgSeq.GetImageCount(); i++)
    {
        auto row = *m_FrameList.data->append();
        row[m_FrameList.columns.index] = i;
        row[m_FrameList.columns.active] = (1 == activeFlags[i]);
    }
    m_FrameList.view.columns_autosize();
    m_FrameList.view.get_selection()->set_mode(Gtk::SelectionMode::SELECTION_MULTIPLE);
    m_FrameList.view.set_cursor(Gtk::TreeModel::Path("0"));
    m_FrameList.view.signal_cursor_changed().connect(sigc::mem_fun(*this, &c_FrameSelectDlg::OnFrameListCursorChanged));
    m_FrameList.view.signal_key_press_event().connect(sigc::mem_fun(*this, &c_FrameSelectDlg::OnListKeyPress), false);

    auto listScrWin = Gtk::manage(new Gtk::ScrolledWindow());
    listScrWin->add(m_FrameList.view);
    listScrWin->show();

    auto btnActAll = Gtk::manage(new Gtk::Button("Activate all"));
    btnActAll->signal_clicked().connect(sigc::mem_fun(*this, &c_FrameSelectDlg::OnActivateAll));
    btnActAll->show();

    int iconSizeInPx;
    Gtk::IconSize::lookup(Configuration::GetToolIconSize(), iconSizeInPx, iconSizeInPx);
    auto iconImg = Gtk::manage(new Gtk::Image(Utils::LoadIcon("sync.svg", iconSizeInPx, iconSizeInPx)));
    iconImg->show();
    m_SyncListWSlider.set_image(*iconImg);
    m_SyncListWSlider.set_tooltip_text("Synchronize frame list with slider");
    m_SyncListWSlider.set_active(true);
    m_SyncListWSlider.signal_toggled().connect(sigc::mem_fun(*this, &c_FrameSelectDlg::OnSyncToggled));
    m_SyncListWSlider.show();

    auto btnBox = Gtk::manage(new Gtk::HBox());
    btnBox->pack_start(*btnActAll, Gtk::PackOptions::PACK_SHRINK);
    btnBox->pack_start(m_SyncListWSlider, Gtk::PackOptions::PACK_SHRINK);
    btnBox->show();

    auto box = Gtk::manage(new Gtk::VBox());
    box->pack_start(*btnBox, Gtk::PackOptions::PACK_SHRINK);
    box->pack_start(*listScrWin, Gtk::PackOptions::PACK_EXPAND_WIDGET);
    box->show();
    return box;
}

void c_FrameSelectDlg::OnActivateAll()
{
    for (auto &row: m_FrameList.data->children())
        row[m_FrameList.columns.active] = true;
}

void c_FrameSelectDlg::InitControls()
{
    libskry::c_Image firstImg = m_ImgSeq.GetImageByIdx(0);
    if (!firstImg)
        std::cout << "Failed to load first image " << std::endl;
    else
        m_ImgView.SetImage(firstImg);

    m_ImgView.signal_draw().connect(sigc::mem_fun(*this, &c_FrameSelectDlg::OnDrawImage));
    m_ImgView.show();

    m_VideoPos.set_adjustment(Gtk::Adjustment::create(0, 0, m_ImgSeq.GetImageCount()-1));
    m_VideoPos.set_digits(0);
    //m_VideoPos.set_draw_value(false);
    m_VideoPos.set_value_pos(Gtk::PositionType::POS_TOP);
    m_VideoPos.set_has_origin(false);
    m_VideoPos.signal_value_changed().connect(sigc::mem_fun(*this, &c_FrameSelectDlg::OnVideoPosScroll));
    m_VideoPos.show();

    auto hbox = Gtk::manage(new Gtk::HBox());
    hbox->pack_start(*CreateFrameListBox(), Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);
    hbox->pack_start(*CreateVisualizationBox(), Gtk::PackOptions::PACK_EXPAND_WIDGET, Utils::Const::widgetPaddingInPixels);
    hbox->show();

    get_content_area()->pack_start(*hbox);

    auto separator = Gtk::manage(new Gtk::Separator());
    separator->show();
    get_content_area()->pack_end(*separator, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    add_button("OK", Gtk::RESPONSE_OK);
    add_button("Cancel", Gtk::RESPONSE_CANCEL);
}

c_FrameSelectDlg::c_FrameSelectDlg(libskry::c_ImageSequence &imgSeq)
: Gtk::Dialog(), m_ImgSeq(imgSeq), m_ActiveFlags(nullptr)
{
    set_title("Select frames for processing");
    InitControls();
    signal_response().connect(sigc::mem_fun(*this, &c_FrameSelectDlg::OnResponse));
    Utils::RestorePosSize(Configuration::FrameSelectDlgPosSize, *this);
}

void c_FrameSelectDlg::OnResponse(int responseId)
{
    Utils::SavePosSize(*this, Configuration::FrameSelectDlgPosSize);
}

void c_FrameSelectDlg::OnVideoPosScroll()
{
    libskry::c_Image img = m_ImgSeq.GetImageByIdx((int)m_VideoPos.get_value());

    if (!img)
        std::cout << "Failed to load image " << m_VideoPos.get_value() << std::endl;
    else
        m_ImgView.SetImage(img);

    if (m_SyncListWSlider.get_active())
        m_FrameList.view.set_cursor(Gtk::TreeModel::Path(Glib::ustring::format((size_t)m_VideoPos.get_value())));
}

bool c_FrameSelectDlg::OnDrawImage(const Cairo::RefPtr<Cairo::Context>& cr)
{
    if (!m_FrameList.data->children()[(size_t)m_VideoPos.get_value()][m_FrameList.columns.active])
    {
        int w = m_ImgView.GetImage()->get_width(),
            h = m_ImgView.GetImage()->get_height();

        double xofs, yofs;
        m_ImgView.GetDisplayOffset(xofs, yofs);

        cr->set_line_width(3);
        cr->set_source_rgb(1, 0, 0);

        cr->move_to(0-xofs, 0-yofs);
        cr->line_to(w-1-xofs, h-1-yofs);
        cr->stroke();

        cr->move_to(w-1-xofs, 0-yofs);
        cr->line_to(0-xofs, h-1-yofs);
        cr->stroke();
    }
    return true;
}

void c_FrameSelectDlg::OnFrameListCursorChanged()
{
    if (m_SyncListWSlider.get_active())
    {
        Gtk::TreeModel::Path path;
        Gtk::TreeViewColumn *focus_column;
        m_FrameList.view.get_cursor(path, focus_column);

        m_VideoPos.set_value(((*m_FrameList.data->get_iter(path))[m_FrameList.columns.index]));
    }
}

c_FrameSelectDlg::~c_FrameSelectDlg()
{
    delete[] m_ActiveFlags;
}

const uint8_t *c_FrameSelectDlg::GetActiveFlags()
{
    if (!m_ActiveFlags)
        m_ActiveFlags = new uint8_t[m_ImgSeq.GetImageCount()];

    for (auto &row: m_FrameList.data->children())
        m_ActiveFlags[row[m_FrameList.columns.index]] = row[m_FrameList.columns.active];

    return m_ActiveFlags;
}

bool c_FrameSelectDlg::OnListKeyPress(GdkEventKey *event)
{
    if (event->keyval == KEY_DEACTIVATE_FRAMES ||
        event->keyval == KEY_TOGGLE_FRAMES)
    {
        bool toggle = (event->keyval == KEY_TOGGLE_FRAMES);

        for (auto &row: m_FrameList.view.get_selection()->get_selected_rows())
        {
            auto iter = m_FrameList.data->get_iter(row);
            if (toggle)
                (*iter)[m_FrameList.columns.active] = !(*iter)[m_FrameList.columns.active];
            else
                (*iter)[m_FrameList.columns.active] = false;
        }

        // Refresh, in case we just toggled the currenty displayed frame
        m_ImgView.Refresh();

        return true; // prevent normal handling of GDK_KEY_space (after our toggle,
                     // it would re-enable the frame if focus was on the checkboxes column)
    }

    return false;
}
