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

#include "utils.h"

struct Job_t
{
    libskry::c_ImageSequence imgSeq; // has to be the first field

    enum SKRY_output_format outputFmt;
    Utils::Const::OutputSaveMode outputSaveMode;
    libskry::c_Image stackedImg;

    enum SKRY_img_alignment_method alignmentMethod;

    enum SKRY_quality_criterion qualityCriterion;
    unsigned qualityThreshold; ///< Interpreted according to 'qualityCriterion'

    std::string sourcePath; ///< For image series: directory only; for videos: full path to the video file
    std::string destDir; ///< Effective if outputSaveMode==OutputSaveMode::SPECIFIED_PATH

    bool automaticAnchorPlacement;
    std::vector<struct SKRY_point> anchors; ///< Used when automaticAnchorPlacement==false

    bool automaticRefPointsPlacement;
    std::vector<struct SKRY_point> refPoints; ///< Used when automaticRefPointsPlacement==false

    unsigned refPtBlockSize;
    unsigned refPtSearchRadius;

    struct
    {
        unsigned spacing;          ///< Spacing in pixels

        /// Min. image brightness that a ref. point can be placed at (values: [0; 1])
        /** Value is relative to the image's darkest (0.0) and brightest (1.0) pixels. */
        float brightnessThreshold;

        /// Structure detection threshold; value of 1.2 is recommended
        /** The greater the value, the more local contrast is required to place
            a ref. point. */
        float structureThreshold;

        /** Corresponds with pixel size of smallest structures. Should equal 1
            for optimally-sampled or undersampled images. Use higher values
            for oversampled (blurry) material. */
        unsigned structureScale;
    } refPtAutoPlacementParams;

    std::string flatFieldFileName; /// If empty, no flat-fielding will be performed

    /// If not SKRY_CFA_NONE, mono images will be treated as raw color with this filter pattern
    enum SKRY_CFA_pattern cfaPattern;
};

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

    Glib::Threads::Mutex &GetAccessGuard();

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
}

#endif // STACKISTRY_WORKER_THREAD_HEADER
