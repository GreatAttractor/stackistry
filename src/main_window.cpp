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
    Main window implementation.
*/

#include <cassert>
#include <iomanip>
#include <iostream>
#include <vector>
#include <string>

#include <gdkmm.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <glibmm/threads.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <gtkmm/cellrendererprogress.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/stock.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/treemodelcolumn.h>
#if defined(_OPENMP)
#include <omp.h>
#endif

#include "select_points.h"
#include "config.h"
#include "frame_select.h"
#include "main_window.h"
#include "preferences.h"
#include "settings_dlg.h"
#include "utils.h"
#include "worker.h"


#define LOCK() Glib::Threads::Mutex::Lock lock(Worker::GetAccessGuard())

#define VERSION_MAJOR 0
#define VERSION_MINOR 0
#define VERSION_SUBMINOR 2
#define VERSION_DATE "2016-05-08"

namespace ActionName
{
    const char *quit = "quit";
    const char *addFolders = "add_folders";
    const char *addImages = "add_images";
    const char *addVideos = "add_videos";
    const char *startProcessing = "start_processing";
    const char *pauseResumeProcessing = "pause_resume_processing";
    const char *stopProcessing = "stop_processing";
    const char *setAnchors = "set_anchors";
    const char *saveStackedImage = "save_stacked_image";
    const char *selectFrames = "select_frames";
    const char *preferences = "preferences";
    const char *settings = "settings";
    const char *toggleVisualization = "toggle_visualization";
    const char *removeJobs = "remove_jobs";
    const char *createFlatField = "create_flat-field";
    const char *about = "about";

    //TESTING ###############
    const char *QUALITY_TEST = "quality_test";
    const char *backgroundTest = "background_test";
    //END TESTING ###############
}

namespace WidgetName
{
    const char *Toolbar = "Toolbar";
    const char *JobsCtxMenu = "JobsCtxMenu";
    const char *MenuBar = "MenuBar";
    const char *MenuFile = "MenuFile";
    const char *MenuEdit = "MenuEdit";
    const char *MenuProcessing = "MenuProcessing";
    const char *MenuTest = "MenuTest";
    const char *MenuAbout = "MenuAbout";
}

static
libskry::c_Image GetFirstActiveImage(libskry::c_ImageSequence &imgSeq, enum SKRY_result &result)
{
    imgSeq.SeekStart();
    return imgSeq.GetCurrentImage(&result);
}

void ShowMsg(Gtk::Window &parent, std::string title, std::string message, Gtk::MessageType msgType)
{
    Gtk::MessageDialog msg(parent, message,
                       false, msgType, Gtk::ButtonsType::BUTTONS_OK, true);
    msg.set_title(title);
    msg.run();
}

Gtk::TreeModel::Path c_MainWindow::GetJobsListFocusedRow()
{
    Gtk::TreeModel::Path path;
    Gtk::TreeViewColumn *focus_column;
    m_Jobs.view.get_cursor(path, focus_column);
    return path;
}

c_MainWindow::Job_t &c_MainWindow::GetJobAt(const Gtk::TreeModel::Path &path)
{
    return GetJobAt(m_Jobs.data->get_iter(path));
}

c_MainWindow::Job_t &c_MainWindow::GetJobAt(const Gtk::ListStore::iterator &iter)
{
    return *((decltype(m_Jobs.columns.job)::ElementType)((*iter)[m_Jobs.columns.job]));
}

void c_MainWindow::OnAddFolders()
{
    //
}

void c_MainWindow::OnAddImageSeries()
{
    Gtk::FileChooserDialog dlg(*this, "Add image series", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_OPEN);
    dlg.add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    dlg.add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    dlg.set_current_folder(Configuration::LastOpenDir);
    dlg.set_select_multiple();

    auto fltAll = Gtk::FileFilter::create();
    fltAll->add_pattern("*");
    fltAll->set_name("All files");
    dlg.add_filter(fltAll);

    auto fltBmp = Gtk::FileFilter::create();
    fltBmp->add_pattern("*.bmp");
    fltBmp->set_name("BMP (*.bmp)");
    dlg.add_filter(fltBmp);

    PrepareDialog(dlg);
    if (Gtk::ResponseType::RESPONSE_OK == dlg.run())
    {
        std::vector<std::string> fileNames = dlg.get_filenames();
        if (fileNames.size() <= 1)
        {
            ShowMsg(dlg, "Error", "At least two image files are required.", Gtk::MessageType::MESSAGE_ERROR);
            return;
        }

        std::shared_ptr<Job_t> newJob = std::make_shared<Job_t>(Job_t { libskry::c_ImageSequence::InitImageList(fileNames) });
        newJob->sourcePath = Glib::path_get_dirname(fileNames[0]);
        SetDefaultSettings(*newJob);

        if (!newJob->imgSeq)
        {
            std::cout << "Failed to initialize image sequence." << std::endl;
        }
        else
        {
            auto row = *m_Jobs.data->append();
            row[m_Jobs.columns.jobSource] = newJob->sourcePath;
            row[m_Jobs.columns.state]     = "Waiting";
            row[m_Jobs.columns.progressText] = "";
            row[m_Jobs.columns.job]       = newJob;
        }
    }
    Configuration::LastOpenDir = dlg.get_current_folder();
}

void c_MainWindow::PrepareDialog(Gtk::Dialog &dlg)
{
    dlg.set_modal();
    dlg.set_transient_for(*this);
}

void c_MainWindow::OnStartProcessing()
{
    if (Worker::IsRunning())
    {
        std::cerr << "Cannot start processing, worker is still running." << std::endl;
        return;
    }

    while (!m_JobsToProcess.empty())
        m_JobsToProcess.pop();

    for (auto &row: m_Jobs.view.get_selection()->get_selected_rows())
        m_JobsToProcess.push(row);

    m_RunningJob = m_Jobs.data->get_iter(m_JobsToProcess.front());
    m_JobsToProcess.pop();
    Job_t &job = GetJobAt(m_RunningJob);
    if (job.anchors.empty())
    {
        if (!SetAnchors(job))
        {
            m_RunningJob = Gtk::ListStore::iterator(nullptr);
            return;
        }
    }

    Worker::StartProcessing(job.imgSeq, job.anchors, job.refPtSpacing,
                            job.automaticRefPointsPlacement,
                            job.refPoints,
                            job.qualityCriterion, job.qualityThreshold, job.flatFieldFileName);
    UpdateActionsState();
}

