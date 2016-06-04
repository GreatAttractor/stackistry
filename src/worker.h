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
#include <vector>

#include <cairomm/surface.h>
#include <glibmm/threads.h>
#include <skry/skry_cpp.hpp>


namespace Worker
{
    enum class ProcPhase { IDLE = 0, IMAGE_ALIGNMENT, QUALITY_ESTIMATION, REF_POINT_ALIGNMENT, IMAGE_STACKING, NUM_PHASES };

    std::string GetProcPhaseStr(ProcPhase phase);

    void StartProcessing(libskry::c_ImageSequence &imgSeq,
                         /// Must not be empty
                         const std::vector<struct SKRY_point> &anchors,
                         unsigned refPtSpacing,
                         bool automaticRefPtPlacement,
                         /// Used when 'automaticRefPointsPlacement' is false; may be empty
                         const std::vector<struct SKRY_point> &refPoints,
                         enum SKRY_quality_criterion qualCriterion,
                         unsigned qualThreshold, ///< Interpreted according to 'qualCriterion'
                         std::string flatFieldFileName ///< If empty, flat-fielding is disabled
                         );

    size_t GetStep();

    bool IsRunning();

    /// Should be called only after checking that IsRunning returns 'false'
    void WaitUntilFinished();

    void ConnectProgressSignal(const sigc::slot<void>& slot);

    Glib::Threads::Mutex &GetAccessGuard();

    ProcPhase GetPhase();

    const Cairo::RefPtr<Cairo::ImageSurface> &GetVisualizationImage();

    /// Blocks until the worker thread finishes; calls WaitUntilFinished() internally
    void AbortProcessing();

    /** Performs a move, so should be called only once after each processing run
        (subsequent calls will return null, until next processing run finishes). */
    libskry::c_Image GetStackedImage();

    void SetVisualizationEnabled(bool enabled);
    bool IsVisualizationEnabled();

    /// Returns true if the main thread needs to provide reference points
    bool IsWaitingForReferencePoints();
    /// Returns the first active image after alignment
    libskry::c_Image GetAlignedFirstImage();
    /// Also notifies the worker thread that it may continue
    void SetReferencePoints(const std::vector<struct SKRY_point> refPoints);

    enum SKRY_result GetLastResult();
}

#endif // STACKISTRY_WORKER_THREAD_HEADER
