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
    Point selection dialog header.
*/

#include <vector>

#include <cairomm/surface.h>
#include <gtkmm/dialog.h>
#include <gtkmm/label.h>
#include <skry/skry_cpp.hpp>

#include "img_viewer.h"


class c_SelectPointsDlg: public Gtk::Dialog
{
    libskry::c_Image m_Img;
    c_ImageViewer m_ImgView;
    std::vector<struct SKRY_point> m_Points;
    std::vector<struct SKRY_point> m_AutomaticPoints;
    Gtk::Label m_InfoText;

    // Signal handlers -------------
    bool OnImageBtnPress(GdkEventButton *event);
    void OnRemoveClick();
    void OnAutoClick();
    //------------------------------

    void InitControls(bool hasAutoBtn);
    void DrawPointAndRefresh(int x, int y);

public:
    c_SelectPointsDlg(const libskry::c_Image &img,
                      const std::vector<struct SKRY_point> &points,
                      /// If not empty, will be applied when "Automatic" btn is pressed
                      const std::vector<struct SKRY_point> &automaticPoints);

    /// Existing contents of 'points' will be removed
    void GetPoints(std::vector<struct SKRY_point> &points) const;

    void SetInfoText(std::string text);
};