bool c_MainWindow::SetAnchorsAutomatically(Job_t &job)
{
    enum SKRY_result result;
    libskry::c_Image firstImg = GetFirstActiveImage(job.imgSeq, result);
    if (!firstImg)
    {
        ShowMsg(*this, "Error", std::string("Error loading the first image of ") + job.sourcePath + ":\n" +
                                SKRY_get_error_message(result), Gtk::MessageType::MESSAGE_ERROR);
        return false;
    }
    job.anchors.push_back(libskry::c_ImageAlignment::SuggestAnchorPos(
        firstImg, Utils::Const::Defaults::placementBrightnessThreshold, Utils::Const::imgAlignmentRefBlockSize));
    return true;
}

void c_MainWindow::OnPauseResumeProcessing()
{
}

void c_MainWindow::OnQualityTest()
{
//    Gtk::FileChooserDialog dlg(*this, "Select image", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_OPEN);
//    dlg.add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
//    dlg.add_button("OK", Gtk::ResponseType::RESPONSE_OK);
//    dlg.set_modal();
//    if (Gtk::ResponseType::RESPONSE_OK == dlg.run())
//    {
//        libskry::c_Image img = libskry::c_Image::Load(dlg.get_filename().c_str(), 0);
//        if (!img)
//            std::cout << "Couldn't load " << dlg.get_filename() << std::endl;
//        else
//        {
//            libskry::c_Image img8 = libskry::c_Image::ConvertPixelFormat(img, SKRY_PIX_MONO8);
//
//
//#define DO_LOOP 1
//
//#if DO_LOOP
//            for (unsigned blur_radius = 1; blur_radius < 30; blur_radius++)
//#endif
//            {
//
//                libskry::c_Image blurred = img8.BoxBlur(
//                                                       #if DO_LOOP
//                                                       blur_radius,
//                                                       #else
//                                                       4,
//                                                       #endif
//                                                       3);
//
//                libskry::c_Image qimg(img8.GetWidth(), img8.GetHeight(), SKRY_PIX_MONO8, nullptr, false);
//
//                ptrdiff_t imgStrd = img8.GetLineStrideInBytes(),
//                          blurredStrd = blurred.GetLineStrideInBytes(),
//                          qimgStrd = qimg.GetLineStrideInBytes();
//
//                uint8_t *imgLn = (uint8_t *)img8.GetLine(0);
//                uint8_t *blurredLn = (uint8_t *)blurred.GetLine(0);
//                uint8_t *qimgLn = (uint8_t *)qimg.GetLine(0);
//
//                uint64_t maxDiff = 0;
//                uint64_t sumDiffs = 0;
//                for (unsigned y = 0; y < img8.GetHeight(); y++)
//                {
//                    for (unsigned x = 0; x < img8.GetWidth(); x++)
//                    {
//                        uint64_t diff = std::abs(imgLn[x] - blurredLn[x]);
//                        qimgLn[x] = diff;
//
//                        sumDiffs += diff;
//                        if (diff > maxDiff)
//                            maxDiff = diff;
//                    }
//
//                    imgLn += imgStrd;
//                    blurredLn += blurredStrd;
//                    qimgLn += qimgStrd;
//                }
//
//
//                std::cout <<
//            #if DO_LOOP
//                "Radius: " << blur_radius <<
//            #endif
//                "; max diff = " << maxDiff << ", sum = " << sumDiffs << std::endl;
//
//                qimgLn = (uint8_t *)qimg.GetLine(0);
//                for (unsigned y = 0; y < img8.GetHeight(); y++)
//                {
//                    for (unsigned x = 0; x < img8.GetWidth(); x++)
//                    {
//
//                        qimgLn[x] = (qimgLn[x] * 0xFF / maxDiff);
//                    }
//                    qimgLn += qimgStrd;
//                }
//
//            #if DO_LOOP
//                auto surface = Utils::ConvertImgToSurface(qimg);
//                auto pixbuf = Gdk::Pixbuf::create(surface,  0, 0, qimg.GetWidth(), qimg.GetHeight());
//                pixbuf->save(Glib::ustring("img_diff") + Glib::ustring::format(std::setfill(L'0'), std::setw(2), blur_radius), "bmp");
//            #endif
//                m_Visualization.SetImage(qimg);
//            }
//        }
//    }
}

void c_MainWindow::OnStopProcessing()
{
    Worker::AbortProcessing();
    if (m_RunningJob)
    {
        (*m_RunningJob)[m_Jobs.columns.progress] = 0;
        (*m_RunningJob)[m_Jobs.columns.percentageProgress] = 0;
        (*m_RunningJob)[m_Jobs.columns.progressText] = "";
        (*m_RunningJob)[m_Jobs.columns.state] = "Waiting";
        GetJobAt(m_RunningJob).imgSeq.Deactivate();

        m_RunningJob = Gtk::ListStore::iterator(nullptr);
        SetStatusBarText("Idle");
    }
    while (!m_JobsToProcess.empty())
        m_JobsToProcess.pop();

    UpdateActionsState();
}

c_MainWindow::Job_t &c_MainWindow::GetCurrentJob()
{
    Gtk::ListStore::iterator itJob = m_Jobs.data->get_iter(GetJobsListFocusedRow());
    return GetJobAt(itJob);
}

bool c_MainWindow::SetAnchors(Job_t &job)
{
    enum SKRY_result result;
    libskry::c_Image firstImg = GetFirstActiveImage(job.imgSeq, result);
    if (!firstImg)
    {
        ShowMsg(*this, "Error", std::string("Error loading the first image of ") + job.sourcePath + ":\n" +
                                SKRY_get_error_message(result), Gtk::MessageType::MESSAGE_ERROR);

        return false;
    }

    c_SelectPointsDlg dlg(firstImg, job.anchors,
                          { libskry::c_ImageAlignment::SuggestAnchorPos(
                              firstImg, Utils::Const::Defaults::placementBrightnessThreshold,
                              Utils::Const::imgAlignmentRefBlockSize) });
    dlg.set_title("Set video stabilization anchors");
    dlg.SetInfoText("Automatic placement should work reliably in most cases. "
                    "Otherwise, click the image to place anchors (one anchor is sufficient if there is not much image drift).");
    PrepareDialog(dlg);
    Utils::RestorePosSize(Configuration::AnchorSelectDlgPosSize, dlg);
    auto response = dlg.run();
    if (response == Gtk::ResponseType::RESPONSE_OK)
    {
        dlg.GetPoints(job.anchors);
        // Modified anchors may modify video size and target framing
        // after image alignment, so old ref. points may be invalid
        job.refPoints.clear();
    }
    job.imgSeq.Deactivate();
    Utils::SavePosSize(dlg, Configuration::AnchorSelectDlgPosSize);
    return (response == Gtk::ResponseType::RESPONSE_OK);
}

void c_MainWindow::OnSetAnchors()
{
    SetAnchors(GetCurrentJob());
    GetCurrentJob().imgSeq.Deactivate();
}

