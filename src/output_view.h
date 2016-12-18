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
    Output viewer widget header.
*/

#ifndef STACKISTRY_OUTPUT_VIEWER_WIDGET_HEADER
#define STACKISTRY_OUTPUT_VIEWER_WIDGET_HEADER

#include "img_viewer.h"


enum class OutputImgType
{
    Visualization = 0, ///< Processing visualization
    Stack         = 1, ///< Image stack
    BestFragments = 2  ///< Composite of best fragments of all images
};


/** Provides an additional combo box at the top for selecting
    the type of currently viewed output image. */
class c_OutputViewer: public c_ImageViewer
{
public:

    typedef sigc::signal<void> OutputImgTypeChangedSignal_t;

private:

    Gtk::ComboBoxText m_OutputTypeCombo;
    OutputImgTypeChangedSignal_t m_OutputImgTypeChangedSignal;

    void UpdateTooltip();
    void OnOutputTypeChanged();

public:

    c_OutputViewer();

    OutputImgTypeChangedSignal_t signal_OutputImgTypeChanged()
    {
        return m_OutputImgTypeChangedSignal;
    }

    OutputImgType GetOutputImgType() const
    {
        return (OutputImgType)m_OutputTypeCombo.get_active_row_number();
    }

    void SetOutputImgType(OutputImgType type)
    {
        m_OutputTypeCombo.set_active((int)type);
    }
};


#endif // STACKISTRY_OUTPUT_VIEWER_WIDGET_HEADER
