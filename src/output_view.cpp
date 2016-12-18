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
    Output viewer widget implementation.
*/

#include <glibmm/i18n.h>
#include <string>

#include "output_view.h"


c_OutputViewer::c_OutputViewer()
{
    m_OutputTypeCombo.append(_("Visualization"));
    m_OutputTypeCombo.append(_("Stack"));
    m_OutputTypeCombo.append(_("Best fragments"));

    m_OutputTypeCombo.signal_changed().connect(
            [this]()
            {
                SetApplyZoom(GetOutputImgType() != OutputImgType::Visualization);
                UpdateTooltip();
                m_OutputImgTypeChangedSignal.emit();
            });

    signal_ImageSet().connect(sigc::mem_fun(*this, &c_OutputViewer::UpdateTooltip));

    m_OutputTypeCombo.set_active(0);
    m_OutputTypeCombo.show();

    m_ZoomBox.pack_start(m_OutputTypeCombo, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);
    m_ZoomBox.reorder_child(m_OutputTypeCombo, 0);
}

void c_OutputViewer::UpdateTooltip()
{
    if (!m_Img)
    {
        std::string s;
        switch (GetOutputImgType())
        {
        case OutputImgType::Stack:
            s = _("Stack not available; select a job which has finished processing");
            break;

        case OutputImgType::BestFragments:
            s = _("Best fragments composite image not available; select a job which has finished quality estimation");
            break;

        case OutputImgType::Visualization:
            s = _("Visualization not available");
            break;
        }

        set_tooltip_text(s);
    }
    else
        set_tooltip_text("");
}
