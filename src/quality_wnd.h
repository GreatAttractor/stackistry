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
    Quality graph window header.
*/

#ifndef STACKISTRY_QUALITY_WINDOW_HEADER
#define STACKISTRY_QUALITY_WINDOW_HEADER

#include <memory>

#include <gtkmm/button.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/label.h>
#include <gtkmm/window.h>

#include "job.h"
#include "utils.h"


class c_QualityWindow: public Gtk::Window
{
public:

    typedef sigc::signal<void> ExportSignal_t;

private:

    std::shared_ptr<Job_t> m_Job;
    SKRY_quality_t m_MinQ, m_MaxQ;
    Utils::Types::c_Histogram m_Histogram;

    Gtk::DrawingArea m_DrawArea;
    Gtk::Label m_JobName;
    Gtk::Button m_Export;

    struct
    {
        bool chrono, sorted, histogram;
    } m_DrawItems;

    // Internal signal handlers -------------

    bool OnDraw(const Cairo::RefPtr<Cairo::Context>& cr);
    // --------------------------------------

    void InitControls();

    ExportSignal_t m_ExportSignal;

public:

    c_QualityWindow();

    void JobDeletionNotify();
    void SetJob(std::shared_ptr<Job_t> job);

    /** Should be called when contents of the previously set job may have changed.
        Redraws the graph. */
    void Update();

    ExportSignal_t signal_Export()
    {
        return m_ExportSignal;
    }

};

#endif // STACKISTRY_QUALITY_WINDOW_HEADER