static
void GetOutputFormatFromFilter(Glib::ustring filterName,
                               enum SKRY_output_format &outputFmt)
{
    for (auto &outfDescr: Utils::Vars::outputFormatDescription)
        if (filterName == outfDescr.name)
        {
            outputFmt = outfDescr.skryOutpFmt;
            return;
        }

    assert(0);
}

void c_MainWindow::OnSaveStackedImage()
{
    Gtk::FileChooserDialog dlg("Save stacked image", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SAVE);
    dlg.add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    dlg.add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    bool hiBitDepthFilterSelected = false;
    for (Utils::Vars::OutputFormatDescr_t &outFmt: Utils::Vars::outputFormatDescription)
    {
        auto filter = Gtk::FileFilter::create();
        Glib::ustring filterName = outFmt.name;
        for (auto &pattern: outFmt.patterns)
        {
            filter->add_pattern(pattern);
        }
        filter->set_name(filterName);
        dlg.add_filter(filter);

        if (!hiBitDepthFilterSelected && OUTPUT_FMT_BITS_PER_CHANNEL[outFmt.skryOutpFmt] >= 16)
        {
            dlg.set_filter(filter);
            hiBitDepthFilterSelected = true;
        }
    }

    PrepareDialog(dlg);
    if (dlg.run() == Gtk::ResponseType::RESPONSE_OK)
    {
        enum SKRY_output_format outpFmt;

        GetOutputFormatFromFilter(dlg.get_filter()->get_name(), outpFmt);
        const libskry::c_Image &stackedImg = GetJobAt(GetJobsListFocusedRow()).stackedImg;
        enum SKRY_pixel_format pixFmt = Utils::FindMatchingFormat(outpFmt, NUM_CHANNELS[stackedImg.GetPixelFormat()]);
        libskry::c_Image finalImg = libskry::c_Image::ConvertPixelFormat(stackedImg, pixFmt);
        enum SKRY_result result;
        if (SKRY_SUCCESS != (result = finalImg.Save(dlg.get_filename().c_str(), outpFmt)))
        {
            std::cout << "Failed to save image" << std::endl;

            ShowMsg(*this, "Error", std::string("Error saving output image ") + dlg.get_filename() + ":\n" +
                                    SKRY_get_error_message(result), Gtk::MessageType::MESSAGE_ERROR);
        }
    }
}

void c_MainWindow::OnSettings()
{
    Job_t &job = GetJobAt(m_Jobs.view.get_selection()->get_selected_rows()[0]);

    std::vector<std::string> selJobNames;
    for (auto &selJob: m_Jobs.view.get_selection()->get_selected_rows())
        selJobNames.push_back(GetJobAt(selJob).sourcePath);

    c_SettingsDlg dlg(selJobNames);

    // Fill the dialog using settings of the first selected job
    dlg.SetOutputSaveMode(job.outputSaveMode);
    dlg.SetAutoSaveOutputFormat(job.outputFmt);
    dlg.SetDestinationDir(job.destDir);
    dlg.SetRefPtSpacing(job.refPtSpacing);
    dlg.SetRefPointsAutomatic(job.automaticRefPointsPlacement);
    dlg.SetFlatFieldFileName(job.flatFieldFileName);
    dlg.SetAnchorsAutomatic(!job.anchors.empty());
    dlg.SetQualityCriterion(job.qualityCriterion, job.qualityThreshold);
    dlg.SetCFAPattern(job.cfaPattern);

    PrepareDialog(dlg);
    if (Gtk::ResponseType::RESPONSE_OK == dlg.run())
        for (auto &selJob: m_Jobs.view.get_selection()->get_selected_rows())
        {
            Job_t &job = GetJobAt(selJob);
            auto smode = dlg.GetOutputSaveMode();
            job.outputSaveMode = smode;
            if (smode == Utils::Const::OutputSaveMode::SPECIFIED_PATH)
            {
                job.destDir = dlg.GetDestinationDir();
            }
            job.refPtSpacing = dlg.GetRefPointSpacing();
            job.automaticRefPointsPlacement = dlg.GetRefPointsAutomatic();
            if (job.automaticRefPointsPlacement)
            {
                job.refPoints.clear();
            }
            job.flatFieldFileName = dlg.GetFlatFieldFileName();
            job.qualityCriterion = dlg.GetQualityCriterion();
            job.qualityThreshold = dlg.GetQualityThreshold();
            job.outputFmt = dlg.GetAutoSaveOutputFormat();
            job.cfaPattern = dlg.GetCFAPattern();
            job.imgSeq.ReinterpretAsCFA(job.cfaPattern);

            job.anchors.clear();
            if (dlg.GetAnchorsAutomatic())
            {
                job.imgSeq.SeekStart();
                job.anchors.push_back(libskry::c_ImageAlignment::SuggestAnchorPos(job.imgSeq.GetCurrentImage(),
                                                                                  Utils::Const::Defaults::placementBrightnessThreshold,
                                                                                  Utils::Const::imgAlignmentRefBlockSize));
            }
        }
}

/// The location to enforce all action-sensitivity conditions
void c_MainWindow::UpdateActionsState()
{
    size_t numSelJobs = m_Jobs.view.get_selection()->get_selected_rows().size();

    for (auto &action: { ActionName::pauseResumeProcessing,
                         ActionName::stopProcessing })
    {
        m_ActionGroup->get_action(action)->set_sensitive(Worker::IsRunning());
    }

    for (auto &action: { ActionName::addFolders,
                         ActionName::addImages,
                         ActionName::addVideos })
    {
        m_ActionGroup->get_action(action)->set_sensitive(!Worker::IsRunning());
    }

    m_ActionGroup->get_action(ActionName::startProcessing)->set_sensitive(numSelJobs > 0 && !Worker::IsRunning());

    for (auto &action: { ActionName::setAnchors,
                         ActionName::selectFrames })
    {
        m_ActionGroup->get_action(action)->set_sensitive(
            numSelJobs == 1
            && GetJobsListFocusedRow()
            && !(Worker::IsRunning() && m_RunningJob == m_Jobs.data->get_iter(GetJobsListFocusedRow()))
        );
    }

    m_ActionGroup->get_action(ActionName::settings)->set_sensitive(numSelJobs > 0);
    m_ActionGroup->get_action(ActionName::removeJobs)->set_sensitive(numSelJobs > 0 && !Worker::IsRunning());

    m_ActionGroup->get_action(ActionName::saveStackedImage)->set_sensitive(
        numSelJobs == 1
        && GetJobsListFocusedRow()
        && GetJobAt(GetJobsListFocusedRow()).stackedImg.IsValid()
    );
}

