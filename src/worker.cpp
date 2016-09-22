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
    Worker thread implementation.
*/

#include <cassert>
#define _USE_MATH_DEFINES
#include <cmath>   // for M_PI
#include <cstddef>
#include <iostream>
#include <memory>

#include <glibmm/dispatcher.h>
#include <glibmm/i18n.h>
#include <glibmm/thread.h>
#include <glibmm/threads.h>
#include <glibmm/timer.h>

#include "utils.h"
#include "worker.h"


namespace Worker
{

namespace Vars
{
    static bool abortRequested = false;
    static size_t step;
    static libskry::c_ImageSequence *imgSeq = 0;
    /// A pointer used only when returning an aligned first image to the main thread
    static libskry::c_ImageAlignment *imgAlignment = 0;
    static std::vector<struct SKRY_point> anchors;
    static bool isWorkerRunning = false;
    static ProcPhase procPhase = ProcPhase::IDLE;
    static Glib::Threads::Thread *workerThread;
    static Glib::Dispatcher dispatcher;
    static Cairo::RefPtr<Cairo::ImageSurface> visualizationImg;
    static libskry::c_Image stackedImg;
    static bool enableVisualization = false;

    static bool automaticRefPtPlacement;
    static std::vector<struct SKRY_point> refPoints;
    static unsigned refPtSpacing;
    static double refPtPlacementThreshold;
    static bool isWaitingForReferencePoints;

    static std::string flatField;
    static enum SKRY_quality_criterion qualCriterion;
    static unsigned qualThreshold;
    static enum SKRY_result lastResult;


    static Glib::Threads::Mutex mtx; ///< Access guard for shared variables

    /// Used for notification by main thread that reference points have been provided
    static Glib::Threads::Mutex mtxRefPt;
    static Glib::Threads::Cond condRefPt;

