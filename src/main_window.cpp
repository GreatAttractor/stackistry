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
    Main window implementation.
*/

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <string>

#include <gdkmm.h>
#include <glibmm/fileutils.h>
#include <glibmm/i18n.h>
#include <glibmm/miscutils.h>
#include <glibmm/threads.h>
#include <glibmm/ustring.h>
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


#define LOCK() Glib::Threads::RecMutex::Lock lock(Worker::GetAccessGuard())

#define VERSION_MAJOR 0
#define VERSION_MINOR 2
#define VERSION_SUBMINOR 0
#define VERSION_DATE "2017-01-07"

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
    const char *saveBestFragmentsImage = "save_best_fragments_image";
    const char *selectFrames = "select_frames";
    const char *preferences = "preferences";
    const char *settings = "settings";
    const char *toggleVisualization = "toggle_visualization";
    const char *removeJobs = "remove_jobs";
    const char *createFlatField = "create_flat-field";
    const char *about = "about";
    const char *toggleQualityWnd = "toggle_quality_wnd";
    const char *exportQualityData = "export_quality_data";
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
    const char *MenuView = "MenuView";
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

Job_t &c_MainWindow::GetJobAt(const Gtk::TreeModel::Path &path)
{
    return GetJobAt(m_Jobs.data->get_iter(path));
}

Job_t &c_MainWindow::GetJobAt(const Gtk::ListStore::iterator &iter)
{
    return *GetJobPtrAt(iter);
}

std::shared_ptr<Job_t> c_MainWindow::GetJobPtrAt(const Gtk::ListStore::iterator &iter)
{
    return (decltype(m_Jobs.columns.job)::ElementType)((*iter)[m_Jobs.columns.job]);
}

void c_MainWindow::OnAddFolders()
{
    //
}