void c_MainWindow::OnSelectFrames()
{
    c_FrameSelectDlg dlg(GetCurrentJob().imgSeq);
    PrepareDialog(dlg);
    if (dlg.run() == Gtk::ResponseType::RESPONSE_OK)
    {
        GetCurrentJob().imgSeq.SetActiveImages(dlg.GetActiveFlags());
        // Modified frame selection may modify video size and target framing
        // after image alignment, so old ref. points may be invalid
        GetCurrentJob().refPoints.clear();
    }
    GetCurrentJob().imgSeq.Deactivate();
}

void c_MainWindow::SetDefaultSettings(c_MainWindow::Job_t &job)
{
    assert(job.imgSeq);

    job.outputSaveMode = Utils::Const::Defaults::saveMode;
    job.outputFmt = Utils::Const::Defaults::outputFmt;
    job.refPtSpacing = Utils::Const::Defaults::referencePointSpacing;
    job.qualityCriterion = Utils::Const::Defaults::qualityCriterion;
    job.qualityThreshold = Utils::Const::Defaults::qualityThreshold;
    job.automaticRefPointsPlacement = true;
    job.cfaPattern = SKRY_CFA_NONE;
    enum SKRY_result result;
    libskry::c_Image firstImg = GetFirstActiveImage(job.imgSeq, result);
    if (!firstImg)
    {
        ShowMsg(*this, "Error", std::string("Error loading the first image of ") + job.sourcePath + ":\n" +
                                SKRY_get_error_message(result), Gtk::MessageType::MESSAGE_ERROR);
    }
    else
        job.anchors.push_back(libskry::c_ImageAlignment::SuggestAnchorPos(
                                job.imgSeq.GetCurrentImage(),
                                Utils::Const::Defaults::placementBrightnessThreshold,
                                Utils::Const::imgAlignmentRefBlockSize));
}

void c_MainWindow::OnAddVideos()
{
    Gtk::FileChooserDialog dlg(*this, "Add video(s)", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_OPEN);
    dlg.add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    dlg.add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    dlg.set_current_folder(Configuration::LastOpenDir);
    dlg.set_select_multiple();

    auto fltAll = Gtk::FileFilter::create();
    fltAll->add_pattern("*");
    fltAll->set_name("All files");
    dlg.add_filter(fltAll);

    auto fltAvi = Gtk::FileFilter::create();
    fltAvi->add_pattern("*.avi");
    fltAvi->set_name("AVI (*.avi)");
    dlg.add_filter(fltAvi);

    auto fltSer = Gtk::FileFilter::create();
    fltSer->add_pattern("*.ser");
    fltSer->set_name("SER (*.ser)");
    dlg.add_filter(fltSer);

    PrepareDialog(dlg);
    if (Gtk::ResponseType::RESPONSE_OK == dlg.run())
    {
        for (auto &fname: dlg.get_filenames())
        {
            enum SKRY_result result;
            std::shared_ptr<Job_t> newJob = std::make_shared<Job_t>(Job_t { libskry::c_ImageSequence::InitVideoFile(fname.c_str(), &result) });
            if (!newJob->imgSeq)
            {
                ShowMsg(dlg, "Error", std::string("Could not open video file ") + fname + ":\n" +
                        SKRY_get_error_message(result), Gtk::MessageType::MESSAGE_ERROR);
                continue;
            }

            newJob->sourcePath = fname;
            SetDefaultSettings(*newJob);

            if (!newJob->imgSeq)
            {
                std::cout << "Failed to initialize image sequence." << std::endl;
            }
            else
            {
                auto row = *m_Jobs.data->append();
                row[m_Jobs.columns.jobSource] = newJob->sourcePath;
                row[m_Jobs.columns.state]     = "Waiting";
                row[m_Jobs.columns.progressText] = "";
                row[m_Jobs.columns.job]       = newJob;
            }
        }
    }
    Configuration::LastOpenDir = dlg.get_current_folder();
}

void c_MainWindow::OnBackgroundTest()
{
//    Gtk::FileChooserDialog dlg(*this, "Select image", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_OPEN);
//    dlg.add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
//    dlg.add_button("OK", Gtk::ResponseType::RESPONSE_OK);
//    dlg.set_modal();
//    if (Gtk::ResponseType::RESPONSE_OK == dlg.run())
//    {
//        libskry::c_Image img = libskry::c_Image::Load(dlg.get_filename().c_str(), 0);
//        if (!img)
//            std::cout << "Couldn't load " << dlg.get_filename() << std::endl;
//        else
//        {
//            libskry::c_Image img8 = libskry::c_Image::ConvertPixelFormat(img, SKRY_PIX_MONO8);
//            uint8_t bthreshold = img8.GetBackgroundThreshold();
//            std::cout << "Background threshold is " << (unsigned)bthreshold << std::endl;
//
//            for (unsigned y = 0; y < img8.GetHeight(); y++)
//            {
//                uint8_t *line = (uint8_t *)img8.GetLine(y);
//                for (unsigned x = 0; x < img8.GetWidth(); x++)
//                    if (line[x] <= bthreshold)
//                        line[x] = 0xBB;
//            }
//
//            m_Visualization.SetImage(img8, true);
//        }
//    }
}

void c_MainWindow::OnQuit()
{
    close();
}

