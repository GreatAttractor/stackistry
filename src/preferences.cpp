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
    Preferences dialog implementation.
*/

#include <sstream>

#include <gtkmm/box.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <gtkmm/separator.h>

#include "config.h"
#include "preferences.h"
#include "utils.h"


const int MIN_ICON_SIZE_PIX = 8;
const int MAX_ICON_SIZE_PIX = 128;

//TODO: move this to Utils?
template<typename T>
bool ConvertString(const char *str,
                   T *result = nullptr)
{
    T val;
    std::stringstream ss;
    ss << str;
    ss >> val;
    if (result) *result = val;
    return !ss.fail();
}

c_PreferencesDlg::c_PreferencesDlg()
{
    set_title("Preferences");
    InitControls();
    signal_response().connect(sigc::mem_fun(*this, &c_PreferencesDlg::OnResponse));
    Utils::RestorePosSize(Configuration::PreferencesDlgPosSize, *this);
}

void c_PreferencesDlg::InitControls()
{
    int iconSizePx;
    Gtk::IconSize::lookup(Configuration::GetToolIconSize(), iconSizePx, iconSizePx);
    m_ToolIconSize.set_adjustment(Gtk::Adjustment::create(iconSizePx, MIN_ICON_SIZE_PIX, MAX_ICON_SIZE_PIX));
    m_ToolIconSize.set_digits(0);
    m_ToolIconSize.set_value_pos(Gtk::PositionType::POS_LEFT);
    for (unsigned sizeMark: { 8, 16, 24, 32, 40, 64, 80, 100, 128 })
        m_ToolIconSize.add_mark(sizeMark, Gtk::PositionType::POS_TOP, Glib::ustring::format(sizeMark));

    Gtk::HBox *boxToolIconSize = Gtk::manage(new Gtk::HBox());
    boxToolIconSize->pack_start(*Gtk::manage(new Gtk::Label("Tool icon size:")), Gtk::PackOptions::PACK_SHRINK);
    boxToolIconSize->pack_start(m_ToolIconSize);
    boxToolIconSize->set_spacing(10);
    //?boxToolIconSize->set_baseline_position(Gtk::BaselinePosition::BASELINE_POSITION_BOTTOM);
    boxToolIconSize->show_all();

    get_content_area()->pack_start(*boxToolIconSize, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    auto separator = Gtk::manage(new Gtk::Separator());
    separator->show();
    get_content_area()->pack_end(*separator, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);


    add_button("OK", Gtk::RESPONSE_OK);
    add_button("Cancel", Gtk::RESPONSE_CANCEL);

    //FIXME: not working when a Gtk::Entry has focus! #### set_default(*get_widget_for_response(Gtk::ResponseType::RESPONSE_OK));
}

void c_PreferencesDlg::OnResponse(int responseId)
{
    Utils::SavePosSize(*this, Configuration::PreferencesDlgPosSize);
}

bool c_PreferencesDlg::HasCorrectInput()
{
//    int result;
//    bool convResult = ConvertString<int>(m_ToolIconSize.get_text().c_str(), &result);
//    return convResult && result > 0;
    return true;
}

int c_PreferencesDlg::GetToolIconSize()
{
//    int result;
//    ConvertString<int>(m_ToolIconSize.get_text().c_str(), &result);
//    return result;
    return (int)m_ToolIconSize.get_value();
}
