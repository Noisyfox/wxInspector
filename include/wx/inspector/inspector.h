#ifndef WX_INSPECTOR_INSPECTOR_H
#define WX_INSPECTOR_INSPECTOR_H

#include <wx/object.h>
#include <wx/app.h>
#include "wx/inspector/plugin.h"

class wxConfigBase;

namespace wxInspector {

// --- Public API ---

bool Init(wxConfigBase* config = nullptr);
void Show(wxObject* selectObj = nullptr, bool refreshTree = false);
void Hide();
void RefreshTree();
void SelectObject(wxObject* obj);
bool IsVisible();
void RegisterPlugin(wxInspectorPlugin* plugin);

} // namespace wxInspector

// --- wxInspectorMixin ---

class wxInspectorMixin {
public:
    wxInspectorMixin();
    ~wxInspectorMixin();

    wxInspectorMixin(const wxInspectorMixin&) = delete;
    wxInspectorMixin& operator=(const wxInspectorMixin&) = delete;

protected:
    void SetupInspectorAccelerator(wxWindow* window);

private:
    static const int ID_INSPECTOR_TOGGLE = wxID_HIGHEST + 9999;
    void OnToggleInspector(wxCommandEvent& event);
    wxWindow* m_accelWindow;
};

#endif // WX_INSPECTOR_INSPECTOR_H
