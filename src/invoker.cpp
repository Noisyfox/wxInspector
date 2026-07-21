#include "wx/inspector/invoker.h"
#include "wx/inspector/method_registry.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/config.h>
#include <wx/tokenzr.h>
#include <wx/clipbrd.h>

namespace wxInspector {

enum {
    ID_METHOD_CHOICE = wxID_HIGHEST + 200,
    ID_PARAM_FIELD,
    ID_INVOKE_BTN,
    ID_RESULT_AREA,
    ID_HISTORY_LIST
};

MethodInvokerPanel::MethodInvokerPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY),
      m_currentObj(InspectableObject::FromWindow(nullptr))
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Method selection row
    wxBoxSizer* methodRow = new wxBoxSizer(wxHORIZONTAL);
    methodRow->Add(new wxStaticText(this, wxID_ANY, "Method:"),
        0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_methodChoice = new wxChoice(this, ID_METHOD_CHOICE);
    methodRow->Add(m_methodChoice, 1, wxEXPAND);
    mainSizer->Add(methodRow, 0, wxEXPAND | wxALL, 4);

    // Parameter row
    wxBoxSizer* paramRow = new wxBoxSizer(wxHORIZONTAL);
    paramRow->Add(new wxStaticText(this, wxID_ANY, "Args:"),
        0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_paramField = new wxTextCtrl(this, ID_PARAM_FIELD,
        wxEmptyString, wxDefaultPosition, wxDefaultSize,
        wxTE_PROCESS_ENTER);
    paramRow->Add(m_paramField, 1, wxEXPAND);
    m_invokeBtn = new wxButton(this, ID_INVOKE_BTN, "Invoke");
    paramRow->Add(m_invokeBtn, 0, wxLEFT, 5);
    mainSizer->Add(paramRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

    // Result area
    wxStaticBox* resultBox = new wxStaticBox(this, wxID_ANY, "Result");
    wxStaticBoxSizer* resultSizer = new wxStaticBoxSizer(resultBox, wxVERTICAL);
    m_resultArea = new wxTextCtrl(this, ID_RESULT_AREA, wxEmptyString,
        wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY);
    m_resultArea->SetMinSize(wxSize(-1, 80));
    resultSizer->Add(m_resultArea, 1, wxEXPAND);
    mainSizer->Add(resultSizer, 1, wxEXPAND | wxALL, 4);

    // History
    wxStaticBox* histBox = new wxStaticBox(this, wxID_ANY, "History");
    wxStaticBoxSizer* histSizer = new wxStaticBoxSizer(histBox, wxVERTICAL);
    m_historyList = new wxListBox(this, ID_HISTORY_LIST);
    m_historyList->SetMinSize(wxSize(-1, 60));
    histSizer->Add(m_historyList, 1, wxEXPAND);
    mainSizer->Add(histSizer, 0, wxEXPAND | wxALL, 4);

    SetSizer(mainSizer);

    m_methodChoice->Bind(wxEVT_CHOICE, &MethodInvokerPanel::OnMethodSelected, this);
    m_invokeBtn->Bind(wxEVT_BUTTON, &MethodInvokerPanel::OnInvoke, this);
    m_paramField->Bind(wxEVT_TEXT_ENTER, &MethodInvokerPanel::OnInvoke, this);
    m_historyList->Bind(wxEVT_LISTBOX, &MethodInvokerPanel::OnHistorySelect, this);

    // Load history from config
    wxConfigBase* cfg = wxConfig::Get();
    if (cfg) {
        cfg->SetPath("/wxInspector/MethodHistory");
        for (int i = 0; i < MAX_HISTORY; i++) {
            wxString key = wxString::Format("entry%d", i);
            wxString value;
            if (cfg->Read(key, &value) && !value.empty()) {
                m_history.push_back(value);
            }
        }
        cfg->SetPath("/");
    }

    // Populate history list from loaded entries
    for (auto it = m_history.rbegin(); it != m_history.rend(); ++it)
        m_historyList->Append(*it);
}

void MethodInvokerPanel::ShowObject(InspectableObject& obj)
{
    m_currentObj = obj;
    UpdateMethodList(obj);
}

void MethodInvokerPanel::Clear()
{
    m_methodChoice->Clear();
    m_paramField->Clear();
    m_resultArea->Clear();
    m_currentMethods.clear();
}

void MethodInvokerPanel::UpdateMethodList(InspectableObject& obj)
{
    m_methodChoice->Clear();
    m_currentMethods.clear();

    if (!obj.IsValid()) return;

    m_currentMethods = MethodRegistry::Get().GetMethods(obj);
    for (auto& m : m_currentMethods) {
        wxString label = m.name + " — " + m.signature;
        m_methodChoice->Append(label);
    }

    if (!m_currentMethods.empty())
        m_methodChoice->Select(0);
}

void MethodInvokerPanel::OnMethodSelected(wxCommandEvent& event)
{
    int sel = event.GetSelection();
    if (sel >= 0 && sel < (int)m_currentMethods.size()) {
        m_resultArea->SetValue(
            m_currentMethods[sel].name + ": " +
            m_currentMethods[sel].description);
    }
}

void MethodInvokerPanel::OnInvoke(wxCommandEvent&)
{
    int sel = m_methodChoice->GetSelection();
    if (sel < 0 || sel >= (int)m_currentMethods.size()) {
        m_resultArea->SetValue("Error: No method selected");
        return;
    }

    // Parse arguments
    wxString argStr = m_paramField->GetValue();
    wxVector<wxString> args;
    wxStringTokenizer tk(argStr, ",", wxTOKEN_STRTOK);
    while (tk.HasMoreTokens()) {
        wxString token = tk.GetNextToken();
        token.Trim(true).Trim(false);
        if (!token.empty()) args.push_back(token);
    }

    MethodInfo& method = m_currentMethods[sel];
    wxString result;
    if (method.invoke) {
        result = method.invoke(args);
    } else {
        result = "Error: Method has no invoke handler";
    }

    m_resultArea->SetValue(result);

    // Add to history
    wxString histEntry = method.name + "(" + argStr + ") -> " + result;
    m_history.push_back(histEntry);
    if (m_history.size() > MAX_HISTORY)
        m_history.erase(m_history.begin());

    m_historyList->Clear();
    for (auto it = m_history.rbegin(); it != m_history.rend(); ++it)
        m_historyList->Append(*it);

    // Save history to config
    wxConfigBase* cfg = wxConfig::Get();
    if (cfg) {
        cfg->SetPath("/wxInspector/MethodHistory");
        for (size_t i = 0; i < m_history.size(); i++) {
            wxString key = wxString::Format("entry%zu", i);
            cfg->Write(key, m_history[i]);
        }
        cfg->SetPath("/");
    }
}

void MethodInvokerPanel::OnHistorySelect(wxCommandEvent& event)
{
    int sel = event.GetSelection();
    if (sel >= 0 && sel < (int)m_history.size()) {
        // History is stored in reverse order
        int actualIdx = (int)m_history.size() - 1 - sel;
        if (actualIdx >= 0) {
            m_resultArea->SetValue(m_history[actualIdx]);
        }
    }
}

} // namespace wxInspector
