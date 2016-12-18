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
    Worker thread implementation.
*/

#include <algorithm>
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
    /// Currently processed job
    static Job_t *job;
    static bool abortRequested = false;
    static size_t step;

    /// Used only when returning the best-quality image to the main thread
    static libskry::c_ImageAlignment *imgAlign = nullptr;
    static libskry::c_QualityEstimation *qualEst = nullptr;

    static bool isWorkerRunning = false;
    static ProcPhase procPhase = ProcPhase::IDLE;
    static Glib::Threads::Thread *workerThread;
    static Glib::Dispatcher dispatcher;
    static Cairo::RefPtr<Cairo::ImageSurface> visualizationImg;
    static bool enableVisualization = false;
    static bool isWaitingForReferencePoints;
    static enum SKRY_result lastResult;
    static Glib::Threads::RecMutex mtx; ///< Access guard for shared variables
    /// Used for notification by main thread that reference points have been provided
    static Glib::Threads::Mutex mtxRefPt;
    static Glib::Threads::Cond condRefPt;
    /// Current zoom factor specified in the main window's visualization widget
    static double zoomFactor = 1.0;
    /// Current zoom interpolation method specified in the main window's visualization widget
    static auto interpolationMethod = Utils::Const::Defaults::interpolation;
}

#define LOCK() Glib::Threads::RecMutex::Lock lock(Vars::mtx)

void WorkerThreadFunc();

