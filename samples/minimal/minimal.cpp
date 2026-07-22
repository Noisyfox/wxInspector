#include <wx/wx.h>
#include <wx/inspector/inspector.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/slider.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/spinctrl.h>
#include <wx/radiobut.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/sizer.h>
#include <wx/menu.h>
#include <wx/aboutdlg.h>

class SampleFrame : public wxFrame {
public:
    SampleFrame() : wxFrame(nullptr, wxID_ANY, "wxInspector Sample",
                             wxDefaultPosition, wxSize(500, 400))
    {
        SetupMenu();
        SetupUI();
        CreateStatusBar();
        SetStatusText("Press Ctrl+Shift+I to open wxInspector");
    }

private:
    void SetupMenu() {
        wxMenu* fileMenu = new wxMenu();
        fileMenu->Append(wxID_EXIT, "E&xit\tAlt+X");
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(true); }, wxID_EXIT);

        wxMenu* helpMenu = new wxMenu();
        helpMenu->Append(wxID_ABOUT, "&About");
        Bind(wxEVT_MENU, [this](wxCommandEvent&) {
            wxAboutDialogInfo info;
            info.SetName("wxInspector Sample");
            info.SetDescription("A minimal wxWidgets app for testing wxInspector");
            wxAboutBox(info);
        }, wxID_ABOUT);

        wxMenuBar* mb = new wxMenuBar();
        mb->Append(fileMenu, "&File");
        mb->Append(helpMenu, "&Help");
        SetMenuBar(mb);
    }

    void SetupUI() {
        wxPanel* panel = new wxPanel(this, wxID_ANY);
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        // --- Controls section ---
        wxStaticBox* ctrlBox = new wxStaticBox(panel, wxID_ANY, "Controls");
        wxStaticBoxSizer* ctrlSizer = new wxStaticBoxSizer(ctrlBox, wxVERTICAL);

        // Row 1: Text entry
        wxBoxSizer* row1 = new wxBoxSizer(wxHORIZONTAL);
        row1->Add(new wxStaticText(panel, wxID_ANY, "Name:"),
            0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
        wxTextCtrl* textCtrl = new wxTextCtrl(panel, wxID_ANY, "Sample text");
        textCtrl->SetName("nameTextCtrl");
        row1->Add(textCtrl, 1, wxEXPAND);
        ctrlSizer->Add(row1, 0, wxEXPAND | wxALL, 5);

        // Row 2: Checkbox + Radio buttons
        wxBoxSizer* row2 = new wxBoxSizer(wxHORIZONTAL);
        wxCheckBox* check = new wxCheckBox(panel, wxID_ANY, "Enable feature");
        check->SetName("enableCheckBox");
        check->SetValue(true);
        row2->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 20);
        row2->Add(new wxRadioButton(panel, wxID_ANY, "Option A"), 0, wxALIGN_CENTER_VERTICAL);
        row2->Add(new wxRadioButton(panel, wxID_ANY, "Option B"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
        ctrlSizer->Add(row2, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

        // Row 3: Slider + SpinCtrl
        wxBoxSizer* row3 = new wxBoxSizer(wxHORIZONTAL);
        row3->Add(new wxStaticText(panel, wxID_ANY, "Value:"),
            0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
        wxSlider* slider = new wxSlider(panel, wxID_ANY, 50, 0, 100);
        slider->SetName("valueSlider");
        row3->Add(slider, 1, wxEXPAND);
        wxSpinCtrl* spin = new wxSpinCtrl(panel, wxID_ANY, "50",
            wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100);
        spin->SetName("valueSpin");
        row3->Add(spin, 0, wxLEFT, 10);
        ctrlSizer->Add(row3, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

        // Row 4: Choice + ComboBox
        wxBoxSizer* row4 = new wxBoxSizer(wxHORIZONTAL);
        row4->Add(new wxStaticText(panel, wxID_ANY, "Type:"),
            0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
        wxChoice* choice = new wxChoice(panel, wxID_ANY,
            wxDefaultPosition, wxDefaultSize,
            wxArrayString{"Type A", "Type B", "Type C"});
        choice->SetName("typeChoice");
        choice->Select(0);
        row4->Add(choice, 1);
        wxComboBox* combo = new wxComboBox(panel, wxID_ANY, "",
            wxDefaultPosition, wxDefaultSize,
            wxArrayString{"Item 1", "Item 2", "Item 3"});
        combo->SetName("itemCombo");
        combo->SetValue("Item 1");
        row4->Add(combo, 1, wxLEFT, 10);
        ctrlSizer->Add(row4, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

        // Row 5: Gauge
        wxGauge* gauge = new wxGauge(panel, wxID_ANY, 100);
        gauge->SetName("progressGauge");
        gauge->SetValue(65);
        ctrlSizer->Add(gauge, 0, wxEXPAND | wxALL, 5);

        // Row 6: Buttons
        wxBoxSizer* row6 = new wxBoxSizer(wxHORIZONTAL);
        wxButton* okBtn = new wxButton(panel, wxID_OK, "OK");
        okBtn->SetName("okButton");
        wxButton* cancelBtn = new wxButton(panel, wxID_CANCEL, "Cancel");
        cancelBtn->SetName("cancelButton");
        wxButton* applyBtn = new wxButton(panel, wxID_APPLY, "Apply");
        applyBtn->SetName("applyButton");
        row6->Add(okBtn, 0, wxRIGHT, 5);
        row6->Add(cancelBtn, 0, wxRIGHT, 5);
        row6->Add(applyBtn, 0);
        ctrlSizer->Add(row6, 0, wxALIGN_RIGHT | wxALL, 5);

        mainSizer->Add(ctrlSizer, 0, wxEXPAND | wxALL, 10);

        // --- Nested panel with its own sizer ---
        wxStaticBox* nestedBox = new wxStaticBox(panel, wxID_ANY, "Nested Panel");
        wxStaticBoxSizer* nestedSizer = new wxStaticBoxSizer(nestedBox, wxHORIZONTAL);
        wxPanel* nestedPanel = new wxPanel(panel, wxID_ANY);
        nestedPanel->SetName("nestedPanel");
        wxBoxSizer* nestedInner = new wxBoxSizer(wxVERTICAL);
        nestedInner->Add(new wxStaticText(nestedPanel, wxID_ANY, "Sub-item 1"),
            0, wxALL, 5);
        nestedInner->Add(new wxTextCtrl(nestedPanel, wxID_ANY, "nested text"),
            0, wxEXPAND | wxALL, 5);
        nestedInner->Add(new wxCheckBox(nestedPanel, wxID_ANY, "Nested option"),
            0, wxALL, 5);
        nestedPanel->SetSizer(nestedInner);
        nestedSizer->Add(nestedPanel, 1, wxEXPAND | wxALL, 5);
        mainSizer->Add(nestedSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

        panel->SetSizer(mainSizer);

        // Wire up OK button to show a message
        okBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            wxMessageBox("OK clicked!", "wxInspector Sample");
        });
    }
};

class SampleApp : public wxApp, public wxInspectorMixin {
public:
    bool OnInit() override {
        if (!wxInspector::Init()) {
            wxLogError("Failed to initialize wxInspector");
        }

        SampleFrame* frame = new SampleFrame();

        // Setup inspector accelerator on the main frame
        SetupInspectorAccelerator(frame);

        frame->Show(true);

        return true;
    }
};

wxIMPLEMENT_APP_CONSOLE(SampleApp);
