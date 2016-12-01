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
    Worker thread header.
*/


#ifndef STACKISTRY_WORKER_THREAD_HEADER
#define STACKISTRY_WORKER_THREAD_HEADER

#include <string>
#include <tuple>
#include <vector>

#include <cairomm/surface.h>
#include <glibmm/threads.h>

#include "job.h"
#include "utils.h"


namespace Worker
{
    enum class ProcPhase { IDLE = 0, IMAGE_ALIGNMENT, QUALITY_ESTIMATION, REF_POINT_ALIGNMENT, IMAGE_STACKING, NUM_PHASES };

    std::string GetProcPhaseStr(ProcPhase phase);

    void StartProcessing(Job_t *job);

    size_t GetStep();

    bool IsRunning();

    /// Should be called only after checking that IsRunning returns 'false'
    void WaitUntilFinished();

    void ConnectProgressSignal(const sigc::slot<void>& slot);

    Glib::Threads::RecMutex &GetAccessGuard();

    ProcPhase GetPhase();

    const Cairo::RefPtr<Cairo::ImageSurface> &GetVisualizationImage();

    /// Blocks until the worker thread finishes; calls WaitUntilFinished() internally
    void AbortProcessing();

    void SetVisualizationEnabled(bool enabled);
    bool IsVisualizationEnabled();

    /// Returns true if the main thread needs to provide reference points
    bool IsWaitingForReferencePoints();

    /// Notifies the worker thread that it may continue
    void NotifyReferencePointsSet();

    /// Can be called after quality estimation completes
    libskry::c_Image GetBestQualityAlignedImage();

    enum SKRY_result GetLastResult();

    /// Used by the main thread to indicate the current visualization zoom factor
    void SetZoomFactor(double zoom, Utils::Const::InterpolationMethod interpolationMethod);

    /// Returns the last values set with SetZoomFactor()
    std::tuple<double, Utils::Const::InterpolationMethod> GetZoomFactor();
}

#endif // STACKISTRY_WORKER_THREAD_HEADER
