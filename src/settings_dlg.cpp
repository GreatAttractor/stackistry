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
    Processing settings dialog implementation.
*/

#include <vector>
#include <string>

#include <glibmm/i18n.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/separator.h>
#include <gtkmm/textview.h>

#include "config.h"
#include "settings_dlg.h"
#include "utils.h"

static
std::vector<std::string> GetStackingCriteriaStrings()
{
    return
    {
        _("percent of the best fragments"),      // SKRY_PERCENTAGE_BEST
        _("percent or better relative quality"), // SKRY_MIN_REL_QUALITY
        _("best fragments"),                     // SKRY_NUMBER_BEST
    };
}

static
std::vector<std::string> GetOutputSaveModeStrings()
{
    return
    {
        _("disabled"),                   // OutputSaveMode::NONE
        _("in source video's folder"),   // OutputSaveMode::SOURCE_PATH
        _("specify folder")              // OutputSaveMode::SPECIFIED_PATH
    };
}

static
std::vector<std::string> GetAlignmentMethodStrings()
{
    return
    {
        _("anchors"),                  // SKRY_IMG_ALGN_ANCHORS
        _("image intensity centroid"), // SKRY_IMG_ALGN_CENTROID
    };
}

c_SettingsDlg::c_SettingsDlg(const std::vector<std::string> &jobNames, const Job_t &firstJob)
{
    set_title(_("Processing settings"));
    InitControls(jobNames);
    Utils::RestorePosSize(Configuration::SettingsDlgPosSize,  *this);

    // Set controls' values according to 'firstJob'

    m_DestFolderChooser.set_filename(firstJob.destDir);
    m_OutputSaveMode.set_active(firstJob.outputSaveMode);
    m_RefPtSpacing.set_value(firstJob.refPtAutoPlacementParams.spacing);
    m_RefPtBrightThresh.set_value(firstJob.refPtAutoPlacementParams.brightnessThreshold * 100);
    m_FlatFieldCheckBtn.set_active(!firstJob.flatFieldFileName.empty());
    if (!firstJob.flatFieldFileName.empty())
        m_FlatFieldChooser.set_filename(firstJob.flatFieldFileName);

    m_AlignmentMethod.set_active((int)firstJob.alignmentMethod);
    m_VideoStbAnchorsMode.set_active(firstJob.automaticAnchorPlacement ? 0 : 1);
    m_QualityCriterion.set_active((int)firstJob.qualityCriterion);
    m_QualityThreshold.set_value(firstJob.qualityThreshold);

    size_t index = 0;
    for (auto &descr: Utils::Vars::outputFormatDescription)
    {
        if (descr.skryOutpFmt == firstJob.outputFmt)
            m_AutoSaveOutputFormat.set_active(index);
        else
            index++;
    }

    m_RefPtPlacementMode.set_active(firstJob.automaticRefPointsPlacement ? 0 : 1);

    m_TreatMonoAsCFA.set_active(firstJob.cfaPattern != SKRY_CFA_NONE);
    if (firstJob.cfaPattern != SKRY_CFA_NONE)
        m_CFAPattern.set_active((unsigned)firstJob.cfaPattern);

    m_RefPtRefBlockSize.set_value(firstJob.refPtBlockSize);
    m_RefPtSearchRadius.set_value(firstJob.refPtSearchRadius);
    m_StructureScale.set_value(firstJob.refPtAutoPlacementParams.structureScale);
    m_StructureThreshold.set_value(firstJob.refPtAutoPlacementParams.structureThreshold);
}

std::vector<Glib::RefPtr<Gtk::FileFilter>> GetInputImageFilters()
{
    decltype(GetInputImageFilters()) result;

    auto fltAll = Gtk::FileFilter::create();
    fltAll->add_pattern("*");
    fltAll->set_name(_("All files"));
    result.push_back(fltAll);

    auto fltBmp = Gtk::FileFilter::create();
    fltBmp->add_pattern("*.tif");
    fltBmp->add_pattern("*.tiff");
    fltBmp->set_name("TIFF (*.tif, *.tiff)");
    result.push_back(fltBmp);

    return result;
}

