#ifndef WX_INSPECTOR_INSPECTOR_H
#define WX_INSPECTOR_INSPECTOR_H

#include <wx/object.h>
#include <wx/app.h>
#include "wx/inspector/plugin.h"

class wxConfigBase;
class wxCommandEvent;

namespace wxInspector {

class InspectionFrame;

void RegisterPlugin(wxInspectorPlugin* plugin);

} // namespace wxInspector

class wxInspectable {
public:
    wxInspectable();
    virtual ~wxInspectable();

    wxInspectable(const wxInspectable&) = delete;
    wxInspectable& operator=(const wxInspectable&) = delete;

    void ShowInspector(wxObject* selectObj = nullptr);
    void HideInspector();
    bool IsInspectorVisible() const;
    void RefreshInspectorTree();
    void SelectInspectorObject(wxObject* obj);

protected:
    void SetupInspectorAccelerator(wxWindow* window);

private:
    static const int ID_INSPECTOR_TOGGLE = wxID_HIGHEST + 9999;
    void OnToggleInspector(wxCommandEvent& event);

    wxInspector::InspectionFrame* m_inspectorFrame;
    wxWindow* m_accelWindow;
};

#endif // WX_INSPECTOR_INSPECTOR_H
