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
    Processing settings dialog header.
*/

#ifndef STACKISTRY_SETTINGS_DIALOG_HEADER
#define STACKISTRY_SETTINGS_DIALOG_HEADER

#include <string>

#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/dialog.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/filechooserbutton.h>
#include <gtkmm/label.h>
#include <gtkmm/spinbutton.h>

#include "utils.h"


class c_SettingsDlg: public Gtk::Dialog
{
public:
    c_SettingsDlg(const std::vector<std::string> &jobNames);
    ~c_SettingsDlg();

    Utils::Const::OutputSaveMode GetOutputSaveMode() const;
    void SetOutputSaveMode(Utils::Const::OutputSaveMode mode);

    void SetDestinationDir(std::string dir);
    std::string GetDestinationDir() const;

    unsigned GetRefPointSpacing() const;
    void SetRefPtSpacing(unsigned sp);

    double GetRefPointBrightThresh() const;
    void SetRefPtBrightThresh(double threshold);

    std::string GetFlatFieldFileName() const;
    /// Specify an empty string to disable flat-fielding
    void SetFlatFieldFileName(std::string flatField);

    bool GetAnchorsAutomatic() const;
    void SetAnchorsAutomatic(bool automatic);

    bool GetRefPointsAutomatic() const;
    void SetRefPointsAutomatic(bool automatic);

    enum SKRY_quality_criterion GetQualityCriterion() const;
    unsigned GetQualityThreshold() const;
    void SetQualityCriterion(enum SKRY_quality_criterion qualCrit, unsigned threshold);

    enum SKRY_output_format GetAutoSaveOutputFormat() const;
    void SetAutoSaveOutputFormat(enum SKRY_output_format outpFmt);

    enum SKRY_CFA_pattern GetCFAPattern() const;
    void SetCFAPattern(enum SKRY_CFA_pattern pattern);

private:
    Gtk::ComboBoxText m_OutputSaveMode;
    Gtk::ComboBoxText m_VideoStbAnchorsMode;
    Gtk::ComboBoxText m_RefPtPlacementMode;
    Gtk::Label m_RefPtSpacingLabel;
    Gtk::SpinButton m_RefPtSpacing;
    Gtk::Label m_RefPtBrightThreshLabel;
    Gtk::SpinButton m_RefPtBrightThresh;
    Gtk::FileChooserButton m_DestFolderChooser;
    Gtk::ComboBoxText m_QualityCriterion;
    Gtk::SpinButton m_QualityThreshold;
    Gtk::FileChooserButton m_FlatFieldChooser;
    Gtk::CheckButton m_FlatFieldCheckBtn;
    Gtk::ComboBoxText m_AutoSaveOutputFormat;
    Gtk::Label m_OutpFmtLabel;

    Gtk::CheckButton m_TreatMonoAsCFA;
    Gtk::ComboBoxText m_CFAPattern;

    void InitControls(const std::vector<std::string> &jobNames);
    Glib::RefPtr<Gtk::Adjustment> CreatePercentageAdj();


    // Signal handlers --------------
    void OnRefPtMode();
    void OnStackingThresholdType();
    void OnOutputSaveMode();
    void OnDestPathSet();
    void OnFlatFieldToggle();
    void OnTreatAsCFAToggle();
};


#endif // STACKISTRY_SETTINGS_DIALOG_HEADER
