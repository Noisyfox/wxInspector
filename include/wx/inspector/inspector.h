#ifndef WX_INSPECTOR_INSPECTOR_H
#define WX_INSPECTOR_INSPECTOR_H

#include <wx/object.h>
#include <wx/app.h>
#include "wx/inspector/plugin.h"

class wxConfigBase;
class wxCommandEvent;

namespace wxInspector {

class InspectionFrame;

// --- Registration ---

#ifdef WXINSPECTOR_DISABLE
    inline void RegisterPlugin(wxInspectorPlugin*) {}
#else
    namespace detail {
        void RegisterPluginImpl(wxInspectorPlugin* plugin);
    }
    inline void RegisterPlugin(wxInspectorPlugin* plugin) {
        detail::RegisterPluginImpl(plugin);
    }
#endif

// --- Mixin (real implementation, always in library) ---

#ifndef WXINSPECTOR_DISABLE
class wxInspectableImpl {
public:
    wxInspectableImpl();

    wxInspectableImpl(const wxInspectableImpl&) = delete;
    wxInspectableImpl& operator=(const wxInspectableImpl&) = delete;

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
#endif

// --- Public-facing type ---

#ifdef WXINSPECTOR_DISABLE
class wxInspectable {
public:
    void ShowInspector(wxObject* = nullptr) {}
    void HideInspector() {}
    bool IsInspectorVisible() const { return false; }
    void RefreshInspectorTree() {}
    void SelectInspectorObject(wxObject*) {}
protected:
    void SetupInspectorAccelerator(wxWindow*) {}
};
#else
using wxInspectable = wxInspectableImpl;
#endif

} // namespace wxInspector

#endif // WX_INSPECTOR_INSPECTOR_H
