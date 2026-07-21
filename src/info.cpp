#include "wx/inspector/info.h"
#include "wx/inspector/property_provider.h"
#include <wx/sizer.h>
#include <wx/toplevel.h>
#include <wx/propgrid/advprops.h>

namespace wxInspector {

ObjectInfoPanel::ObjectInfoPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY),
      m_currentObj(InspectableObject::FromWindow(nullptr))
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    m_pg = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition,
        wxDefaultSize,
        wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER |
        wxPG_DEFAULT_STYLE);
    m_pg->SetExtraStyle(wxPG_EX_MODE_BUTTONS);
    sizer->Add(m_pg, 1, wxEXPAND);
    SetSizer(sizer);

    m_pg->Bind(wxEVT_PG_CHANGED, &ObjectInfoPanel::OnPropertyChanged, this);
}

void ObjectInfoPanel::Clear()
{
    m_pg->Clear();
}

void ObjectInfoPanel::ShowObject(InspectableObject& obj)
{
    m_currentObj = obj;
    PopulateGrid(obj);
}

void ObjectInfoPanel::Refresh()
{
    if (m_currentObj.IsValid()) {
        PopulateGrid(m_currentObj);
    }
}

void ObjectInfoPanel::PopulateGrid(InspectableObject& obj)
{
    m_pg->Freeze();
    m_pg->Clear();

    if (!obj.IsValid()) {
        m_pg->Append(new wxPropertyCategory("Info"));
        m_pg->Append(new wxStringProperty("Status", wxPG_LABEL,
            "Object destroyed or invalid"));
        m_pg->Thaw();
        return;
    }

    wxVector<PropertyDef> props = PropertyProvider::Get().GetProperties(obj);

    // Group by category
    wxString currentCategory;
    for (auto& prop : props) {
        if (prop.category != currentCategory) {
            currentCategory = prop.category;
            m_pg->Append(new wxPropertyCategory(prop.category));
        }

        wxPGProperty* pgProp = nullptr;
        switch (prop.type) {
        case PropertyType::ReadOnly:
            pgProp = new wxStringProperty(prop.name, wxPG_LABEL, prop.value);
            pgProp->ChangeFlag(wxPGFlags::ReadOnly, true);
            break;
        case PropertyType::Boolean:
            pgProp = new wxBoolProperty(prop.name, wxPG_LABEL,
                prop.value == "true");
            break;
        case PropertyType::Integer:
            pgProp = new wxIntProperty(prop.name, wxPG_LABEL);
            pgProp->SetValueFromString(prop.value);
            break;
        case PropertyType::Colour:
            pgProp = new wxColourProperty(prop.name, wxPG_LABEL,
                wxColour(prop.value));
            break;
        case PropertyType::String:
        case PropertyType::Font:
        case PropertyType::Size:
        case PropertyType::Point:
        case PropertyType::Rect:
            pgProp = new wxStringProperty(prop.name, wxPG_LABEL, prop.value);
            break;
        case PropertyType::Choice:
            if (!prop.choices.empty()) {
                wxArrayString choices;
                wxArrayInt values;
                for (size_t i = 0; i < prop.choices.size(); i++) {
                    choices.Add(prop.choices[i]);
                    values.Add((int)i);
                }
                pgProp = new wxEnumProperty(prop.name, wxPG_LABEL, choices, values);
                int sel = 0;
                for (size_t i = 0; i < prop.choices.size(); i++) {
                    if (prop.choices[i] == prop.value) { sel = (int)i; break; }
                }
                pgProp->SetChoiceSelection(sel);
            }
            break;
        }

        if (pgProp) {
            if (prop.readOnly) {
                pgProp->ChangeFlag(wxPGFlags::ReadOnly, true);
            }
            // Store the setter in the property's client data
            // We use a custom approach: store setter index in a map
            m_pg->Append(pgProp);
        }
    }

    m_pg->Thaw();
}

void ObjectInfoPanel::OnPropertyChanged(wxPropertyGridEvent& event)
{
    wxPGProperty* pgProp = event.GetProperty();
    if (!pgProp) return;

    wxString propName = pgProp->GetName();
    wxString newValue = pgProp->GetValueAsString();

    // Re-fetch properties and find the matching setter
    if (!m_currentObj.IsValid()) return;
    wxVector<PropertyDef> props =
        PropertyProvider::Get().GetProperties(m_currentObj);
    for (auto& prop : props) {
        if (prop.name == propName && prop.setter) {
            prop.setter(newValue);
            break;
        }
    }
}

} // namespace wxInspector