void c_MainWindow::InitActions()
{
    m_ActionGroup = Gtk::ActionGroup::create();
    m_ActionGroup->add(Gtk::Action::create(WidgetName::MenuFile, "_File"));
    m_ActionGroup->add(Gtk::Action::create(ActionName::quit, Gtk::Stock::QUIT),
                  sigc::mem_fun(*this, &c_MainWindow::OnQuit));
    m_ActionGroup->add(Gtk::Action::create(ActionName::addImages, "Add image series...", "Add image series..."),
                  sigc::mem_fun(*this, &c_MainWindow::OnAddImageSeries));
    m_ActionGroup->add(Gtk::Action::create(ActionName::addFolders, "Add image series from folder(s)..."),
                  sigc::mem_fun(*this, &c_MainWindow::OnAddFolders));
    m_ActionGroup->add(Gtk::Action::create(ActionName::addVideos, "Add video(s)...", "Add video(s)..."),
                  sigc::mem_fun(*this, &c_MainWindow::OnAddVideos));
    m_ActionGroup->add(Gtk::Action::create(ActionName::saveStackedImage, "Save stacked image..."),
                  sigc::mem_fun(*this, &c_MainWindow::OnSaveStackedImage));
    m_ActionGroup->add(Gtk::Action::create(ActionName::createFlatField, "Create flat-field from video...",
                                           "Create flat-field from video..."),
                  sigc::mem_fun(*this, &c_MainWindow::OnCreateFlatField));

    m_ActionGroup->add(Gtk::Action::create(WidgetName::MenuEdit, "_Edit"));
    m_ActionGroup->add(Gtk::Action::create(ActionName::setAnchors, "Set video stabilization anchors..."),
                  sigc::mem_fun(*this, &c_MainWindow::OnSetAnchors));
    m_ActionGroup->add(Gtk::Action::create(ActionName::selectFrames, "Select frames..."),
                  sigc::mem_fun(*this, &c_MainWindow::OnSelectFrames));
    m_ActionGroup->add(Gtk::Action::create(ActionName::settings, "Processing settings...",
                                           "Processing settings..."),
                  sigc::mem_fun(*this, &c_MainWindow::OnSettings));
    m_ActionGroup->add(Gtk::Action::create(ActionName::removeJobs, "Remove from list", "Remove from list"),
                  sigc::mem_fun(*this, &c_MainWindow::OnRemoveJobs));
    m_ActionGroup->add(Gtk::Action::create(ActionName::preferences, "Preferences..."),
                  sigc::mem_fun(*this, &c_MainWindow::OnPreferences));

    m_ActionGroup->add(Gtk::Action::create(WidgetName::MenuProcessing, "_Processing"));
    m_ActionGroup->add(Gtk::Action::create(ActionName::startProcessing, "Start processing",
                                           "Start processing selected job(s)"),
                  sigc::mem_fun(*this, &c_MainWindow::OnStartProcessing));
    m_ActionGroup->add(Gtk::Action::create(ActionName::pauseResumeProcessing, "Pause processing"),
                  sigc::mem_fun(*this, &c_MainWindow::OnPauseResumeProcessing));
    m_ActionGroup->add(Gtk::Action::create(ActionName::stopProcessing, "Stop processing", "Stop processing"),
                  sigc::mem_fun(*this, &c_MainWindow::OnStopProcessing));
    m_ActVisualization = Gtk::ToggleAction::create(ActionName::toggleVisualization, "Show visualization", "Show visualization (slows down processing)");
    m_ActionGroup->add(m_ActVisualization, sigc::mem_fun(*this, &c_MainWindow::OnToggleVisualization));

    m_ActionGroup->add(Gtk::Action::create(WidgetName::MenuTest, "_Test"));
    m_ActionGroup->add(Gtk::Action::create(ActionName::QUALITY_TEST, "Quality..."),
                  sigc::mem_fun(*this, &c_MainWindow::OnQualityTest));
    m_ActionGroup->add(Gtk::Action::create(ActionName::backgroundTest, "Background..."),
                  sigc::mem_fun(*this, &c_MainWindow::OnBackgroundTest));

    m_ActionGroup->add(Gtk::Action::create(WidgetName::MenuAbout, "_About"));
    m_ActionGroup->add(Gtk::Action::create(ActionName::about, "About Stackistry..."),
                  sigc::mem_fun(*this, &c_MainWindow::OnAbout));

    m_UIManager = Gtk::UIManager::create();
    m_UIManager->insert_action_group(m_ActionGroup);
    add_accel_group(m_UIManager->get_accel_group());

    Glib::ustring uiInfo =
    Glib::ustring("<ui>") +
    "    <menubar name='" + WidgetName::MenuBar + "'>"
    "        <menu action='" + WidgetName::MenuFile + "'>"
    "            <menuitem action='" + ActionName::addVideos + "' />"
    "            <menuitem action='" + ActionName::addImages + "' />"
    //"            <menuitem action='" + ActionName::addFolders + "' />"  TODO: implement this
    "            <menuitem action='" + ActionName::saveStackedImage + "' />"
    "            <menuitem action='" + ActionName::createFlatField + "' />"
    "            <separator />"
    "            <menuitem action='" + ActionName::quit + "' />"
    "        </menu>"

    "        <menu action='" + WidgetName::MenuEdit + "'>"
    "            <menuitem action='" + ActionName::selectFrames + "' />"
    "            <menuitem action='" + ActionName::settings + "' />"
    "            <menuitem action='" + ActionName::setAnchors + "' />"
    "            <separator />"
    "            <menuitem action='" + ActionName::removeJobs + "' />"
    "            <separator />"
    "            <menuitem action='" + ActionName::preferences + "' />"
    "        </menu>"

    "        <menu action='" + WidgetName::MenuProcessing + "'>"
    "            <menuitem action='" + ActionName::startProcessing + "' />"
    //"            <menuitem action='" + ActionName::pauseResumeProcessing + "' />"
    "            <menuitem action='" + ActionName::stopProcessing + "' />"
    "            <separator />"
    "            <menuitem action='" + ActionName::toggleVisualization + "' />"
    "        </menu>"

    "        <menu action='" + WidgetName::MenuAbout + "'>"
    "            <menuitem action='" + ActionName::about + "' />"
    "        </menu>"

//    "        <menu action='" + WidgetName::MenuTest + "'>"
//    "            <menuitem action='" + ActionName::QUALITY_TEST + "' />"
//    "            <menuitem action='" + ActionName::backgroundTest + "' />"
//    "        </menu>"

    "    </menubar>"

    "    <toolbar name='" + WidgetName::Toolbar + "'>"
    "       <toolitem name='" + ActionName::addVideos + "' action='" + ActionName::addVideos + "'/>"
    "       <toolitem name='" + ActionName::addImages + "' action='" + ActionName::addImages + "'/>"
    "       <toolitem name='" + ActionName::createFlatField + "' action='" + ActionName::createFlatField + "'/>"
    "       <separator/>"
    "       <toolitem name='" + ActionName::settings + "' action='" + ActionName::settings + "'/>"
    "       <toolitem name='" + ActionName::removeJobs + "' action='" + ActionName::removeJobs + "'/>"
    "       <separator/>"
    "       <toolitem name='" + ActionName::startProcessing + "' action='" + ActionName::startProcessing + "'/>"
    "       <toolitem name='" + ActionName::stopProcessing + "' action='" + ActionName::stopProcessing + "'/>"
    "       <separator/>"
    "       <toolitem action='" + ActionName::toggleVisualization + "'/>"
    "    </toolbar>"

    "    <popup name='" + WidgetName::JobsCtxMenu + "'>"
    "        <menuitem action='" + ActionName::selectFrames + "' />"
    "        <menuitem action='" + ActionName::settings + "' />"
    "        <menuitem action='" + ActionName::setAnchors + "' />"
    "        <menuitem action='" + ActionName::saveStackedImage + "' />"
    "        <separator/>"
    "        <menuitem action='" + ActionName::removeJobs + "'/>"
    "    </popup>"
    "</ui>";

    m_UIManager->add_ui_from_string(uiInfo);
    UpdateActionsState();
}

void c_MainWindow::OnJobCursorChanged()
{
    UpdateActionsState();
}