void c_SettingsDlg::ApplySettings(Job_t &job)
{
    job.outputSaveMode = (Utils::Const::OutputSaveMode)m_OutputSaveMode.get_active_row_number();
    job.destDir = m_DestFolderChooser.get_filename();

    job.alignmentMethod = (enum SKRY_img_alignment_method)m_AlignmentMethod.get_active_row_number();

    job.refPtAutoPlacementParams.spacing = (unsigned)m_RefPtSpacing.get_value_as_int();
    job.refPtAutoPlacementParams.brightnessThreshold = m_RefPtBrightThresh.get_value() / 100.0;
    job.automaticRefPointsPlacement = (m_RefPtPlacementMode.get_active_row_number() == 0);
    if (job.automaticRefPointsPlacement)
        job.refPoints.clear();

    job.flatFieldFileName = m_FlatFieldChooser.get_filename();
    job.qualityCriterion = (enum SKRY_quality_criterion)m_QualityCriterion.get_active_row_number();
    job.qualityThreshold = m_QualityThreshold.get_value_as_int();
    job.outputFmt = Utils::Vars::outputFormatDescription[m_AutoSaveOutputFormat.get_active_row_number()].skryOutpFmt;
    job.cfaPattern = m_TreatMonoAsCFA.get_active()
                     ? (enum SKRY_CFA_pattern)m_CFAPattern.get_active_row_number()
                     : SKRY_CFA_NONE;
    job.imgSeq.ReinterpretAsCFA(job.cfaPattern);

    job.automaticAnchorPlacement = (m_VideoStbAnchorsMode.get_active_row_number() == 0);
    if (job.automaticAnchorPlacement)
        job.anchors.clear();

    job.refPtBlockSize = (unsigned)m_RefPtRefBlockSize.get_value();
    job.refPtSearchRadius = (unsigned)m_RefPtSearchRadius.get_value();
    job.refPtAutoPlacementParams.structureThreshold = (float)m_StructureThreshold.get_value();
    job.refPtAutoPlacementParams.structureScale = (unsigned)m_StructureScale.get_value();
}

