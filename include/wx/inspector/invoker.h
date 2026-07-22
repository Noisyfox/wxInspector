#ifndef WX_INSPECTOR_INVOKER_H
#define WX_INSPECTOR_INVOKER_H

#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include "wx/inspector/object.h"
#include "wx/inspector/plugin.h"

namespace wxInspector {

class MethodInvokerPanel : public wxPanel {
public:
    MethodInvokerPanel(wxWindow* parent);

    void ShowObject(InspectableObject& obj);
    void Clear();

private:
    void OnMethodSelected(wxCommandEvent& event);
    void OnInvoke(wxCommandEvent& event);

    void UpdateMethodList(InspectableObject& obj);

    wxChoice* m_methodChoice;
    wxTextCtrl* m_paramField;
    wxButton* m_invokeBtn;
    wxTextCtrl* m_resultArea;

    wxVector<MethodInfo> m_currentMethods;
    InspectableObject m_currentObj;
};

} // namespace wxInspector

#endif // WX_INSPECTOR_INVOKER_H
