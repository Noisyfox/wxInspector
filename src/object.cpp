#include "wx/inspector/object.h"
#include <wx/toplevel.h>
#include <wx/sizer.h>

namespace wxInspector {

InspectableObject::InspectableObject(Kind kind, wxObject* obj)
    : m_kind(kind)
    , m_object(obj)
{
}

InspectableObject InspectableObject::FromWindow(wxWindow* window)
{
    return InspectableObject(Kind::Window, static_cast<wxObject*>(window));
}

InspectableObject InspectableObject::FromSizer(wxSizer* sizer)
{
    return InspectableObject(Kind::Sizer, static_cast<wxObject*>(sizer));
}

InspectableObject InspectableObject::FromSizerItem(wxSizerItem* item)
{
    return InspectableObject(Kind::SizerItem, static_cast<wxObject*>(item));
}

wxString InspectableObject::GetDisplayName() const
{
    if (!m_object) return "(null)";

    switch (m_kind) {
        case Kind::Window: {
            wxWindow* win = static_cast<wxWindow*>(m_object);
            wxString label = win->GetLabel();
            if (label.empty()) {
                wxTopLevelWindow* tlw = wxDynamicCast(win, wxTopLevelWindow);
                if (tlw) label = tlw->GetTitle();
            }
            wxString name = win->GetName();
            wxString result = GetClassName();
            if (!label.empty()) {
                result += wxT(" \"") + label + wxT("\"");
            }
            if (!name.empty()) {
                result += wxT(" [") + name + wxT("]");
            }
            return result;
        }
        case Kind::Sizer: {
            return GetClassName();
        }
        case Kind::SizerItem: {
            wxSizerItem* item = static_cast<wxSizerItem*>(m_object);
            if (item->IsWindow()) {
                return wxT("Item: ") + InspectableObject::FromWindow(item->GetWindow()).GetDisplayName();
            } else if (item->IsSizer()) {
                return wxT("Item: ") + InspectableObject::FromSizer(item->GetSizer()).GetDisplayName();
            } else if (item->IsSpacer()) {
                wxSize size = item->GetSpacer();
                return wxString::Format("Spacer (%d, %d)", size.x, size.y);
            }
            return wxT("Item: (unknown)");
        }
    }
    return wxT("(unknown)");
}

wxString InspectableObject::GetClassName() const
{
    if (!m_object) return wxT("(null)");
    const wxClassInfo* info = m_object->GetClassInfo();
    if (info) {
        return wxString(info->GetClassName());
    }
    return wxT("wxObject");
}

wxString InspectableObject::GetClassHierarchy() const
{
    if (!m_object) return wxT("(null)");
    wxString result;
    const wxClassInfo* info = m_object->GetClassInfo();
    while (info) {
        if (!result.empty()) result += wxT(" <- ");
        result += wxString(info->GetClassName());
        info = info->GetBaseClass1();
    }
    return result;
}

wxWindow* InspectableObject::AsWindow() const
{
    if (m_kind == Kind::Window && m_object) {
        return static_cast<wxWindow*>(m_object);
    }
    return nullptr;
}

wxSizer* InspectableObject::AsSizer() const
{
    if (m_kind == Kind::Sizer && m_object) {
        return static_cast<wxSizer*>(m_object);
    }
    return nullptr;
}

wxSizerItem* InspectableObject::AsSizerItem() const
{
    if (m_kind == Kind::SizerItem && m_object) {
        return static_cast<wxSizerItem*>(m_object);
    }
    return nullptr;
}

bool InspectableObject::IsValid() const
{
    return m_object != nullptr;
}

} // namespace wxInspector