static libskry::c_Image GetAlignedCurrentImage(
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

/// Returns the last values set with SetZoomFactor()
std::tuple<double, Utils::Const::InterpolationMethod> GetZoomFactor()
{
    LOCK();

    return std::make_tuple(Vars::zoomFactor, Vars::interpolationMethod);
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

void StartProcessing(Job_t *job)
{
    job->quality.framesChrono.clear();
    job->quality.framesSorted.clear();
    job->qualityDataReadyNotification = false;

    Vars::visualizationImg = Cairo::RefPtr<Cairo::ImageSurface>(nullptr);

    job->stackedImg = libskry::c_Image();
    job->bestFragmentsImg = libskry::c_Image();

    Vars::job = job;

    Vars::isWorkerRunning = true;
    Vars::abortRequested = false;
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
    Vars::visualizationImg = GetScaledImg(Vars::job->imgSeq.GetCurrentImage());
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
    Vars::visualizationImg = GetScaledImg(GetAlignedCurrentImage(imgSeq, imgAlignment));

    //TODO: draw something?.. e.g. image in grayscale with quality color-mapped
}

static
libskry::c_Image GetAlignedImage(
    size_t imgIdx, ///< Image index within the active images' subset
    const libskry::c_ImageSequence &imgSeq,
    const libskry::c_ImageAlignment &imgAlignment)
{
    libskry::c_Image img = imgSeq.GetImageByIdx(imgSeq.GetAbsoluteImgIdx(imgIdx));
    if (!img)
        return libskry::c_Image();

    img = libskry::c_Image::ConvertPixelFormat(img, SKRY_PIX_BGRA8, SKRY_DEMOSAIC_HQLINEAR);

    struct SKRY_rect intersection = imgAlignment.GetIntersection();

    struct SKRY_point offset = imgAlignment.GetImageOffset(imgIdx);
    int xmin = intersection.x + offset.x;
    int ymin = intersection.y + offset.y;

    struct SKRY_palette srcPal;
    img.GetPalette(srcPal);
    libskry::c_Image alignedImg(intersection.width, intersection.height, img.GetPixelFormat(), &srcPal, false);
    libskry::c_Image::ResizeAndTranslate(img, alignedImg, xmin, ymin,
                                         intersection.width, intersection.height, 0, 0, false);
    return alignedImg;
}

static libskry::c_Image GetAlignedCurrentImage(
    const libskry::c_ImageSequence &imgSeq,
    const libskry::c_ImageAlignment &imgAlignment)
{
    return GetAlignedImage(imgSeq.GetCurrentImgIdxWithinActiveSubset(), imgSeq, imgAlignment);
}

static void CreateRefPtAlignmentVisualization(
    const libskry::c_ImageSequence &imgSeq,
    const libskry::c_ImageAlignment &imgAlignment,
    const libskry::c_RefPointAlignment &refPtAlignment)
{
    Vars::visualizationImg = GetScaledImg(GetAlignedCurrentImage(imgSeq, imgAlignment));
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

/// Returns true if the main thread needs to provide reference points
bool IsWaitingForReferencePoints()
{
    return Vars::isWaitingForReferencePoints;
}

/// Notifies the worker thread that it may continue
void NotifyReferencePointsSet()
{
    Vars::isWaitingForReferencePoints = false;
    Glib::Threads::Mutex::Lock lock(Vars::mtxRefPt);
    Vars::condRefPt.signal();
}

/// Can be called after quality estimation completes
libskry::c_Image GetBestQualityAlignedImage()
{
    return GetAlignedImage(Vars::qualEst->GetBestImageIdx(),
                           Vars::job->imgSeq,
                           *Vars::imgAlign);

}

class c_PtrReset
{
public:
    ~c_PtrReset()
    {
        Vars::imgAlign = nullptr;
        Vars::qualEst = nullptr;
    }
};

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
    c_PtrReset ptrReset;

    libskry::c_ImageAlignment imgAlignment(
            Vars::job->imgSeq,
            Vars::job->alignmentMethod,
            Vars::job->anchors,
            Utils::Const::imgAlignmentRefBlockSize/2,
            Utils::Const::imgAlignmentRefBlockSize/2,
            Utils::Const::Defaults::placementBrightnessThreshold);

    if (!imgAlignment)
    {
        std::cerr << "Could not initialize image alignment." << std::endl;
        { LOCK();
            Vars::isWorkerRunning = false;
            NotifyMainThread();
            return;
        }
    }

    Vars::imgAlign = &imgAlignment;

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
    Vars::qualEst = &qualEstimation;


    { LOCK();
        StartProcessingPhase(ProcPhase::QUALITY_ESTIMATION);
    }
    while (SKRY_SUCCESS == (Vars::lastResult = qualEstimation.Step()))
    {
        { LOCK();
            CHECK_ABORT();
            Vars::step++;
            if (Vars::enableVisualization)
                CreateQualityEstimationVisualization(Vars::job->imgSeq, imgAlignment, qualEstimation);
        }
        NotifyMainThread();
    }
    { LOCK();

        if (Vars::lastResult != SKRY_LAST_STEP)
        {
            Vars::isWorkerRunning = false;
            NotifyMainThread();
            return;
        }
        else
        {
            Vars::job->bestFragmentsImg = qualEstimation.GetBestFragmentsImage();

            Vars::job->quality.framesChrono = qualEstimation.GetImagesQuality();
            Vars::job->quality.framesSorted = Vars::job->quality.framesChrono;

            // Sort descending
            std::sort(Vars::job->quality.framesSorted.begin(),
                      Vars::job->quality.framesSorted.end(),
                      [](const SKRY_quality_t &a, const SKRY_quality_t &b) { return a > b; });

            Vars::job->qualityDataReadyNotification = true;
            NotifyMainThread(); // in order to refresh the quality graph window
        }
    }

    if (!Vars::job->automaticRefPointsPlacement && Vars::job->refPoints.empty())
    {
        { LOCK();
            Vars::isWaitingForReferencePoints = true;
            NotifyMainThread();
        }

        { Glib::Threads::Mutex::Lock lock(Vars::mtxRefPt);

            Vars::condRefPt.wait(Vars::mtxRefPt);
            if (Vars::job->refPoints.empty()) // the user canceled the "Select ref. points" dialog
            {
                Vars::job->automaticRefPointsPlacement = true;
            }
        }
    }

    libskry::c_RefPointAlignment refPtAlignment(qualEstimation,
                                                Vars::job->refPoints,

                                                Vars::job->quality.criterion,
                                                Vars::job->quality.threshold,

                                                Vars::job->refPtBlockSize,
                                                Vars::job->refPtSearchRadius,
                                                &Vars::lastResult,
                                                Vars::job->refPtAutoPlacementParams.brightnessThreshold,
                                                Vars::job->refPtAutoPlacementParams.structureThreshold,
                                                Vars::job->refPtAutoPlacementParams.structureScale,
                                                Vars::job->refPtAutoPlacementParams.spacing);
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
                CreateRefPtAlignmentVisualization(Vars::job->imgSeq, imgAlignment, refPtAlignment);
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
    if (!Vars::job->flatFieldFileName.empty())
    {
        flatField = libskry::c_Image::Load(Vars::job->flatFieldFileName.c_str(), &Vars::lastResult);
        if (!flatField)
        {
            std::cerr << "Could not load flat-field from " << Vars::job->flatFieldFileName << std::endl;
            NotifyMainThread();
            return;
        }
    }

    libskry::c_Stacking stacking(refPtAlignment,
                                 Vars::job->flatFieldFileName.empty() ? nullptr : &flatField,
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

    { LOCK();
        Vars::job->stackedImg = stacking.GetFinalImageStack();
    }
    if (!Vars::job->stackedImg)
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

Glib::Threads::RecMutex &GetAccessGuard()
{
    return Vars::mtx;
}

const Cairo::RefPtr<Cairo::ImageSurface> &GetVisualizationImage()
{
    return Vars::visualizationImg;
}

std::unique_ptr<int> sth;

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
