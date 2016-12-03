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
    Quality graph window implementation.
*/

#include <algorithm>
#include <cstdlib>

#include <glibmm/i18n.h>
#include <gtkmm/togglebutton.h>

#include "config.h"
#include "quality_wnd.h"
#include "utils.h"


namespace Color
{
    const GdkRGBA chronoGraph{ 0.3, 0.75, 0.35, 1.0 };
    const GdkRGBA sortedGraph{ 0.2, 0.8, 0.9, 1.0 };
    const GdkRGBA grid{ 0.67, 0.67, 0.67, 1.0 };
    const GdkRGBA histogram{ 0.8, 0.94, 0.93, 1.0 };
}

const double chronoLineWidth = 2.0;
const double sortedLineWidth = 3.0;

c_QualityWindow::c_QualityWindow()
{
    set_title(_("Frame Quality"));
    set_border_width(Utils::Const::widgetPaddingInPixels);

    m_DrawItems.chrono = true;
    m_DrawItems.sorted = true;
    m_DrawItems.histogram = true;

    resize(640, 480);
    Utils::RestorePosSize(Configuration::QualityWndPosSize, *this);

    InitControls();
}

void c_QualityWindow::InitControls()
{
    Gtk::Box *contents = Gtk::manage(new Gtk::VBox());
    contents->show();

    m_JobName.set_alignment(Gtk::Align::ALIGN_START);
    m_JobName.show();
    contents->pack_start(m_JobName, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);


    m_DrawArea.signal_draw().connect(sigc::mem_fun(*this, &c_QualityWindow::OnDraw));
    Utils::SetBackgroundColor(m_DrawArea, Gdk::RGBA("white"));
    m_DrawArea.show();
    contents->pack_start(m_DrawArea, Gtk::PackOptions::PACK_EXPAND_WIDGET, Utils::Const::widgetPaddingInPixels);

    m_Export.set_label(_("Export..."));
    m_Export.set_tooltip_text(_("Export frame quality data to a file"));
    m_Export.signal_pressed().connect([this]() { m_ExportSignal.emit(); });
    m_Export.set_halign(Gtk::Align::ALIGN_START);
    m_Export.set_sensitive(false);
    m_Export.show();

    auto btnChrono = Gtk::manage(new Gtk::ToggleButton(_("chrono")));
    btnChrono->set_active(true);
    btnChrono->set_tooltip_text(_("Show quality values in chronological order"));
    btnChrono->show();
    btnChrono->signal_toggled().connect(
        [=]()
        {
            m_DrawItems.chrono = btnChrono->get_active();
            m_DrawArea.queue_draw();
        } );

    auto btnSorted = Gtk::manage(new Gtk::ToggleButton(_("sorted")));
    btnSorted->set_active(true);
    btnSorted->set_tooltip_text(_("Show sorted quality values"));
    btnSorted->show();
    btnSorted->signal_toggled().connect(
        [=]()
        {
            m_DrawItems.sorted = btnSorted->get_active();
            m_DrawArea.queue_draw();
        } );

    auto btnHist = Gtk::manage(new Gtk::ToggleButton(_("hist")));
    btnHist->set_active(true);
    btnHist->set_tooltip_text(_("Show histogram"));
    btnHist->show();
    btnHist->signal_toggled().connect(
        [=]()
        {
            m_DrawItems.histogram = btnHist->get_active();
            m_DrawArea.queue_draw();
        } );

    auto buttonBox = Gtk::manage(new Gtk::HBox());
    buttonBox->pack_start(m_Export, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);
    buttonBox->show();

    buttonBox->pack_end(*btnHist, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);
    buttonBox->pack_end(*btnSorted, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);
    buttonBox->pack_end(*btnChrono, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    contents->pack_start(*buttonBox, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    add(*contents);
}

static
void DrawGraph(const Cairo::RefPtr<Cairo::Context> &cr,
               const std::vector<SKRY_quality_t> &values,
               int height, double lineWidth, const GdkRGBA &color,
               double hstep, double vscale, double minValue)
{
    cr->set_dash(std::vector<double>{ } , 0);
    Utils::SetColor(cr, color);
    cr->set_line_width(lineWidth);
    cr->move_to(0, height - vscale * (values[0] - minValue));
    for (size_t i = 1; i < values.size(); i++)
        cr->line_to(i * hstep, height - vscale * (values[i] - minValue));

    cr->stroke();
}

void DrawHistogram(const Cairo::RefPtr<Cairo::Context> &cr,
                   const Utils::Types::c_Histogram &histogram,
                   int canvasWidth, int canvasHeight,
                   const GdkRGBA &color)
{
    Utils::SetColor(cr, color);

    const size_t numBins = histogram.GetBins().size();
    for (size_t i = 0; i < numBins; i++)
    {
        int binHeight = canvasHeight * histogram.GetBins()[i] / histogram.GetMaxBinCount();

        cr->rectangle(i * canvasWidth/numBins, canvasHeight - binHeight,
                      canvasWidth/numBins + 1, binHeight);

        cr->fill();
    }
}

static
void DrawGrid(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height, const GdkRGBA &color)
{
    Utils::SetColor(cr, color);
    cr->set_line_width(1);
    cr->set_dash(std::vector<double>{ 5 } , 0);

    const int gridDiv = 4;
    for (int i = 0; i <= gridDiv; i++)
    {
        int dy = (i == 0 ? 0 : 1);
        cr->move_to(0,       i * height/gridDiv - dy);
        cr->line_to(width-1, i * height/gridDiv - dy);
        cr->stroke();
    }
}

bool c_QualityWindow::OnDraw(const Cairo::RefPtr<Cairo::Context>& cr)
{
    if (!m_Job || m_Job->quality.framesChrono.empty())
        return false; // fall back to default - fill with background color

    int width = m_DrawArea.get_width(),
        height = m_DrawArea.get_height();

    if (m_DrawItems.histogram)
        DrawHistogram(cr, m_Histogram, width, height, Color::histogram);

    DrawGrid(cr, width, height, Color::grid);

    if (m_MaxQ == m_MinQ && (m_DrawItems.sorted || m_DrawItems.chrono))
    {
        Utils::SetColor(cr, Color::chronoGraph);
        cr->set_line_width(chronoLineWidth);
        cr->move_to(0, height/2);
        cr->line_to(width-1, height/2);
        cr->stroke();
    }
    else
    {
        double vscale = height  / (m_MaxQ - m_MinQ);
        double hstep = (double)width / (m_Job->quality.framesChrono.size() - 1);

        if (m_DrawItems.sorted)
            DrawGraph(cr, m_Job->quality.framesSorted, height, sortedLineWidth, Color::sortedGraph, hstep, vscale, m_MinQ);

        if (m_DrawItems.chrono)
            DrawGraph(cr, m_Job->quality.framesChrono, height, chronoLineWidth, Color::chronoGraph, hstep, vscale, m_MinQ);
    }

    return true;
}

void c_QualityWindow::JobDeletionNotify()
{
    if (m_Job.use_count() == 1)
    {
        // If we remain the sole owner, the job has been just deleted
        // from the job list in the main window.
        m_Job.reset();

        if (is_visible())
            queue_draw();
    }
}

void c_QualityWindow::SetJob(std::shared_ptr<Job_t> job)
{
    m_Job = job;
    Update();
}

/** Should be called when contents of the previously set job may have changed.
    Redraws the graph. */
void c_QualityWindow::Update()
{
    if (m_Job && !m_Job->quality.framesChrono.empty())
    {
        auto minmaxq = std::minmax_element(m_Job->quality.framesChrono.begin(),
                                           m_Job->quality.framesChrono.end());
        m_MinQ = *minmaxq.first;
        m_MaxQ = *minmaxq.second;

        m_Histogram.CreateFromData(std::min((size_t)Configuration::NumQualityHistogramBins, m_Job->quality.framesChrono.size()),
                                   m_Job->quality.framesChrono, m_MinQ, m_MaxQ);

        m_JobName.set_text(m_Job->sourcePath);
        m_Export.set_sensitive(true);
    }
    else
    {
        m_JobName.set_text("");
        m_Export.set_sensitive(false);
    }

    if (is_visible())
        queue_draw();
}
