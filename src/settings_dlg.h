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
#include <gtkmm/spinbutton.h>

#include "utils.h"
#include "worker.h"


class c_SettingsDlg: public Gtk::Dialog
{
public:
    c_SettingsDlg(const std::vector<std::string> &jobNames, const Job_t &firstJob);
    ~c_SettingsDlg();

    void ApplySettings(Job_t &job);

private:
    Gtk::ComboBoxText m_OutputSaveMode;
    Gtk::Label m_VideoStbAnchorsModeLabel;
    Gtk::ComboBoxText m_VideoStbAnchorsMode;
    Gtk::FileChooserButton m_DestFolderChooser;
    Gtk::ComboBoxText m_QualityCriterion;
    Gtk::SpinButton m_QualityThreshold;
    Gtk::FileChooserButton m_FlatFieldChooser;
    Gtk::CheckButton m_FlatFieldCheckBtn;
    Gtk::ComboBoxText m_AutoSaveOutputFormat;
    Gtk::Label m_OutpFmtLabel;
    Gtk::CheckButton m_TreatMonoAsCFA;
    Gtk::ComboBoxText m_CFAPattern;
    Gtk::ComboBoxText m_AlignmentMethod;
    Gtk::CheckButton m_ExportQualityData;

    // Reference point placement parameters
    Gtk::ComboBoxText m_RefPtPlacementMode;

    Gtk::VBox *m_RefPtAutoControls;

    Gtk::SpinButton m_RefPtSpacing;
    Gtk::SpinButton m_RefPtBrightThresh;
    Gtk::SpinButton m_RefPtSearchRadius;
    Gtk::SpinButton m_RefPtRefBlockSize;
    Gtk::SpinButton m_StructureThreshold;
    Gtk::SpinButton m_StructureScale;


    void InitControls(const std::vector<std::string> &jobNames);
    Glib::RefPtr<Gtk::Adjustment> CreatePercentageAdj();
    void InitRefPointControls();

    // Signal handlers --------------
    void OnRefPtMode();
    void OnStackingThresholdType();
    void OnOutputSaveMode();
    void OnDestPathSet();
    void OnFlatFieldToggle();
    void OnTreatAsCFAToggle();
};


#endif // STACKISTRY_SETTINGS_DIALOG_HEADER