void c_MainWindow::CreateJobsListView()
{
    m_Jobs.data = Gtk::ListStore::create(m_Jobs.columns);
    m_Jobs.view.set_model(m_Jobs.data);

    m_Jobs.view.append_column("Job",  m_Jobs.columns.jobSource);
    m_Jobs.view.get_column(0)->set_resizable();
    auto crend = dynamic_cast<Gtk::CellRendererText *>(m_Jobs.view.get_column_cell_renderer(0));
    crend->property_ellipsize() = Pango::EllipsizeMode::ELLIPSIZE_START;

    m_Jobs.view.append_column("State", m_Jobs.columns.state);
    m_Jobs.view.get_column(1)->set_resizable();

    auto *crProgress = Gtk::manage(new Gtk::CellRendererProgress());
    m_Jobs.view.append_column("Progress", *crProgress);
    auto col = m_Jobs.view.get_column(2);
    col->add_attribute(crProgress->property_value(), m_Jobs.columns.percentageProgress);
    col->add_attribute(crProgress->property_text(), m_Jobs.columns.progressText);
    col->set_resizable();

    m_Jobs.view.signal_button_press_event().connect(sigc::mem_fun(*this, &c_MainWindow::OnJobBtnPressed), false);
    m_Jobs.view.signal_cursor_changed().connect(sigc::mem_fun(*this, &c_MainWindow::OnJobCursorChanged));
    m_Jobs.view.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &c_MainWindow::OnSelectionChanged));

    m_Jobs.view.set_tooltip_column(m_Jobs.columns.jobSource.index());
    m_Jobs.view.get_selection()->set_mode(Gtk::SelectionMode::SELECTION_MULTIPLE);

    m_Jobs.view.get_column(0)->set_fixed_width(Configuration::JobColWidth);
    m_Jobs.view.get_column(1)->set_fixed_width(Configuration::StateColWidth);
    m_Jobs.view.get_column(2)->set_fixed_width(Configuration::ProgressColWidth);

    m_Jobs.view.show();
}

bool c_MainWindow::OnJobBtnPressed(GdkEventButton *event)
{
    if (event->type == GDK_BUTTON_PRESS && event->button == Utils::Const::MouseButtons::RIGHT)
    {
        auto ctxMenu = dynamic_cast<Gtk::Menu *>(m_UIManager->get_widget(Glib::ustring("/") + WidgetName::JobsCtxMenu));
        ctxMenu->popup(event->button, event->time);

        if (m_Jobs.view.get_selection()->get_selected_rows().size() > 1)
            return true; // do not call default handler (it would deselect all rows and select the right-clicked one)
        else
            return false; // call default handler to select the right-clicked row
    }
    else
        return false;
}

void c_MainWindow::OnToggleVisualization()
{
    Worker::SetVisualizationEnabled(m_ActVisualization->get_active());
}

void c_MainWindow::SetStatusBarText(const Glib::ustring &text)
{
    m_StatusBar.remove_all_messages();
    m_StatusBar.push(text);
}

void c_MainWindow::AutoSaveStack(const Job_t &job)
{
    assert(job.stackedImg);

    enum SKRY_pixel_format pixFmt = Utils::FindMatchingFormat(job.outputFmt, NUM_CHANNELS[job.stackedImg.GetPixelFormat()]);
    libskry::c_Image convImg = libskry::c_Image::ConvertPixelFormat(job.stackedImg, pixFmt);

    std::string destDir;
    if (job.outputSaveMode == Utils::Const::OutputSaveMode::SOURCE_PATH)
    {
        destDir = (job.imgSeq.GetType() == SKRY_IMG_SEQ_IMAGE_FILES
                   ? job.sourcePath
                   : Glib::path_get_dirname(job.sourcePath));
    }
    else
        destDir = job.destDir;

    std::string destFName = (job.imgSeq.GetType() == SKRY_IMG_SEQ_IMAGE_FILES
                                ? "stack"
                                : Glib::path_get_basename(job.sourcePath) + "_stacked");
    std::string destExt = Utils::GetOutputFormatDescr(job.outputFmt).defaultExtension;

    unsigned replaceCounter = 0;
    while (Glib::file_test(Glib::build_filename(destDir, destFName +
                            (replaceCounter ? (std::string)Glib::ustring::format(replaceCounter) + destExt : destExt)),
                           Glib::FileTest::FILE_TEST_EXISTS))
    {
        replaceCounter++;
    }

    std::string destPath = Glib::build_filename(destDir, destFName +
                            (replaceCounter ? (std::string)Glib::ustring::format(replaceCounter) + destExt : destExt));
    if (SKRY_SUCCESS != convImg.Save(destPath.c_str(), job.outputFmt))
    {
        std::cout << "Could not save stack as " << destPath << std::endl;
    }
}

