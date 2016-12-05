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
    Frame selection dialog header.
*/

#ifndef STACKISTRY_FRAME_SELECT_DIALOG
#define STACKISTRY_FRAME_SELECT_DIALOG

#include <cstdint>
#include <vector>

#include <cairomm/context.h>
#include <gtkmm/dialog.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scale.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/treeview.h>
#include <skry/skry_cpp.hpp>

#include "img_viewer.h"


class c_FrameSelectDlg: public Gtk::Dialog
{
public:
    c_FrameSelectDlg(libskry::c_ImageSequence &imgSeq);

    /// Element count = number of images in 'imgSeq'
    std::vector<uint8_t> GetActiveFlags() const;

private:
    class c_FrameListModelColumns: public Gtk::TreeModelColumnRecord
    {
    public:
        Gtk::TreeModelColumn<size_t> index;
        Gtk::TreeModelColumn<bool> active;

        c_FrameListModelColumns()
        {
            add(index);
            add(active);
        }
    };

    libskry::c_ImageSequence &m_ImgSeq;

    c_ImageViewer m_ImgView;
    Gtk::Scale m_VideoPos;
    Gtk::ToggleButton m_SyncListWSlider;
    struct
    {
        c_FrameListModelColumns      columns;
        Glib::RefPtr<Gtk::ListStore> data;
        Gtk::TreeView                view;
    } m_FrameList;

    void InitControls();
    Gtk::Box *CreateVisualizationBox();
    Gtk::Box *CreateFrameListBox();

    // Signal handlers ----------------------------
    void OnVideoPosScroll();
    void OnActivateAll();
    void OnSyncToggled();
    bool OnDrawImage(const Cairo::RefPtr<Cairo::Context>& cr);
    void OnFrameListCursorChanged();
    void OnResponse(int responseId);
    bool OnListKeyPress(GdkEventKey *event);
};

#endif // STACKISTRY_FRAME_SELECT_DIALOG
