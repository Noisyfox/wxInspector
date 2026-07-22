#ifndef WX_INSPECTOR_FRAME_H
#define WX_INSPECTOR_FRAME_H

#include <wx/frame.h>
#include <wx/aui/aui.h>
#include "wx/inspector/object.h"

namespace wxInspector {

class InspectionTree;
class ObjectInfoPanel;
class MethodInvokerPanel;
class EventLoggerPanel;
class InspectionHighlighter;

class InspectionFrame : public wxFrame {
public:
    InspectionFrame(wxWindow* parent, wxPoint pos = wxDefaultPosition,
                    wxSize size = wxDefaultSize);
    ~InspectionFrame();

    void Show(wxObject* selectObj = nullptr);
    void SelectObject(wxObject* obj);
    void RefreshTree();

    InspectionTree* GetTree() { return m_tree; }
    ObjectInfoPanel* GetInfoPanel() { return m_info; }
    MethodInvokerPanel* GetInvokerPanel() { return m_invoker; }
    EventLoggerPanel* GetEventPanel() { return m_events; }
    InspectionHighlighter* GetHighlighter() { return m_highlighter; }

private:
    void OnTreeSelectionChanged(wxCommandEvent& event);
    void OnHighlight(wxCommandEvent& event);
    void OnFindWidget(wxCommandEvent& event);
    void OnLayout(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    void SetupMenuBar();
    void SetupToolBar();
    void SetupAUI();

    wxAuiManager m_auiMgr;
    InspectionTree* m_tree;
    ObjectInfoPanel* m_info;
    MethodInvokerPanel* m_invoker;
    EventLoggerPanel* m_events;
    InspectionHighlighter* m_highlighter;

    wxDECLARE_EVENT_TABLE();
};

} // namespace wxInspector

#endif // WX_INSPECTOR_FRAME_H
