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
    Job structure header.
*/
#ifndef STACKISTRY_JOB_STRUCT_HEADER
#define STACKISTRY_JOB_STRUCT_HEADER


#include <skry/skry_cpp.hpp>


struct Job_t
{
    libskry::c_ImageSequence imgSeq; // has to be the first field

    enum SKRY_output_format outputFmt;
    Utils::Const::OutputSaveMode outputSaveMode;
    libskry::c_Image stackedImg;

    enum SKRY_img_alignment_method alignmentMethod;

    struct
    {
        enum SKRY_quality_criterion criterion;
        unsigned threshold; ///< Interpreted according to 'criterion'

        /// Frame quality in chronological order
        std::vector<SKRY_quality_t> framesChrono;

        /// Frame quality, sorted descending
        std::vector<SKRY_quality_t> framesSorted;
    } quality;

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

    /// If 'true', frame quality data will be saved to a file in the same location as the stack
    bool exportQualityData;

    /// 'True' if quality data has been calculated by the worker thread
    bool qualityDataReadyNotification;
};

#endif // STACKISTRY_JOB_STRUCT_HEADER
