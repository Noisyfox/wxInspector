#ifndef WX_INSPECTOR_EVENTS_H
#define WX_INSPECTOR_EVENTS_H

#include <wx/panel.h>
#include <wx/checklst.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dataview.h>
#include <wx/vector.h>
#include <wx/eventfilter.h>
#include <map>
#include "wx/inspector/object.h"

namespace wxInspector {

struct EventLogEntry {
    wxString timestamp;
    wxString eventType;
    wxString eventData;
    wxString category;  // Mouse, Keyboard, Focus, Size, Command, Misc
};

class EventLoggerPanel : public wxPanel, public wxEventFilter {
public:
    EventLoggerPanel(wxWindow* parent);
    ~EventLoggerPanel();

    void ShowObject(InspectableObject& obj);
    void StartCapture();
    void StopCapture();
    void Clear();

private:
    void OnStart(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);
    void OnClear(wxCommandEvent& event);

    void PopulateEventTypes(wxClassInfo* classInfo);

    // wxEventFilter override — intercepts all events globally.
    int FilterEvent(wxEvent& event) override;

    // Records one entry in the log.
    void OnEvent(wxEvent& event, const wxString& typeName,
                 const wxString& category);
    wxString FormatEventData(wxEvent& event, const wxString& typeName);

    wxCheckListBox* m_eventTypeList;
    wxCheckBox* m_captureChildrenChk;
    wxButton* m_startBtn;
    wxButton* m_stopBtn;
    wxButton* m_clearBtn;
    wxDataViewListCtrl* m_logView;

    wxWindow* m_targetWindow;
    bool m_isCapturing;

    wxVector<EventLogEntry> m_entries;

    std::map<int, int> m_eventTypeToIndex;

    static const int MAX_ENTRIES = 500;
};

} // namespace wxInspector

#endif // WX_INSPECTOR_EVENTS_H