void c_SettingsDlg::InitRefPointControls()
{
    auto frRefPt = Gtk::manage(new Gtk::Frame(_("Reference points")));
    auto refPtControls = Gtk::manage(new Gtk::VBox());
    refPtControls->show();
    frRefPt->add(*refPtControls);
    frRefPt->show();

    auto lRefPtMode = Gtk::manage(new Gtk::Label());
    lRefPtMode->set_text(_("Placement:"));

    m_RefPtPlacementMode.append(_("automatic"));
    m_RefPtPlacementMode.append(_("manual"));
    m_RefPtPlacementMode.set_tooltip_text(_("If set to manual, a selection dialog will be shown later during processing"));
    m_RefPtPlacementMode.set_active(0);
    m_RefPtPlacementMode.signal_changed().connect(sigc::mem_fun(*this, &c_SettingsDlg::OnRefPtMode));

    auto placementModeBox = Utils::PackIntoBox<Gtk::HBox>({ lRefPtMode, &m_RefPtPlacementMode });
    placementModeBox->set_valign(Gtk::Align::ALIGN_START);

    m_RefPtSpacing.set_adjustment(Gtk::Adjustment::create(/*TODO: make the default a constant*/40, 20, 80, 5));
    m_RefPtBrightThresh.set_adjustment(Gtk::Adjustment::create(100 * Utils::Const::Defaults::placementBrightnessThreshold, 0, 100));

    m_StructureThreshold.set_adjustment(Gtk::Adjustment::create(1.2, 1.0, 5.0, 0.01, 0.1));
    m_StructureThreshold.set_digits(2);
    m_StructureThreshold.set_tooltip_text(_("Relative local contrast level required to place a reference point; value of 1.2 usually works well"));

    m_StructureScale.set_adjustment(Gtk::Adjustment::create(1, 1, 5));
    m_StructureScale.set_digits(0);
    m_StructureScale.set_tooltip_text(_("Pixel size of the smallest structures. Should equal 1 for optimally-sampled "
                                        "or undersampled images. Use higher values for oversampled (blurry) material."));

    m_RefPtAutoControls = Utils::PackIntoBox<Gtk::VBox>( {
                            Utils::PackIntoBox<Gtk::HBox>({ Gtk::manage(new Gtk::Label(_("spacing in pixels:"))),        &m_RefPtSpacing }),
                            Utils::PackIntoBox<Gtk::HBox>({ Gtk::manage(new Gtk::Label(_("brightness threshold (%):"))), &m_RefPtBrightThresh }),
                            Utils::PackIntoBox<Gtk::HBox>({ Gtk::manage(new Gtk::Label(_("structure threshold:"))),      &m_StructureThreshold }),
                            Utils::PackIntoBox<Gtk::HBox>({ Gtk::manage(new Gtk::Label(_("structure scale:"))),          &m_StructureScale })
                          });

    refPtControls->pack_start(*Utils::PackIntoBox<Gtk::HBox>({ placementModeBox, m_RefPtAutoControls }),
                              Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    m_RefPtRefBlockSize.set_adjustment(Gtk::Adjustment::create(32, 8, 256));
    m_RefPtRefBlockSize.set_digits(0);

    m_RefPtSearchRadius.set_adjustment(Gtk::Adjustment::create(16, 1, 256, 1));
    m_RefPtSearchRadius.set_digits(0);

    refPtControls->pack_start(*Utils::PackIntoBox<Gtk::HBox>({ Gtk::manage(new Gtk::Label(_("Reference block size:"))), &m_RefPtRefBlockSize }),
                              Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);
    refPtControls->pack_start(*Utils::PackIntoBox<Gtk::HBox>({ Gtk::manage(new Gtk::Label(_("Search radius:"))), &m_RefPtSearchRadius }),
                              Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    get_content_area()->pack_start(*frRefPt, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);
}

void c_SettingsDlg::InitControls(const std::vector<std::string> &jobNames)
{
    auto lJobs = Gtk::manage(new Gtk::Label(_("Settings for job(s):"), Gtk::Align::ALIGN_START, Gtk::Align::ALIGN_CENTER));
    lJobs->show();
    get_content_area()->pack_start(*lJobs, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    auto *jobsList = Gtk::manage(new Gtk::TextView());
    jobsList->set_cursor_visible(false);
    jobsList->set_editable(false);
    for (size_t i = 0; i < jobNames.size(); i++)
    {
        jobsList->get_buffer()->insert_at_cursor(jobNames[i] + "\n");
    }
    jobsList->show();
    auto jobsListScrWin = Gtk::manage(new Gtk::ScrolledWindow());
    jobsListScrWin->add(*jobsList);
    jobsListScrWin->show();
    get_content_area()->pack_start(*jobsListScrWin, Gtk::PackOptions::PACK_EXPAND_WIDGET, Utils::Const::widgetPaddingInPixels);

    auto *jobsSeparator = Gtk::manage(new Gtk::Separator());
    jobsSeparator->show();
    get_content_area()->pack_start(*jobsSeparator, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    auto lAlignment = Gtk::manage(new Gtk::Label(_("Video stabilization method:")));
    lAlignment->show();
    for (auto s: GetAlignmentMethodStrings())
    {
        m_AlignmentMethod.append(s);
    }
    m_AlignmentMethod.set_active(0);
    m_AlignmentMethod.show();
    m_AlignmentMethod.signal_changed().connect([this]()
                                               {
                                                   bool anchorMode = (m_AlignmentMethod.get_active_row_number() == 0);
                                                   m_VideoStbAnchorsModeLabel.set_sensitive(anchorMode);
                                                   m_VideoStbAnchorsMode.set_sensitive(anchorMode);
                                               });

    m_VideoStbAnchorsModeLabel.set_text(_("anchor placement:"));
    m_VideoStbAnchorsModeLabel.show();
    m_VideoStbAnchorsMode.append(_("automatic"));
    m_VideoStbAnchorsMode.append(_("manual"));
    m_VideoStbAnchorsMode.set_active(0);
    m_VideoStbAnchorsMode.show();

    get_content_area()->pack_start(*Utils::PackIntoBox<Gtk::HBox>({ lAlignment, &m_AlignmentMethod,
                                                                    &m_VideoStbAnchorsModeLabel, &m_VideoStbAnchorsMode }),
                                   Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    InitRefPointControls();

    auto lStack = Gtk::manage(new Gtk::Label(_("Stacking criterion:")));
    lStack->show();

    m_QualityThreshold.set_adjustment(CreatePercentageAdj());
    m_QualityThreshold.show();

    for (auto &criterionStr: GetStackingCriteriaStrings())
        m_QualityCriterion.append(criterionStr);

    m_QualityCriterion.set_active(SKRY_quality_criterion::SKRY_MIN_REL_QUALITY);
    m_QualityCriterion.signal_changed().connect(sigc::mem_fun(*this, &c_SettingsDlg::OnStackingThresholdType));
    m_QualityCriterion.show();

    get_content_area()->pack_start(*Utils::PackIntoBox<Gtk::HBox>({ lStack, &m_QualityThreshold, &m_QualityCriterion }),
                                   Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);


    auto lOutp = Gtk::manage(new Gtk::Label());
    lOutp->set_text(_("Save stacked image automatically:"));
    lOutp->show();

    for (auto &saveModeStr: GetOutputSaveModeStrings())
        m_OutputSaveMode.append(saveModeStr);

    m_OutputSaveMode.signal_changed().connect(sigc::mem_fun(*this, &c_SettingsDlg::OnOutputSaveMode));
    m_OutputSaveMode.set_active((int)Utils::Const::Defaults::saveMode);
    m_OutputSaveMode.show();
    m_DestFolderChooser.set_create_folders(true);
    m_DestFolderChooser.set_action(Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SELECT_FOLDER);
    m_DestFolderChooser.signal_file_set().connect(sigc::mem_fun(*this, &c_SettingsDlg::OnDestPathSet));
    m_DestFolderChooser.set_sensitive(false);
    m_DestFolderChooser.show();
    get_content_area()->pack_start(*Utils::PackIntoBox<Gtk::HBox>({ lOutp, &m_OutputSaveMode, &m_DestFolderChooser }),
                                   Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    m_OutpFmtLabel.set_text(_("Output format:"));
    m_OutpFmtLabel.show();
    for (Utils::Vars::OutputFormatDescr_t &outFmt: Utils::Vars::outputFormatDescription)
    {
        m_AutoSaveOutputFormat.append(outFmt.name);
    }
    m_AutoSaveOutputFormat.show();
    get_content_area()->pack_start(*Utils::PackIntoBox<Gtk::HBox>({&m_OutpFmtLabel, &m_AutoSaveOutputFormat }),
        Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    m_FlatFieldCheckBtn.set_label(_("Use flat-field for stacking:"));
    m_FlatFieldCheckBtn.set_active(false);
    m_FlatFieldCheckBtn.signal_toggled().connect(sigc::mem_fun(*this, &c_SettingsDlg::OnFlatFieldToggle));
    m_FlatFieldCheckBtn.show();
    for (auto &filter: GetInputImageFilters())
    {
        m_FlatFieldChooser.add_filter(filter);
    }
    m_FlatFieldChooser.set_title(_("Select flat-field image"));
    m_FlatFieldChooser.set_sensitive(false);
    m_FlatFieldChooser.show();
    get_content_area()->pack_start(*Utils::PackIntoBox<Gtk::HBox>({ &m_FlatFieldCheckBtn, &m_FlatFieldChooser }),
                                   Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    m_TreatMonoAsCFA.set_label(_("Treat mono input as raw color:"));
    m_TreatMonoAsCFA.set_active(false);
    m_TreatMonoAsCFA.signal_toggled().connect(sigc::mem_fun(*this, &c_SettingsDlg::OnTreatAsCFAToggle));
    m_TreatMonoAsCFA.show();

    for (unsigned pattern = 0; pattern < SKRY_CFA_MAX; pattern++)
    {
        m_CFAPattern.append(SKRY_CFA_pattern_str[pattern]);
    }
    m_CFAPattern.set_active(0);
    m_CFAPattern.set_sensitive(false);
    m_CFAPattern.show();
    auto *lDemosaicComment = Gtk::manage(new Gtk::Label(_("(also overrides raw color format in SER videos)")));
    lDemosaicComment->show();
    get_content_area()->pack_start(*Utils::PackIntoBox<Gtk::HBox>({ &m_TreatMonoAsCFA, &m_CFAPattern, lDemosaicComment }),
                                    Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    auto separator = Gtk::manage(new Gtk::Separator());
    separator->show();
    get_content_area()->pack_end(*separator, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    add_button(_("OK"),  Gtk::ResponseType::RESPONSE_OK);
    add_button(_("Cancel"), Gtk::ResponseType::RESPONSE_CANCEL);
}

void c_SettingsDlg::OnDestPathSet()
{
    m_DestFolderChooser.set_tooltip_text(m_DestFolderChooser.get_filename());
}

void c_SettingsDlg::OnOutputSaveMode()
{
    m_DestFolderChooser.set_sensitive(m_OutputSaveMode.get_active_row_number() == 2);

    bool autoSaveDisabled =
        (m_OutputSaveMode.get_active_row_number() == (int)Utils::Const::OutputSaveMode::NONE);
    m_OutpFmtLabel.set_sensitive(!autoSaveDisabled);
    m_AutoSaveOutputFormat.set_sensitive(!autoSaveDisabled);
}

void c_SettingsDlg::OnRefPtMode()
{
    bool isAuto = (m_RefPtPlacementMode.get_active_row_number() == 0);
    m_RefPtAutoControls->set_sensitive(isAuto);
}

Glib::RefPtr<Gtk::Adjustment> c_SettingsDlg::CreatePercentageAdj()
{
    return Gtk::Adjustment::create(50, 1, 100);
}

void c_SettingsDlg::OnStackingThresholdType()
{
    if (m_QualityCriterion.get_active_row_number() == (int)SKRY_quality_criterion::SKRY_NUMBER_BEST)
        m_QualityThreshold.set_adjustment(Gtk::Adjustment::create(100, 1, 20000, 10, 50));
    else
        m_QualityThreshold.set_adjustment(CreatePercentageAdj());
}

c_SettingsDlg::~c_SettingsDlg()
{
    Utils::SavePosSize(*this, Configuration::SettingsDlgPosSize);
}


void c_SettingsDlg::OnFlatFieldToggle()
{
    m_FlatFieldChooser.set_sensitive(m_FlatFieldCheckBtn.get_active());
}

void c_SettingsDlg::OnTreatAsCFAToggle()
{
    m_CFAPattern.set_sensitive(m_TreatMonoAsCFA.get_active());
}
