#ifndef WX_INSPECTOR_OBJECT_H
#define WX_INSPECTOR_OBJECT_H

#include <wx/object.h>
#include <wx/string.h>
#include <wx/window.h>
#include <wx/sizer.h>

namespace wxInspector {

class InspectableObject {
public:
    enum class Kind {
        Window,
        Sizer,
        SizerItem
    };

    static InspectableObject FromWindow(wxWindow* window);
    static InspectableObject FromSizer(wxSizer* sizer);
    static InspectableObject FromSizerItem(wxSizerItem* item);

    Kind GetKind() const { return m_kind; }
    wxString GetDisplayName() const;
    wxString GetClassName() const;
    wxString GetClassHierarchy() const;

    wxWindow* AsWindow() const;
    wxSizer* AsSizer() const;
    wxSizerItem* AsSizerItem() const;
    wxObject* GetObject() const { return m_object; }

    bool IsValid() const;

private:
    InspectableObject(Kind kind, wxObject* obj);

    Kind m_kind;
    wxObject* m_object;
};

} // namespace wxInspector

#endif // WX_INSPECTOR_OBJECT_H