    /// Current zoom factor specified in the main window's visualization widget
    static double zoomFactor = 1.0;
    /// Current zoom interpolation method specified in the main window's visualization widget
    static auto interpolationMethod = Utils::Const::Defaults::interpolation;
}

#define LOCK() Glib::Threads::Mutex::Lock lock(Vars::mtx)

void WorkerThreadFunc();

static libskry::c_Image GetAlignedImage(
    const libskry::c_ImageSequence &imgSeq,
    const libskry::c_ImageAlignment &imgAlignment);

// Function definitions ----------------------------

/// Used by the main thread to indicate the current visualization zoom factor
void SetZoomFactor(double zoom, Utils::Const::InterpolationMethod interpolationMethod)
{
    LOCK();
    Vars::zoomFactor = zoom;
    Vars::interpolationMethod = interpolationMethod;
}


Glib::Threads::Mutex &GetRefPtNotificationMtx()
{
    return Vars::mtxRefPt;
}

void AbortProcessing()
{
    { LOCK();
        Vars::abortRequested = true;
    }
    WaitUntilFinished();
}

ProcPhase GetPhase()
{
    return Vars::procPhase;
}

void NotifyMainThread()
{
    Vars::dispatcher();
}

void StartProcessingPhase(ProcPhase newPhase)
{
    Vars::procPhase = newPhase;
    Vars::step = 0;
}

void SetVisualizationEnabled(bool enabled)
{
    LOCK();
    Vars::enableVisualization = enabled;
}

void ConnectProgressSignal(const sigc::slot<void>& slot)
{
    Vars::dispatcher.connect(slot);
}

bool IsRunning()
{
    return Vars::isWorkerRunning;
}

size_t GetStep()
{
    return Vars::step;
}

void WaitUntilFinished()
{
    if (Vars::workerThread)
    {
        Vars::workerThread->join();
        Vars::workerThread = 0;
    }
}

ProcPhase GetProcessingPhase()
{
    return Vars::procPhase;
}

void StartProcessing(libskry::c_ImageSequence &imgSeq,
                     const std::vector<struct SKRY_point> &anchors,
                     unsigned refPtSpacing,
                     double refPtPlacementThreshold,
                     bool automaticRefPtPlacement,
                     const std::vector<struct SKRY_point> &refPoints,
                     enum SKRY_quality_criterion qualCriterion,
                     unsigned qualThreshold,
                     std::string flatFieldFileName)
{
    assert(!anchors.empty());
    Vars::imgSeq = &imgSeq;
    Vars::anchors = anchors;
    Vars::isWorkerRunning = true;
    Vars::abortRequested = false;
    Vars::stackedImg = libskry::c_Image();

    Vars::refPtSpacing = refPtSpacing;
    Vars::refPtPlacementThreshold = refPtPlacementThreshold;
    Vars::automaticRefPtPlacement = automaticRefPtPlacement;
    Vars::refPoints = refPoints;
    Vars::qualCriterion = qualCriterion;
    Vars::qualThreshold = qualThreshold;
    Vars::flatField = flatFieldFileName;

    Vars::workerThread = Glib::Threads::Thread::create(sigc::ptr_fun(&WorkerThreadFunc));
}

/// Returns a version of 'srcImg' scaled by Vars::zoomFactor
Cairo::RefPtr<Cairo::ImageSurface> GetScaledImg(const libskry::c_Image &srcImg)
{
    auto src = Cairo::SurfacePattern::create(Utils::ConvertImgToSurface(srcImg));
    src->set_matrix(Cairo::scaling_matrix(1 / Vars::zoomFactor, 1 / Vars::zoomFactor));
    src->set_filter(Utils::GetFilter(Vars::interpolationMethod));

    auto scaledImg = Cairo::ImageSurface::create(Cairo::Format::FORMAT_RGB24,
                                                 Vars::zoomFactor * srcImg.GetWidth(),
                                                 Vars::zoomFactor * srcImg.GetHeight());

    Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(scaledImg);
    cr->set_source(src);
    cr->rectangle(0, 0, Vars::zoomFactor * srcImg.GetWidth(),
                        Vars::zoomFactor * srcImg.GetHeight());
    cr->fill();

    return scaledImg;
}

static void CreateImgAlignmentVisualization(const libskry::c_ImageAlignment &imgAlignment)
{
    Vars::visualizationImg = GetScaledImg(Vars::imgSeq->GetCurrentImage());
    Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(Vars::visualizationImg);

    if (imgAlignment.GetAlignmentMethod() == SKRY_IMG_ALGN_ANCHORS)
    {
        auto anchors = imgAlignment.GetAnchors();

        for (size_t i = 0; i < anchors.size(); i++)
            if (imgAlignment.IsAnchorValid(i))
                Utils::DrawAnchorPoint(cr, Vars::zoomFactor * anchors[i].x,
                                           Vars::zoomFactor * anchors[i].y);
    }
    else if (imgAlignment.GetAlignmentMethod() == SKRY_IMG_ALGN_CENTROID)
    {
        auto centroid = imgAlignment.GetCentroid();
        Utils::DrawAnchorPoint(cr, Vars::zoomFactor * centroid.x,
                                   Vars::zoomFactor * centroid.y);
    }
}

static void CreateQualityEstimationVisualization(
    const libskry::c_ImageSequence &imgSeq,
    const libskry::c_ImageAlignment &imgAlignment,
    const libskry::c_QualityEstimation &qualEstimation)
{
    Vars::visualizationImg = GetScaledImg(GetAlignedImage(imgSeq, imgAlignment));

    //TODO: draw something?.. e.g. image in grayscale with quality color-mapped
}

static libskry::c_Image GetAlignedImage(
    const libskry::c_ImageSequence &imgSeq,
    const libskry::c_ImageAlignment &imgAlignment)
{
    libskry::c_Image currImg = imgSeq.GetCurrentImage();

    if (currImg)
    {
        currImg = libskry::c_Image::ConvertPixelFormat(currImg, SKRY_PIX_BGRA8, SKRY_DEMOSAIC_HQLINEAR);

        struct SKRY_rect intersection = imgAlignment.GetIntersection();

        struct SKRY_point currOffset = imgAlignment.GetImageOffset(imgSeq.GetCurrentImgIdxWithinActiveSubset());
        int xmin = intersection.x + currOffset.x;
        int ymin = intersection.y + currOffset.y;

        struct SKRY_palette srcPal;
        currImg.GetPalette(srcPal);
        libskry::c_Image alignedImg(intersection.width, intersection.height, currImg.GetPixelFormat(), &srcPal, false);
        libskry::c_Image::ResizeAndTranslate(currImg, alignedImg, xmin, ymin,
                                             intersection.width, intersection.height, 0, 0, false);
        return alignedImg;
    }
    else
        return libskry::c_Image();
}

static void CreateRefPtAlignmentVisualization(
    const libskry::c_ImageSequence &imgSeq,
    const libskry::c_ImageAlignment &imgAlignment,
    const libskry::c_RefPointAlignment &refPtAlignment)
{
    Vars::visualizationImg = GetScaledImg(GetAlignedImage(imgSeq, imgAlignment));
    Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(Vars::visualizationImg);

    const double RADIUS_VALID_POS = 4.0;
    const double RADIUS_INVALID_POS = 2.0;

    int imgIdx = imgSeq.GetCurrentImgIdxWithinActiveSubset();

    cr->set_line_width(1);

    for (int i = 0; i < refPtAlignment.GetNumReferencePoints(); i++)
    {
        bool isValid;
        struct SKRY_point pos = refPtAlignment.GetReferencePointPos(i, imgIdx, isValid);

        if (isValid)
            cr->set_source_rgb(0.8, 0.15, 1);
        else
            cr->set_source_rgb(0.9, 0.3, 0.3);

        cr->arc(Vars::zoomFactor * pos.x,
                Vars::zoomFactor * pos.y,
                isValid ? RADIUS_VALID_POS : RADIUS_INVALID_POS, 0, 2*M_PI);
        cr->stroke();
    }
}

static void CreateStackingVisualization(
    const libskry::c_Stacking &stacking,
    const libskry::c_RefPointAlignment &refPtAlignment)
{
    Vars::visualizationImg = GetScaledImg(stacking.GetPartialImageStack());
    Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(Vars::visualizationImg);


    const struct SKRY_triangle *triangles = SKRY_get_triangles(refPtAlignment.GetTriangulation());
    const struct SKRY_point_flt *verts = stacking.GetRefPtStackingPositions();
    size_t numStackedTris;
    const size_t *stackedTris = stacking.GetCurrentStepStackedTriangles(numStackedTris);

    cr->set_line_cap(Cairo::LineCap::LINE_CAP_ROUND);
    cr->set_source_rgb(0.6, 0.2, 0.7);
    for (size_t i = 0; i < numStackedTris; i++)
    {
        const struct SKRY_point_flt &v0 = verts[triangles[stackedTris[i]].v0],
                                    &v1 = verts[triangles[stackedTris[i]].v1],
                                    &v2 = verts[triangles[stackedTris[i]].v2];

        cr->move_to(Vars::zoomFactor * v0.x, Vars::zoomFactor * v0.y);
        cr->line_to(Vars::zoomFactor * v1.x, Vars::zoomFactor * v1.y);
        cr->line_to(Vars::zoomFactor * v2.x, Vars::zoomFactor * v2.y);
        cr->line_to(Vars::zoomFactor * v0.x, Vars::zoomFactor * v0.y);
        cr->stroke();
    }
}

bool IsVisualizationEnabled()
{
    return Vars::enableVisualization;
}

libskry::c_Image GetAlignedFirstImage()
{
    // We can rewind the image sequence now, as the worker is between phases
    // (after quality estimation, before ref. point alignment)
    Vars::imgSeq->SeekStart();
    return GetAlignedImage(*Vars::imgSeq, *Vars::imgAlignment);
}

/// Returns true if the main thread needs to provide reference points
bool IsWaitingForReferencePoints()
{
    return Vars::isWaitingForReferencePoints;
}

void SetReferencePoints(const std::vector<struct SKRY_point> refPoints)
{
    Vars::refPoints = refPoints;
    Vars::isWaitingForReferencePoints = false;

    Glib::Threads::Mutex::Lock lock(Vars::mtxRefPt);
    Vars::condRefPt.signal();
}

#define CHECK_ABORT()                                  \
    do {                                               \
        if (Vars::abortRequested)                      \
        {                                              \
            Vars::abortRequested = false;              \
            Vars::isWorkerRunning = false;             \
            Vars::isWaitingForReferencePoints = false; \
            return;                                    \
        }                                              \
    } while (0)

void WorkerThreadFunc()
{
    libskry::c_ImageAlignment imgAlignment(
            *Vars::imgSeq,
            SKRY_IMG_ALGN_ANCHORS,
            Vars::anchors,
            Utils::Const::imgAlignmentRefBlockSize/2,
            Utils::Const::imgAlignmentRefBlockSize/2,
            Utils::Const::Defaults::placementBrightnessThreshold);

    Vars::imgAlignment = &imgAlignment;

    if (!imgAlignment)
    {
        std::cerr << "Could not initialize image alignment." << std::endl;
        { LOCK();
            Vars::isWorkerRunning = false;
            NotifyMainThread();
            return;
        }
    }

    { LOCK();
        StartProcessingPhase(ProcPhase::IMAGE_ALIGNMENT);
    }
    while (SKRY_SUCCESS == (Vars::lastResult = imgAlignment.Step()))
    {
        { LOCK();
            CHECK_ABORT();
            Vars::step++;
            if (Vars::enableVisualization)
                CreateImgAlignmentVisualization(imgAlignment);
        }
        NotifyMainThread();
    }
    if (Vars::lastResult != SKRY_LAST_STEP)
    { LOCK();
        Vars::isWorkerRunning = false;
        NotifyMainThread();
        return;
    }

    libskry::c_QualityEstimation qualEstimation(imgAlignment, /*TODO: make it a param*/40, 3);
    if (!qualEstimation)
    {
        std::cerr << "Could not initialize quality estimation." << std::endl;
        { LOCK();
            Vars::isWorkerRunning = false;
            Vars::lastResult = SKRY_OUT_OF_MEMORY;
            NotifyMainThread();
            return;
        }
    }

    { LOCK();
        StartProcessingPhase(ProcPhase::QUALITY_ESTIMATION);
    }
    while (SKRY_SUCCESS == (Vars::lastResult = qualEstimation.Step()))
    {
        { LOCK();
            CHECK_ABORT();
            Vars::step++;
            if (Vars::enableVisualization)
                CreateQualityEstimationVisualization(*Vars::imgSeq, imgAlignment, qualEstimation);
        }
        NotifyMainThread();
    }
    if (Vars::lastResult != SKRY_LAST_STEP)
    { LOCK();
        Vars::isWorkerRunning = false;
        NotifyMainThread();
        return;
    }

    if (!Vars::automaticRefPtPlacement && Vars::refPoints.empty())
    {
        { LOCK();
            Vars::isWaitingForReferencePoints = true;
            NotifyMainThread();
        }

        { Glib::Threads::Mutex::Lock lock(Vars::mtxRefPt);

            Vars::condRefPt.wait(Vars::mtxRefPt);
            if (Vars::refPoints.empty()) // the user canceled the "Select ref. points" dialog
            {
                Vars::automaticRefPtPlacement = true;
            }
        }
    }

    libskry::c_RefPointAlignment refPtAlignment(qualEstimation,
                                                Vars::refPoints,
                                                Vars::refPtPlacementThreshold,
                                                Vars::qualCriterion,
                                                Vars::qualThreshold,
                                                Vars::refPtSpacing);
    if (!refPtAlignment)
    {
        std::cerr << "Could not initialize reference point alignment." << std::endl;
        { LOCK();
            Vars::isWorkerRunning = false;
            Vars::lastResult = SKRY_OUT_OF_MEMORY;
            NotifyMainThread();
            return;
        }
    }
    { LOCK();
        StartProcessingPhase(ProcPhase::REF_POINT_ALIGNMENT);
    }
    while (SKRY_SUCCESS == (Vars::lastResult = refPtAlignment.Step()))
    {
        { LOCK();
            CHECK_ABORT();
            Vars::step++;
            if (Vars::enableVisualization)
                CreateRefPtAlignmentVisualization(*Vars::imgSeq, imgAlignment, refPtAlignment);
        }
        NotifyMainThread();
    }
    if (Vars::lastResult != SKRY_LAST_STEP)
    { LOCK();
        Vars::isWorkerRunning = false;
        NotifyMainThread();
        return;
    }

    libskry::c_Image flatField;
    if (!Vars::flatField.empty())
    {
        flatField = libskry::c_Image::Load(Vars::flatField.c_str(), &Vars::lastResult);
        if (!flatField)
        {
            std::cerr << "Could not load flat-field from " << Vars::flatField << std::endl;
            NotifyMainThread();
            return;
        }
    }

    libskry::c_Stacking stacking(refPtAlignment,
                                 Vars::flatField.empty() ? nullptr : &flatField,
                                 &Vars::lastResult);
    if (!stacking)
    {
        std::cerr << "Could not initialize stacking." << std::endl;
        { LOCK();
            Vars::isWorkerRunning = false;
            NotifyMainThread();
            return;
        }
    }
    { LOCK();
        StartProcessingPhase(ProcPhase::IMAGE_STACKING);
    }
    while (SKRY_SUCCESS == (Vars::lastResult = stacking.Step()))
    {
        { LOCK();
            CHECK_ABORT();
            Vars::step++;
            if (Vars::enableVisualization)
                CreateStackingVisualization(stacking, refPtAlignment);
        }
        NotifyMainThread();
    }

    Vars::stackedImg = stacking.GetFinalImageStack();
    if (!Vars::stackedImg)
    {
        std::cerr << "Failed to obtain the final image stack." << std::endl;
    }

    { LOCK();
        Vars::abortRequested = false;
        Vars::isWorkerRunning = false;
        Vars::isWaitingForReferencePoints = false;
    }
    NotifyMainThread();
}

Glib::Threads::Mutex &GetAccessGuard()
{
    return Vars::mtx;
}

const Cairo::RefPtr<Cairo::ImageSurface> &GetVisualizationImage()
{
    return Vars::visualizationImg;
}

std::unique_ptr<int> sth;

libskry::c_Image GetStackedImage()
{
        return std::move(Vars::stackedImg);
}

enum SKRY_result GetLastResult()
{
    return Vars::lastResult;
}

std::string GetProcPhaseStr(ProcPhase phase)
{
    switch (phase)
    {
        case ProcPhase::IDLE:                return _("Idle");
        case ProcPhase::IMAGE_ALIGNMENT:     return _("Image alignment");
        case ProcPhase::QUALITY_ESTIMATION:  return _("Quality estimation");
        case ProcPhase::REF_POINT_ALIGNMENT: return _("Reference point alignment");
        case ProcPhase::IMAGE_STACKING:      return _("Image stacking");
        default: return "";
    }
}

} // namespace Worker