void c_MainWindow::OnWorkerProgress()
{
    LOCK();

    if (!m_RunningJob)
        return; // an outdated notification, ignore

    if (Worker::IsWaitingForReferencePoints())
    {
        struct Job_t &job = GetJobAt(*m_RunningJob);
        c_SelectPointsDlg dlg(Worker::GetAlignedFirstImage(), job.refPoints, { });
        dlg.set_title("Set reference points");
        dlg.SetInfoText("Place reference points by left-clicking on the image. Avoid blank areas "
                        "with little or no detail. Click Cancel to set points automatically.");
        PrepareDialog(dlg);
        Utils::RestorePosSize(Configuration::SelectRefPointsDlgPosSize, dlg);
        auto response = dlg.run();
        if (response == Gtk::ResponseType::RESPONSE_OK)
        {
            dlg.GetPoints(job.refPoints);
            Worker::SetReferencePoints(job.refPoints);
        }
        else
        {
            job.automaticRefPointsPlacement = true;
            Worker::SetReferencePoints({ });
        }
        Utils::SavePosSize(dlg, Configuration::SelectRefPointsDlgPosSize);
    }

    if (Worker::GetStep() != m_LastStepNotify)
    {
        if (Worker::IsVisualizationEnabled())
            if (Worker::GetVisualizationImage())
                m_Visualization.SetImage(Worker::GetVisualizationImage());

        (*m_RunningJob)[m_Jobs.columns.state] = Worker::ProcPhaseStr[(int)Worker::GetPhase()];
        (*m_RunningJob)[m_Jobs.columns.progress] = Worker::GetStep();
        (*m_RunningJob)[m_Jobs.columns.percentageProgress] =
            100*Worker::GetStep() / GetJobAt(m_RunningJob).imgSeq.GetActiveImageCount();
        (*m_RunningJob)[m_Jobs.columns.progressText] =
            Glib::ustring::format(Worker::GetStep(), "/", GetJobAt(m_RunningJob).imgSeq.GetActiveImageCount());

        SetStatusBarText((*m_RunningJob)[m_Jobs.columns.jobSource] + ": " +
                        Worker::ProcPhaseStr[(int)Worker::GetPhase()] + ", step: " + (*m_RunningJob)[m_Jobs.columns.progressText]);

        m_LastStepNotify = Worker::GetStep();
    }
    if (!Worker::IsRunning())
    {
        SetStatusBarText("Idle");

        (*m_RunningJob)[m_Jobs.columns.progress] = 0;
        (*m_RunningJob)[m_Jobs.columns.percentageProgress] = 0;
        (*m_RunningJob)[m_Jobs.columns.progressText] = "";
        if (Worker::GetLastResult() == SKRY_SUCCESS ||
            Worker::GetLastResult() == SKRY_LAST_STEP)
        {
            (*m_RunningJob)[m_Jobs.columns.state] = "Processed";
        }
        else
            (*m_RunningJob)[m_Jobs.columns.state] = std::string("Error: ") + SKRY_get_error_message(Worker::GetLastResult());

        Worker::WaitUntilFinished();

        Job_t &job = GetJobAt(m_RunningJob);
        job.imgSeq.Deactivate();
        if ((job.stackedImg = Worker::GetStackedImage())
            && m_RunningJob == m_Jobs.data->get_iter(GetJobsListFocusedRow()))
        {
            UpdateActionsState();
        }
        if (job.outputSaveMode != Utils::Const::OutputSaveMode::NONE && job.stackedImg)
            AutoSaveStack(job);

        job.imgSeq.Deactivate();

        if (m_JobsToProcess.empty())
        {
            m_RunningJob = Gtk::ListStore::iterator(nullptr);
        }
        else
        {
            m_RunningJob = m_Jobs.data->get_iter(m_JobsToProcess.front());
            m_JobsToProcess.pop();
            Job_t &job = GetJobAt(m_RunningJob);
            bool startNextJob = true;
            if (job.anchors.empty())
                if (!SetAnchors(job))
                    startNextJob = false;

            if (startNextJob)
                Worker::StartProcessing(job.imgSeq, job.anchors, job.refPtSpacing,
                                        job.automaticRefPointsPlacement,
                                        job.refPoints,
                                        job.qualityCriterion, job.qualityThreshold,
                                        job.flatFieldFileName);
        }
        UpdateActionsState();
    }
}

c_MainWindow::c_MainWindow()
    : m_LastStepNotify(NONE)
{
    set_title("Stackistry");
    set_border_width(Utils::Const::widgetPaddingInPixels);

    InitActions();
    InitControls();

    Utils::RestorePosSize(Configuration::MainWndPosSize, *this);
    if (Configuration::MainWndMaximized)
        maximize();

    signal_delete_event().connect(sigc::mem_fun(*this, &c_MainWindow::OnDelete));
    Worker::ConnectProgressSignal(sigc::mem_fun(*this, &c_MainWindow::OnWorkerProgress));
}

void c_MainWindow::SetToolbarIcons()
{
    int iconSizeInPx;
    Gtk::IconSize::lookup(Configuration::GetToolIconSize(), iconSizeInPx, iconSizeInPx);

    auto iconImg = Gtk::manage(new Gtk::Image(Utils::LoadIcon("add_images.svg", iconSizeInPx, iconSizeInPx)));
    iconImg->show();
    GetToolButton(ActionName::addImages)->set_icon_widget(*iconImg);

    iconImg = Gtk::manage(new Gtk::Image(Utils::LoadIcon("add_video.svg", iconSizeInPx, iconSizeInPx)));
    iconImg->show();
    GetToolButton(ActionName::addVideos)->set_icon_widget(*iconImg);

    iconImg = Gtk::manage(new Gtk::Image(Utils::LoadIcon("show_vis.svg", iconSizeInPx, iconSizeInPx)));
    iconImg->show();
    GetToolButton(ActionName::toggleVisualization)->set_icon_widget(*iconImg);

    iconImg = Gtk::manage(new Gtk::Image(Utils::LoadIcon("settings.svg", iconSizeInPx, iconSizeInPx)));
    iconImg->show();
    GetToolButton(ActionName::settings)->set_icon_widget(*iconImg);

    iconImg = Gtk::manage(new Gtk::Image(Utils::LoadIcon("stop.svg", iconSizeInPx, iconSizeInPx)));
    iconImg->show();
    GetToolButton(ActionName::stopProcessing)->set_icon_widget(*iconImg);

    iconImg = Gtk::manage(new Gtk::Image(Utils::LoadIcon("flat-field.svg", iconSizeInPx, iconSizeInPx)));
    iconImg->show();
    GetToolButton(ActionName::createFlatField)->set_icon_widget(*iconImg);

    iconImg = Gtk::manage(new Gtk::Image(Utils::LoadIcon("remove.svg", iconSizeInPx, iconSizeInPx)));
    iconImg->show();
    GetToolButton(ActionName::removeJobs)->set_icon_widget(*iconImg);

    iconImg = Gtk::manage(new Gtk::Image(Utils::LoadIcon("start.svg", iconSizeInPx, iconSizeInPx)));
    iconImg->show();
    GetToolButton(ActionName::startProcessing)->set_icon_widget(*iconImg);
}

Gtk::ToolButton *c_MainWindow::GetToolButton(const char *actionName)
{
    return dynamic_cast<Gtk::ToolButton *>(m_UIManager->get_widget(Glib::ustring("/") + WidgetName::Toolbar + "/" + actionName));
}

void c_MainWindow::InitControls()
{
    Gtk::VBox *box = manage(new Gtk::VBox());
    add(*box);

    box->pack_start(*m_UIManager->get_widget(Glib::ustring("/") + WidgetName::MenuBar), Gtk::PACK_SHRINK);

    auto *toolbar = dynamic_cast<Gtk::Toolbar *>(m_UIManager->get_widget(Glib::ustring("/") + WidgetName::Toolbar));
    box->pack_start(*toolbar, Gtk::PACK_SHRINK);

    toolbar->set_icon_size(Configuration::GetToolIconSize());
    SetToolbarIcons();

    CreateJobsListView();
    auto listScrWin = Gtk::manage(new Gtk::ScrolledWindow());
    listScrWin->add(m_Jobs.view);
    listScrWin->show();
    m_MainPaned.add1(*listScrWin);

    m_Visualization.show();
    m_MainPaned.add2(m_Visualization);

#if (GTKMM_MAJOR_VERSION >= 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION >= 16))
    m_MainPaned.set_wide_handle();
#endif

    m_MainPaned.set_position(Configuration::MainWndPanedPos != Configuration::UNDEFINED ?
                             Configuration::MainWndPanedPos : 200);
    m_MainPaned.show();

    m_StatusBar.show();
    SetStatusBarText("Idle");

    box->pack_start(m_MainPaned);
    box->pack_end(m_StatusBar, Gtk::PackOptions::PACK_SHRINK);
    box->show();
}