void c_MainWindow::OnAddImageSeries()
{
    Gtk::FileChooserDialog dlg(*this, _("Add image series"), Gtk::FileChooserAction::FILE_CHOOSER_ACTION_OPEN);
    dlg.add_button(_("OK"), Gtk::ResponseType::RESPONSE_OK);
    dlg.add_button(_("Cancel"), Gtk::ResponseType::RESPONSE_CANCEL);
    dlg.set_current_folder(Configuration::LastOpenDir);
    dlg.set_select_multiple();

    auto fltAll = Gtk::FileFilter::create();
    fltAll->add_pattern("*");
    fltAll->set_name(_("All files"));
    dlg.add_filter(fltAll);

    auto fltBmp = Gtk::FileFilter::create();
    fltBmp->add_pattern("*.bmp");
    fltBmp->set_name("BMP (*.bmp)");
    dlg.add_filter(fltBmp);
    auto fltTiff = Gtk::FileFilter::create();
    fltTiff->add_pattern("*.tif");
    fltTiff->add_pattern("*.tiff");
    fltTiff->set_name("TIFF (*.tif, *.tiff)");
    dlg.add_filter(fltTiff);

    PrepareDialog(dlg);
    if (Gtk::ResponseType::RESPONSE_OK == dlg.run())
    {
        std::vector<std::string> fileNames = dlg.get_filenames();
        if (fileNames.size() <= 1)
        {
            ShowMsg(dlg, _("Error"), _("At least two image files are required."), Gtk::MessageType::MESSAGE_ERROR);
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
            row[m_Jobs.columns.state]     = _("Waiting");
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
    if (job.anchors.empty() && !job.automaticAnchorPlacement)
    {
        if (!SetAnchors(job))
        {
            m_RunningJob = Gtk::ListStore::iterator(nullptr);
            return;
        }
    }

    Worker::StartProcessing(&job);
    UpdateActionsState();
    UpdateOutputViewZoomControlsState();
}

bool c_MainWindow::SetAnchorsAutomatically(Job_t &job)
{
    enum SKRY_result result;
    libskry::c_Image firstImg = GetFirstActiveImage(job.imgSeq, result);
    if (!firstImg)
    {
        ShowMsg(*this, _("Error"),
                Glib::ustring::compose(_("Error loading the first image of %1:\n%2"),
                                 job.sourcePath.c_str(), Utils::GetErrorMsg(result)),
                Gtk::MessageType::MESSAGE_ERROR);
        return false;
    }
    job.anchors.push_back(libskry::c_ImageAlignment::SuggestAnchorPos(
        firstImg, Utils::Const::Defaults::placementBrightnessThreshold, Utils::Const::imgAlignmentRefBlockSize));
    return true;
}

void c_MainWindow::OnPauseResumeProcessing()
{
}

void c_MainWindow::OnStopProcessing()
{
    Worker::AbortProcessing();
    if (m_RunningJob)
    {
        (*m_RunningJob)[m_Jobs.columns.progress] = 0;
        (*m_RunningJob)[m_Jobs.columns.percentageProgress] = 0;
        (*m_RunningJob)[m_Jobs.columns.progressText] = "";
        (*m_RunningJob)[m_Jobs.columns.state] = _("Waiting");
        GetJobAt(m_RunningJob).imgSeq.Deactivate();

        m_RunningJob = Gtk::ListStore::iterator(nullptr);
        SetStatusBarText(_("Idle"));
    }
    while (!m_JobsToProcess.empty())
        m_JobsToProcess.pop();

    UpdateActionsState();
    UpdateOutputViewZoomControlsState();
}

Job_t &c_MainWindow::GetCurrentJob()
{
    Gtk::ListStore::iterator itJob = m_Jobs.data->get_iter(GetJobsListFocusedRow());
    return GetJobAt(itJob);
}

std::shared_ptr<Job_t> c_MainWindow::GetCurrentJobPtr()
{
    Gtk::ListStore::iterator itJob = m_Jobs.data->get_iter(GetJobsListFocusedRow());
    return GetJobPtrAt(itJob);
}

bool c_MainWindow::SetAnchors(Job_t &job)
{
    enum SKRY_result result;
    libskry::c_Image firstImg = GetFirstActiveImage(job.imgSeq, result);
    if (!firstImg)
    {
        ShowMsg(*this, _("Error"),
                Glib::ustring::compose(_("Error loading the first image of %1:\n%2"),
                                 job.sourcePath.c_str(), Utils::GetErrorMsg(result)),
                Gtk::MessageType::MESSAGE_ERROR);

        return false;
    }

    c_SelectPointsDlg dlg(firstImg, job.anchors,
                          { libskry::c_ImageAlignment::SuggestAnchorPos(
                              firstImg, Utils::Const::Defaults::placementBrightnessThreshold,
                              Utils::Const::imgAlignmentRefBlockSize) });
    dlg.set_title(_("Set video stabilization anchors"));
    dlg.SetInfoText(_("Automatic placement should work reliably in most cases. "
                    "Otherwise, click the image to place anchors (one anchor is sufficient if there is not much image drift)."));
    PrepareDialog(dlg);
    Utils::RestorePosSize(Configuration::AnchorSelectDlgPosSize, dlg);
    auto response = dlg.run();
    if (response == Gtk::ResponseType::RESPONSE_OK)
    {
        job.automaticAnchorPlacement = false;
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
    SaveImage(GetCurrentJob().stackedImg, _("Save stacked image"), true);
}

void c_MainWindow::OnSaveBestFragmentsImage()
{
    SaveImage(GetCurrentJob().bestFragmentsImg, _("Save best fragments composite"), false);
}

void c_MainWindow::SaveImage(const libskry::c_Image &img, const Glib::ustring &dlgTitle, bool preselectHiBitDephtFilter)
{
    Gtk::FileChooserDialog dlg(dlgTitle, Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SAVE);
    dlg.add_button(_("OK"), Gtk::ResponseType::RESPONSE_OK);
    dlg.add_button(_("Cancel"), Gtk::ResponseType::RESPONSE_CANCEL);
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

        if (preselectHiBitDephtFilter && !hiBitDepthFilterSelected && OUTPUT_FMT_BITS_PER_CHANNEL[outFmt.skryOutpFmt] >= 16)
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
        enum SKRY_pixel_format pixFmt = Utils::FindMatchingFormat(outpFmt, NUM_CHANNELS[img.GetPixelFormat()]);
        libskry::c_Image finalImg = libskry::c_Image::ConvertPixelFormat(img, pixFmt);
        enum SKRY_result result;
        if (SKRY_SUCCESS != (result = finalImg.Save(dlg.get_filename().c_str(), outpFmt)))
        {
            std::cout << "Failed to save image" << std::endl;

            ShowMsg(*this, _("Error"),
                    Glib::ustring::compose(_("Error saving output image %1:\n%2"),
                                     dlg.get_filename().c_str(),
                                     Utils::GetErrorMsg(result)),
                    Gtk::MessageType::MESSAGE_ERROR);
        }
    }
}

void c_MainWindow::OnSettings()
{
    std::vector<std::string> selJobNames;
    for (auto &selJob: m_Jobs.view.get_selection()->get_selected_rows())
        selJobNames.push_back(GetJobAt(selJob).sourcePath);

    // Fill the dialog using settings of the first selected job
    c_SettingsDlg dlg(selJobNames, GetJobAt(m_Jobs.view.get_selection()->get_selected_rows()[0]));

    PrepareDialog(dlg);
    if (Gtk::ResponseType::RESPONSE_OK == dlg.run())
        for (auto &selJob: m_Jobs.view.get_selection()->get_selected_rows())
            dlg.ApplySettings(GetJobAt(selJob));
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

    bool oneJobSelected = (numSelJobs == 1 && GetJobsListFocusedRow());

    { LOCK();
        m_ActionGroup->get_action(ActionName::saveStackedImage)->set_sensitive(
            oneJobSelected && GetCurrentJob().stackedImg);

        m_ActionGroup->get_action(ActionName::saveBestFragmentsImage)->set_sensitive(
            oneJobSelected && GetCurrentJob().bestFragmentsImg);

        m_ActionGroup->get_action(ActionName::exportQualityData)->set_sensitive(
            oneJobSelected && !GetCurrentJob().quality.framesChrono.empty()
        );
    }
}

void c_MainWindow::OnSelectFrames()
{
    c_FrameSelectDlg dlg(GetCurrentJob().imgSeq);
    PrepareDialog(dlg);
    do
    {
        if (dlg.run() == Gtk::ResponseType::RESPONSE_OK)
        {
            std::vector<uint8_t> activeFlags = dlg.GetActiveFlags();
            if (std::count(activeFlags.begin(), activeFlags.end(), 1) < 2)
            {
                ShowMsg(*this, _("Error"), _("At least two frames have to be active."), Gtk::MessageType::MESSAGE_ERROR);
            }
            else
            {
                Job_t &job = GetCurrentJob();
                job.imgSeq.SetActiveImages(activeFlags.data());
                // Modified frame selection may modify video size and target framing
                // after image alignment, so old ref. points may be invalid
                job.refPoints.clear();

                job.quality.framesChrono.clear();
                job.quality.framesSorted.clear();
                m_QualityWnd.Update();

                break;
            }
        }
        else
            break;

    } while (true);
    GetCurrentJob().imgSeq.Deactivate();
}

void c_MainWindow::SetDefaultSettings(Job_t &job)
{
    assert(job.imgSeq);

    job.outputSaveMode = Utils::Const::Defaults::saveMode;
    job.outputFmt = Utils::Const::Defaults::outputFmt;
    job.alignmentMethod = Utils::Const::Defaults::alignmentMethod;
    job.refPtAutoPlacementParams.spacing = Utils::Const::Defaults::referencePointSpacing;
    job.refPtAutoPlacementParams.brightnessThreshold = Utils::Const::Defaults::placementBrightnessThreshold;
    job.refPtAutoPlacementParams.structureScale = Utils::Const::Defaults::refPtStructureScale;
    job.refPtAutoPlacementParams.structureThreshold = Utils::Const::Defaults::refPtStructureThreshold;
    job.quality.criterion = Utils::Const::Defaults::qualityCriterion;
    job.quality.threshold = Utils::Const::Defaults::qualityThreshold;
    job.automaticRefPointsPlacement = true;
    job.automaticAnchorPlacement = true;
    job.cfaPattern = SKRY_CFA_NONE;
    job.refPtBlockSize = Utils::Const::Defaults::refPtRefBlockSize;
    job.refPtSearchRadius = Utils::Const::Defaults::refPtSearchRadius;
    job.exportQualityData = false;
    job.qualityDataReadyNotification = false;
}

void c_MainWindow::OnAddVideos()
{
    Gtk::FileChooserDialog dlg(*this, _("Add video(s)"), Gtk::FileChooserAction::FILE_CHOOSER_ACTION_OPEN);
    dlg.add_button(_("OK"), Gtk::ResponseType::RESPONSE_OK);
    dlg.add_button(_("Cancel"), Gtk::ResponseType::RESPONSE_CANCEL);
    dlg.set_current_folder(Configuration::LastOpenDir);
    dlg.set_select_multiple();

    auto fltAll = Gtk::FileFilter::create();
    fltAll->add_pattern("*");
    fltAll->set_name(_("All files"));
    dlg.add_filter(fltAll);

    auto fltAvi = Gtk::FileFilter::create();
    fltAvi->add_pattern("*.avi");
    fltAvi->set_name("AVI (*.avi)");
    dlg.add_filter(fltAvi);

    auto fltSer = Gtk::FileFilter::create();
    fltSer->add_pattern("*.ser");
    fltSer->set_name("SER (*.ser)");
    dlg.add_filter(fltSer);

    dlg.set_filter(fltAvi);

    PrepareDialog(dlg);
    if (Gtk::ResponseType::RESPONSE_OK == dlg.run())
    {
        for (auto &fname: dlg.get_filenames())
        {
            enum SKRY_result result;
            std::shared_ptr<Job_t> newJob = std::make_shared<Job_t>(Job_t { libskry::c_ImageSequence::InitVideoFile(fname.c_str(), &result) });
            if (!newJob->imgSeq)
            {
                ShowMsg(dlg, _("Error"),
                        Glib::ustring::compose(_("Could not open video file %1:\n%2"),
                                         fname.c_str(), Utils::GetErrorMsg(result)),
                        Gtk::MessageType::MESSAGE_ERROR);
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
                row[m_Jobs.columns.state]     = _("Waiting");
                row[m_Jobs.columns.progressText] = "";
                row[m_Jobs.columns.job]       = newJob;
            }
        }
    }
    Configuration::LastOpenDir = dlg.get_current_folder();
}

void c_MainWindow::OnQuit()
{
    close();
}

void c_MainWindow::InitActions()
{
    m_ActionGroup = Gtk::ActionGroup::create();
    m_ActionGroup->add(Gtk::Action::create(WidgetName::MenuFile, _("_File")));
    m_ActionGroup->add(Gtk::Action::create(ActionName::quit, Gtk::Stock::QUIT),
                  sigc::mem_fun(*this, &c_MainWindow::OnQuit));
    m_ActionGroup->add(Gtk::Action::create(ActionName::addImages, Glib::ustring(_("Add image series...")), Glib::ustring(_("Add image series..."))),
                  sigc::mem_fun(*this, &c_MainWindow::OnAddImageSeries));
    m_ActionGroup->add(Gtk::Action::create(ActionName::addFolders, _("Add image series from folder(s)...")),
                  sigc::mem_fun(*this, &c_MainWindow::OnAddFolders));
    m_ActionGroup->add(Gtk::Action::create(ActionName::addVideos, _("Add video(s)..."), _("Add video(s)...")),
                  sigc::mem_fun(*this, &c_MainWindow::OnAddVideos));
    m_ActionGroup->add(Gtk::Action::create(ActionName::saveStackedImage, _("Save stacked image...")),
                  sigc::mem_fun(*this, &c_MainWindow::OnSaveStackedImage));
    m_ActionGroup->add(Gtk::Action::create(ActionName::saveBestFragmentsImage, _("Save best fragments composite...")),
                  sigc::mem_fun(*this, &c_MainWindow::OnSaveBestFragmentsImage));
    m_ActionGroup->add(Gtk::Action::create(ActionName::createFlatField, _("Create flat-field from video..."),
                                           _("Create flat-field from video...")),
                  sigc::mem_fun(*this, &c_MainWindow::OnCreateFlatField));

    m_ActionGroup->add(Gtk::Action::create(WidgetName::MenuEdit, _("_Edit")));
    m_ActionGroup->add(Gtk::Action::create(ActionName::setAnchors, _("Set video stabilization anchors...")),
                  sigc::mem_fun(*this, &c_MainWindow::OnSetAnchors));
    m_ActionGroup->add(Gtk::Action::create(ActionName::selectFrames, _("Select frames...")),
                  sigc::mem_fun(*this, &c_MainWindow::OnSelectFrames));
    m_ActionGroup->add(Gtk::Action::create(ActionName::settings, _("Processing settings..."),
                                           _("Processing settings...")),
                  sigc::mem_fun(*this, &c_MainWindow::OnSettings));
    m_ActionGroup->add(Gtk::Action::create(ActionName::removeJobs, _("Remove from list"), _("Remove from list")),
                  sigc::mem_fun(*this, &c_MainWindow::OnRemoveJobs));
    m_ActionGroup->add(Gtk::Action::create(ActionName::preferences, _("Preferences...")),
                  sigc::mem_fun(*this, &c_MainWindow::OnPreferences));
    m_ActionGroup->add(Gtk::Action::create(ActionName::exportQualityData, _("Export frame quality data...")),
                  sigc::mem_fun(*this, &c_MainWindow::OnExportQualityData));

    m_ActionGroup->add(Gtk::Action::create(WidgetName::MenuProcessing, _("_Processing")));
    m_ActionGroup->add(Gtk::Action::create(ActionName::startProcessing, _("Start processing"),
                                           _("Start processing selected job(s)")),
                  sigc::mem_fun(*this, &c_MainWindow::OnStartProcessing));
    m_ActionGroup->add(Gtk::Action::create(ActionName::pauseResumeProcessing, _("Pause processing")),
                  sigc::mem_fun(*this, &c_MainWindow::OnPauseResumeProcessing));
    m_ActionGroup->add(Gtk::Action::create(ActionName::stopProcessing, _("Stop processing"), _("Stop processing")),
                  sigc::mem_fun(*this, &c_MainWindow::OnStopProcessing));
    m_ActVisualization = Gtk::ToggleAction::create(ActionName::toggleVisualization, _("Show visualization"), _("Show visualization (slows down processing)"));
    m_ActionGroup->add(m_ActVisualization, sigc::mem_fun(*this, &c_MainWindow::OnToggleVisualization));

    m_ActionGroup->add(Gtk::Action::create(WidgetName::MenuView, _("_View")));

    m_ActQualityWnd = Gtk::ToggleAction::create(ActionName::toggleQualityWnd, _("Frame quality graph"), _("Show frame quality graph"));
    m_ActionGroup->add(m_ActQualityWnd, sigc::mem_fun(*this, &c_MainWindow::OnToggleQualityWnd));

    m_ActionGroup->add(Gtk::Action::create(WidgetName::MenuAbout, _("_About")));
    m_ActionGroup->add(Gtk::Action::create(ActionName::about, _("About Stackistry...")),
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
    "            <menuitem action='" + ActionName::saveBestFragmentsImage + "' />"
    "            <menuitem action='" + ActionName::exportQualityData + "' />"
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
    "            <menuitem action='" + ActionName::stopProcessing + "' />"
    "            <separator />"
    "            <menuitem action='" + ActionName::toggleVisualization + "' />"
    "        </menu>"

    "        <menu action='" + WidgetName::MenuView + "'>"
    "            <menuitem action='" + ActionName::toggleQualityWnd + "' />"
    "        </menu>"

    "        <menu action='" + WidgetName::MenuAbout + "'>"
    "            <menuitem action='" + ActionName::about + "' />"
    "        </menu>"

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
    "       <toolitem action='" + ActionName::toggleQualityWnd + "'/>"
    "    </toolbar>"

    "    <popup name='" + WidgetName::JobsCtxMenu + "'>"
    "        <menuitem action='" + ActionName::selectFrames + "' />"
    "        <menuitem action='" + ActionName::settings + "' />"
    "        <menuitem action='" + ActionName::setAnchors + "' />"
    "        <menuitem action='" + ActionName::saveStackedImage + "' />"
    "        <menuitem action='" + ActionName::saveBestFragmentsImage + "' />"
    "        <menuitem action='" + ActionName::exportQualityData + "' />"
    "        <separator/>"
    "        <menuitem action='" + ActionName::removeJobs + "'/>"
    "    </popup>"
    "</ui>";

    m_UIManager->add_ui_from_string(uiInfo);
    UpdateActionsState();
}

void c_MainWindow::OnToggleQualityWnd()
{
    m_QualityWnd.set_visible(m_ActQualityWnd->get_active());
}

void c_MainWindow::OnJobCursorChanged()
{
    UpdateActionsState();


    if (GetJobsListFocusedRow())
    {
        m_QualityWnd.SetJob(GetCurrentJobPtr());

        { LOCK();

            const Job_t &job = GetCurrentJob();
            switch(m_OutputView.GetOutputImgType())
            {
            case OutputImgType::Stack:
                m_OutputView.SetImage(job.stackedImg);
                break;

            case OutputImgType::BestFragments:
                m_OutputView.SetImage(job.bestFragmentsImg);
                break;
            }
        }
    }
    else
        m_QualityWnd.SetJob(nullptr);
}

void c_MainWindow::CreateJobsListView()
{
    m_Jobs.data = Gtk::ListStore::create(m_Jobs.columns);
    m_Jobs.view.set_model(m_Jobs.data);

    m_Jobs.view.append_column(_("Job"),  m_Jobs.columns.jobSource);
    m_Jobs.view.get_column(0)->set_resizable();
    auto crend = dynamic_cast<Gtk::CellRendererText *>(m_Jobs.view.get_column_cell_renderer(0));
    crend->property_ellipsize() = Pango::EllipsizeMode::ELLIPSIZE_START;

    m_Jobs.view.append_column(_("State"), m_Jobs.columns.state);
    m_Jobs.view.get_column(1)->set_resizable();

    auto *crProgress = Gtk::manage(new Gtk::CellRendererProgress());
    m_Jobs.view.append_column(_("Progress"), *crProgress);
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
    UpdateOutputViewZoomControlsState();

    if (m_ActVisualization->get_active())
        m_OutputView.SetOutputImgType(OutputImgType::Visualization);
}

void c_MainWindow::UpdateOutputViewZoomControlsState()
{
    // The user cannot change the zoom factor if visualization is disabled,
    // because the scaled image is provided (on every processing step) by the
    // worker thread, not generated by the 'm_Visualization' widget.
    m_OutputView.SetZoomControlsEnabled(
            m_OutputView.GetOutputImgType() != OutputImgType::Visualization
            ||
            Worker::IsRunning() && m_ActVisualization->get_active());
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

    std::string destDir = GetDestDir(job);

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

std::string c_MainWindow::GetDestDir(const Job_t &job) const
{
    if (job.outputSaveMode == Utils::Const::OutputSaveMode::SOURCE_PATH)
    {
        return (job.imgSeq.GetType() == SKRY_IMG_SEQ_IMAGE_FILES
                ? job.sourcePath
                : Glib::path_get_dirname(job.sourcePath));
    }
    else
        return job.destDir;
}

void c_MainWindow::OnWorkerProgress()
{
    LOCK();

    if (!m_RunningJob)
        return; // an outdated notification, ignore

    // As of GTK 3.22 on Linux, something got broken in Glib::Dispatcher() - calling Gtk::Dialog::run() (to have the user
    // set the reference points manually) from OnWorkerProgress() causes a recursive entry into OnWorkerProgress() from
    // the new dialog's main loop.
    // Prevent this via an additional bool flag:
    if (m_HandlingManualRefPoints)
        return;

    // Update "Save stacked image", "Save best fragments composite image", "Export quality data"
    // actions' state
    UpdateActionsState();

    if (m_RunningJob)
    {
        Job_t &job = GetJobAt(m_RunningJob);
        if (job.qualityDataReadyNotification)
        {
            job.qualityDataReadyNotification = false;

            m_QualityWnd.Update();
            UpdateActionsState();

            if (job.exportQualityData)
            {
                std::string destDir = GetDestDir(job);
                std::string destFName = "frame_quality";

                // For video files, prepend with the source file name
                if (job.imgSeq.GetType() != SKRY_IMG_SEQ_IMAGE_FILES)
                    destFName = Glib::path_get_basename(job.sourcePath) + "_" + destFName;

                std::string destPath = Glib::build_filename(destDir, destFName + ".txt");

                ExportQualityData(destPath, job);
            }
        }
    }


    if (GetJobsListFocusedRow())
    {
        const Job_t &job = GetCurrentJob();

        switch (m_OutputView.GetOutputImgType())
        {
        case OutputImgType::Stack:
            m_OutputView.SetImage(job.stackedImg);
            break;

        case OutputImgType::BestFragments:
            m_OutputView.SetImage(job.bestFragmentsImg);
            break;
        }
    }

    if (Worker::IsWaitingForReferencePoints())
    {
        m_HandlingManualRefPoints = true;
        struct Job_t &job = GetJobAt(*m_RunningJob);
        c_SelectPointsDlg dlg(Worker::GetBestQualityAlignedImage(), job.refPoints, { });
        dlg.set_title(_("Set reference points"));
        dlg.SetInfoText(_("Place reference points by left-clicking on the image. Avoid blank areas "
                        "with little or no detail. Click Cancel to set points automatically."));
        PrepareDialog(dlg);
        Utils::RestorePosSize(Configuration::SelectRefPointsDlgPosSize, dlg);
        auto response = dlg.run();
        if (response == Gtk::ResponseType::RESPONSE_OK)
            dlg.GetPoints(job.refPoints);
        else
            job.automaticRefPointsPlacement = true;

        Worker::NotifyReferencePointsSet();
        Utils::SavePosSize(dlg, Configuration::SelectRefPointsDlgPosSize);
        m_HandlingManualRefPoints = false;
    }

    if (Worker::GetStep() != m_LastStepNotify)
    {
        if (Worker::IsVisualizationEnabled() && m_OutputView.GetOutputImgType() == OutputImgType::Visualization)
            if (Worker::GetVisualizationImage())
                m_OutputView.SetImage(Worker::GetVisualizationImage());

        (*m_RunningJob)[m_Jobs.columns.state] = Worker::GetProcPhaseStr(Worker::GetPhase());
        (*m_RunningJob)[m_Jobs.columns.progress] = Worker::GetStep();
        (*m_RunningJob)[m_Jobs.columns.percentageProgress] =
            100*Worker::GetStep() / GetJobAt(m_RunningJob).imgSeq.GetActiveImageCount();
        (*m_RunningJob)[m_Jobs.columns.progressText] =
            Glib::ustring::format(Worker::GetStep(), "/", GetJobAt(m_RunningJob).imgSeq.GetActiveImageCount());

        SetStatusBarText((*m_RunningJob)[m_Jobs.columns.jobSource] + " \u2013 " +        // /u2013 = N-dash
                        Worker::GetProcPhaseStr(Worker::GetPhase()) + ", " + _("step") +
                         " " + (*m_RunningJob)[m_Jobs.columns.progressText]);

        m_LastStepNotify = Worker::GetStep();
    }
    if (!Worker::IsRunning())
    {
        SetStatusBarText(_("Idle"));

        (*m_RunningJob)[m_Jobs.columns.progress] = 0;
        (*m_RunningJob)[m_Jobs.columns.percentageProgress] = 0;
        (*m_RunningJob)[m_Jobs.columns.progressText] = "";
        if (Worker::GetLastResult() == SKRY_SUCCESS ||
            Worker::GetLastResult() == SKRY_LAST_STEP)
        {
            (*m_RunningJob)[m_Jobs.columns.state] = _("Processed");
        }
        else
            (*m_RunningJob)[m_Jobs.columns.state] = Glib::ustring::compose(_("Error: %1"), Utils::GetErrorMsg(Worker::GetLastResult()));

        Worker::WaitUntilFinished();

        Job_t &job = GetJobAt(m_RunningJob);
        job.imgSeq.Deactivate();

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
            if (job.anchors.empty() && !job.automaticAnchorPlacement)
                if (!SetAnchors(job))
                    startNextJob = false;

            if (startNextJob)
                Worker::StartProcessing(&job);
        }

        UpdateActionsState();
        UpdateOutputViewZoomControlsState();
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

    auto iconImg = Gtk::manage(new Gtk::Image(Utils::LoadIconFromFile("add_images.svg", iconSizeInPx, iconSizeInPx)));
    iconImg->show();
    GetToolButton(ActionName::addImages)->set_icon_widget(*iconImg);

    iconImg = Gtk::manage(new Gtk::Image(Utils::LoadIconFromFile("add_video.svg", iconSizeInPx, iconSizeInPx)));
    iconImg->show();
    GetToolButton(ActionName::addVideos)->set_icon_widget(*iconImg);

    iconImg = Gtk::manage(new Gtk::Image(Utils::LoadIconFromFile("show_vis.svg", iconSizeInPx, iconSizeInPx)));
    iconImg->show();
    GetToolButton(ActionName::toggleVisualization)->set_icon_widget(*iconImg);

    iconImg = Gtk::manage(new Gtk::Image(Utils::LoadIconFromFile("settings.svg", iconSizeInPx, iconSizeInPx)));
    iconImg->show();
    GetToolButton(ActionName::settings)->set_icon_widget(*iconImg);

    iconImg = Gtk::manage(new Gtk::Image(Utils::LoadIconFromFile("stop.svg", iconSizeInPx, iconSizeInPx)));
    iconImg->show();
    GetToolButton(ActionName::stopProcessing)->set_icon_widget(*iconImg);

    iconImg = Gtk::manage(new Gtk::Image(Utils::LoadIconFromFile("flat-field.svg", iconSizeInPx, iconSizeInPx)));
    iconImg->show();
    GetToolButton(ActionName::createFlatField)->set_icon_widget(*iconImg);

    iconImg = Gtk::manage(new Gtk::Image(Utils::LoadIconFromFile("remove.svg", iconSizeInPx, iconSizeInPx)));
    iconImg->show();
    GetToolButton(ActionName::removeJobs)->set_icon_widget(*iconImg);

    iconImg = Gtk::manage(new Gtk::Image(Utils::LoadIconFromFile("start.svg", iconSizeInPx, iconSizeInPx)));
    iconImg->show();
    GetToolButton(ActionName::startProcessing)->set_icon_widget(*iconImg);

    iconImg = Gtk::manage(new Gtk::Image(Utils::LoadIconFromFile("qual_graph.svg", iconSizeInPx, iconSizeInPx)));
    iconImg->show();
    GetToolButton(ActionName::toggleQualityWnd)->set_icon_widget(*iconImg);
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

    m_OutputView.SetApplyZoom(false);
    m_OutputView.signal_ZoomChanged().connect(sigc::slot<void, int>(
        [this](int zoomPercentVal)
            {
                if (Worker::IsRunning() && m_OutputView.GetOutputImgType() == OutputImgType::Visualization)
                    Worker::SetZoomFactor(zoomPercentVal / 100.0, m_OutputView.GetInterpolationMethod());
            }
    ));
    m_OutputView.signal_OutputImgTypeChanged().connect(sigc::mem_fun(*this, &c_MainWindow::OnOutputImgTypeChanged));

    UpdateOutputViewZoomControlsState();
    m_OutputView.show();
    m_MainPaned.add2(m_OutputView);

#if (GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION >= 16))
    m_MainPaned.set_wide_handle();
#endif

    m_MainPaned.set_position(Configuration::MainWndPanedPos != Configuration::UNDEFINED ?
                             Configuration::MainWndPanedPos : 200);
    m_MainPaned.show();

    m_StatusBar.show();
    SetStatusBarText(_("Idle"));

    box->pack_start(m_MainPaned);
    box->pack_end(m_StatusBar, Gtk::PackOptions::PACK_SHRINK);
    box->show();

    m_QualityWnd.signal_hide().connect([this]() { m_ActQualityWnd->set_active(false); });
    m_QualityWnd.signal_Export().connect(sigc::mem_fun(*this, &c_MainWindow::OnExportQualityData));
    m_QualityWnd.set_transient_for(*this);
}

c_MainWindow::~c_MainWindow()
{
}

bool c_MainWindow::OnDelete(GdkEventAny *event)
{
    Gdk::Rectangle posSize;
    get_position(posSize.gobj()->x, posSize.gobj()->y);
    get_size(posSize.gobj()->width, posSize.gobj()->height);

#if (GTKMM_MAJOR_VERSION > 3 || (GTKMM_MAJOR_VERSION == 3 && GTKMM_MINOR_VERSION >= 12))
    Configuration::MainWndMaximized = is_maximized();
    if (!is_maximized())
#endif
    {
        Configuration::MainWndPosSize = posSize;
    }

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
        Gtk::MessageDialog msg(dlg, _("Invalid value."), false, Gtk::MessageType::MESSAGE_ERROR);
        msg.set_title(_("Error"));
        msg.run();
    }

    if (Gtk::ResponseType::RESPONSE_OK == response)
    {
        Configuration::SetToolIconSize(dlg.GetToolIconSize());
        auto *toolbar = dynamic_cast<Gtk::Toolbar *>(m_UIManager->get_widget(Glib::ustring("/") + WidgetName::Toolbar));
        toolbar->set_icon_size(Configuration::GetToolIconSize());
        SetToolbarIcons();

        if (dlg.GetUILanguage() != std::string(Configuration::UILanguage))
        {
            ShowMsg(*this, _("Information"), _("You have to restart Stackistry for the changes to take effect."),
                    Gtk::MessageType::MESSAGE_INFO);
            Configuration::UILanguage = dlg.GetUILanguage();
        }

        // The number of histogram bins might have changed
        m_QualityWnd.Update();
    }
}

void c_MainWindow::Finalize()
{
    Configuration::JobColWidth = m_Jobs.view.get_column(0)->get_width();
    Configuration::StateColWidth = m_Jobs.view.get_column(1)->get_width();
    Configuration::ProgressColWidth = m_Jobs.view.get_column(2)->get_width();

    Configuration::MainWndPanedPos = m_MainPaned.get_position();
    Utils::SavePosSize(m_QualityWnd, Configuration::QualityWndPosSize);

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
    Gtk::FileChooserDialog dlgOpen(*this, _("Choose video file"), Gtk::FileChooserAction::FILE_CHOOSER_ACTION_OPEN);
    dlgOpen.add_button(_("OK"), Gtk::ResponseType::RESPONSE_OK);
    dlgOpen.add_button(_("Cancel"), Gtk::ResponseType::RESPONSE_CANCEL);

    auto fltAll = Gtk::FileFilter::create();
    fltAll->add_pattern("*");
    fltAll->set_name(_("All files"));
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
            ShowMsg(dlgOpen, _("Error"),
                    Glib::ustring::compose(_("Error opening %1:\n%2"), dlgOpen.get_filename().c_str(),
                                     Utils::GetErrorMsg(result)),
                    Gtk::MessageType::MESSAGE_ERROR);
            return;
        }

        libskry::c_Image flatField = imgSeq.CreateFlatField(&result);
        if (!flatField)
        {
            ShowMsg(dlgOpen, _("Error"),
                    Glib::ustring::compose(_("Failed to create flat-field:\n%1"),
                                     Utils::GetErrorMsg(result)),
                    Gtk::MessageType::MESSAGE_ERROR);
            return;
        }

        Gtk::FileChooserDialog dlgSave(_("Save flat-field"), Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SAVE);
        dlgSave.add_button(_("OK"), Gtk::ResponseType::RESPONSE_OK);
        dlgSave.add_button(_("Cancel"), Gtk::ResponseType::RESPONSE_CANCEL);
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
                ShowMsg(dlgSave, _("Error"),
                        Glib::ustring::compose(_("Error saving %1:\n%2"),
                                         dlgSave.get_filename().c_str(),
                                         Utils::GetErrorMsg(result)),
                        Gtk::MessageType::MESSAGE_ERROR);
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

        "Copyright \u00A9 2016, 2017 Filip Szczerek (ga.software@yahoo.com)\n" +
        Glib::ustring::compose(_("version %1.%2.%3 (%4)\n\n"), VERSION_MAJOR, VERSION_MINOR, VERSION_SUBMINOR, VERSION_DATE) +

        _("This program comes with ABSOLUTELY NO WARRANTY. This is free software, "
        "licensed under GNU General Public License v3 or any later version. "
        "See the LICENSE file for details.") + "\n\n" +

        Glib::ustring::compose(_("Built with <a href='https://github.com/GreatAttractor/libskry'>libskry</a> "

        "version %1.%2.%3 (%4).\n"), LIBSKRY_MAJOR_VERSION, LIBSKRY_MINOR_VERSION, LIBSKRY_SUBMINOR_VERSION, LIBSKRY_RELEASE_DATE) +

        Glib::ustring::compose(_("Using %1 logical CPU(s).\n"), numLogCpus),

        true, Gtk::MessageType::MESSAGE_INFO, Gtk::ButtonsType::BUTTONS_OK, true);

    msg.set_title(_("About Stackistry"));
    auto img = Gtk::manage(new Gtk::Image(Utils::LoadIconFromFile("stackistry-app.svg", 128, 128)));
    img->show();
    msg.set_image(*img);
    msg.run();
}

/// Returns 'false' on failure
bool c_MainWindow::ExportQualityData(const std::string &fileName, const Job_t &job) const
{
    bool success = false;

    std::ofstream file(fileName.c_str());
    if (!file.fail())
    {
        assert(!job.quality.framesChrono.empty());

        file << "Stackistry " << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_SUBMINOR << "\n"
             << "Normalized frame quality of \"" << job.sourcePath << "\"\n\n"
             << "Frame;Active frame;Quality\n";

        auto minmaxQuality = std::minmax_element(job.quality.framesChrono.begin(),
                                                 job.quality.framesChrono.end());

        double range = *minmaxQuality.second - *minmaxQuality.first;

        const uint8_t *imgIsActive = job.imgSeq.GetImgActiveFlags();
        bool exportInactive = Configuration::ExportInactiveFramesQuality;

        size_t activeImgIdx = 0;
        for (size_t i = 0; i < job.imgSeq.GetImageCount(); i++)
        {
            if (imgIsActive[i] || exportInactive)
            {
                file << i << ";";

                if (imgIsActive[i])
                {
                    file << activeImgIdx << ";" << (job.quality.framesChrono[activeImgIdx] - *minmaxQuality.first) / range << "\n";
                    activeImgIdx++;
                }
                else if (exportInactive)
                    file << "-1;0\n";
            }
        }

        success = !file.fail();
    }

    return success;
}

void c_MainWindow::OnExportQualityData()
{
    Gtk::FileChooserDialog dlg(_("Export frame quality data"), Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SAVE);
    dlg.add_button(_("OK"), Gtk::ResponseType::RESPONSE_OK);
    dlg.add_button(_("Cancel"), Gtk::ResponseType::RESPONSE_CANCEL);

    auto fltAll = Gtk::FileFilter::create();
    fltAll->add_pattern("*");
    fltAll->set_name(_("All files"));
    dlg.add_filter(fltAll);

    auto fltTxt = Gtk::FileFilter::create();
    fltTxt->add_pattern("*.txt");
    fltTxt->set_name(_("Text (*.txt)"));
    dlg.add_filter(fltTxt);
    dlg.set_filter(fltTxt);

    PrepareDialog(dlg);
    if (dlg.run() == Gtk::ResponseType::RESPONSE_OK)
    {
        Glib::ustring errorMsg = Glib::ustring::compose(_("Failed to export the quality data as %1."),
                                                        dlg.get_filename().c_str());

        if (!ExportQualityData(dlg.get_filename(), GetCurrentJob()))
        {
            ShowMsg(*this, _("Error"),
                    Glib::ustring::compose(errorMsg, dlg.get_filename()),
                    Gtk::MessageType::MESSAGE_ERROR);
        }
    }
}

void c_MainWindow::OnOutputImgTypeChanged()
{
    UpdateOutputViewZoomControlsState();

    switch (m_OutputView.GetOutputImgType())
    {
    case OutputImgType::Visualization:
        {
            m_OutputView.SetImage(Worker::GetVisualizationImage(), false);
            auto prevZoom = Worker::GetZoomFactor();
            m_OutputView.SetZoom(std::get<0>(prevZoom), std::get<1>(prevZoom));
        }
        break;

    case OutputImgType::Stack:
        if (GetJobsListFocusedRow())
            m_OutputView.SetImage(GetCurrentJob().stackedImg);
        else
            m_OutputView.RemoveImage();
        break;

    case OutputImgType::BestFragments:
        if (GetJobsListFocusedRow())
            m_OutputView.SetImage(GetCurrentJob().bestFragmentsImg);
        else
            m_OutputView.RemoveImage();
        break;
    }
}