c_MainWindow::~c_MainWindow()
{
}

bool c_MainWindow::OnDelete(GdkEventAny *event)
{
    Gdk::Rectangle posSize;
    get_position(posSize.gobj()->x, posSize.gobj()->y);
    get_size(posSize.gobj()->width, posSize.gobj()->height);

#if (GTKMM_MAJOR_VERSION >= 4 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION >= 12))
    Configuration::MainWndMaximized = is_maximized();
    if (!is_maximized())
#endif
        Configuration::MainWndPosSize = posSize;

    Utils::SavePosSize(*this, Configuration::MainWndPosSize);

    return false; // Allow normal closing
}

void c_MainWindow::OnPreferences()
{
    c_PreferencesDlg dlg;
    PrepareDialog(dlg);

    int response;
    while ((response = dlg.run()) == Gtk::ResponseType::RESPONSE_OK
           && !dlg.HasCorrectInput())
    {
        Gtk::MessageDialog msg(dlg, "Invalid value.", false, Gtk::MessageType::MESSAGE_ERROR);
        msg.set_title("Error");
        msg.run();
    }

    if (Gtk::ResponseType::RESPONSE_OK == response)
    {
        Configuration::SetToolIconSize(dlg.GetToolIconSize());
        auto *toolbar = dynamic_cast<Gtk::Toolbar *>(m_UIManager->get_widget(Glib::ustring("/") + WidgetName::Toolbar));
        toolbar->set_icon_size(Configuration::GetToolIconSize());
        SetToolbarIcons();
    }
}

void c_MainWindow::Finalize()
{
    Configuration::JobColWidth = m_Jobs.view.get_column(0)->get_width();
    Configuration::StateColWidth = m_Jobs.view.get_column(1)->get_width();
    Configuration::ProgressColWidth = m_Jobs.view.get_column(2)->get_width();

    Configuration::MainWndPanedPos = m_MainPaned.get_position();

    Worker::AbortProcessing(); //TODO: show message? status bar text? while waiting
}

void c_MainWindow::OnSelectionChanged()
{
    UpdateActionsState();
}

void c_MainWindow::OnRemoveJobs()
{
    auto selRows = m_Jobs.view.get_selection()->get_selected_rows();
    for (auto job = selRows.rbegin(); job != selRows.rend(); job++)
    {
        m_Jobs.data->erase(m_Jobs.data->get_iter(*job));
    }
}

void c_MainWindow::OnCreateFlatField()
{
    Gtk::FileChooserDialog dlgOpen(*this, "Choose video file", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_OPEN);
    dlgOpen.add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    dlgOpen.add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);

    auto fltAll = Gtk::FileFilter::create();
    fltAll->add_pattern("*");
    fltAll->set_name("All files");
    dlgOpen.add_filter(fltAll);

    auto fltAvi = Gtk::FileFilter::create();
    fltAvi->add_pattern("*.avi");
    fltAvi->set_name("AVI (*.avi)");
    dlgOpen.add_filter(fltAvi);

    PrepareDialog(dlgOpen);
    if (Gtk::ResponseType::RESPONSE_OK == dlgOpen.run())
    {
        enum SKRY_result result;
        libskry::c_ImageSequence imgSeq = libskry::c_ImageSequence::InitVideoFile(dlgOpen.get_filename().c_str(), &result);
        if (!imgSeq)
        {
            ShowMsg(dlgOpen, "Error", std::string("Error opening ") + dlgOpen.get_filename() + ":\n"
                    + SKRY_get_error_message(result),
                    Gtk::MessageType::MESSAGE_ERROR);
            return;
        }

        libskry::c_Image flatField = imgSeq.CreateFlatField(&result);
        if (!flatField)
        {
            ShowMsg(dlgOpen, "Error", std::string("Failed to create flat-field:\n")
                    + SKRY_get_error_message(result),
                    Gtk::MessageType::MESSAGE_ERROR);
            return;
        }

        Gtk::FileChooserDialog dlgSave("Save flat-field", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SAVE);
        dlgSave.add_button("OK", Gtk::ResponseType::RESPONSE_OK);
        dlgSave.add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
        for (Utils::Vars::OutputFormatDescr_t &outFmt: Utils::Vars::outputFormatDescription)
        {
            auto filter = Gtk::FileFilter::create();
            Glib::ustring filterName = outFmt.name;
            for (auto &pattern: outFmt.patterns)
            {
                filter->add_pattern(pattern);
            }
            filter->set_name(filterName);
            dlgSave.add_filter(filter);
        }

        PrepareDialog(dlgSave);
        if (dlgSave.run() == Gtk::ResponseType::RESPONSE_OK)
        {
            enum SKRY_output_format outpFmt;
            GetOutputFormatFromFilter(dlgSave.get_filter()->get_name(), outpFmt);
            enum SKRY_pixel_format pixFmt = Utils::FindMatchingFormat(outpFmt, 1);
            libskry::c_Image finalImg = libskry::c_Image::ConvertPixelFormat(flatField, pixFmt);
            if (SKRY_SUCCESS != (result = finalImg.Save(dlgSave.get_filename().c_str(), outpFmt)))
            {
                ShowMsg(dlgSave, "Error", std::string("Error saving ") + dlgSave.get_filename() + ":\n"
                        + SKRY_get_error_message(result), Gtk::MessageType::MESSAGE_ERROR);
            }
        }
    }
}

void c_MainWindow::OnAbout()
{
    size_t numLogCpus = 1;
#if defined(_OPENMP)
    numLogCpus = omp_get_num_procs();
#endif

    Gtk::MessageDialog msg(
        *this,

        std::string("<big><big><b>Stackistry</b></big></big>\n\n") +

        "Copyright \u00A9 2016 Filip Szczerek (ga.software@yahoo.com)\n" +
        Glib::ustring::format("version ", VERSION_MAJOR, ".", VERSION_MINOR, ".", VERSION_SUBMINOR) + " (" + VERSION_DATE + ")\n\n"

        "This program comes with ABSOLUTELY NO WARRANTY. This is free software, "
        "licensed under GNU General Public License v3 or any later version. "
        "See the LICENSE file for details.\n\n" +

        Glib::ustring::format("Using ", numLogCpus, " logical CPU(s)."),

        true, Gtk::MessageType::MESSAGE_INFO, Gtk::ButtonsType::BUTTONS_OK, true);

    msg.set_title("About Stackistry");
    auto img = Gtk::manage(new Gtk::Image(Utils::LoadIcon("stackistry-app.svg", 128, 128)));
    img->show();
    msg.set_image(*img);
    msg.run();
}
