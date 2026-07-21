#ifndef WX_INSPECTOR_EVENTS_H
#define WX_INSPECTOR_EVENTS_H

#include <wx/panel.h>
#include <wx/checklst.h>
#include <wx/button.h>
#include <wx/dataview.h>
#include <wx/vector.h>
#include "wx/inspector/object.h"

namespace wxInspector {

struct EventLogEntry {
    wxString timestamp;
    wxString eventType;
    wxString eventData;
    wxString category;  // Mouse, Keyboard, Focus, Size, Command, Misc
};

class EventLoggerPanel : public wxPanel {
public:
    EventLoggerPanel(wxWindow* parent);

    void ShowObject(InspectableObject& obj);
    void StartCapture();
    void StopCapture();
    void Clear();

private:
    void OnStart(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);
    void OnClear(wxCommandEvent& event);

    void PopulateEventTypes(wxClassInfo* classInfo);
    void ConnectEvents();
    void DisconnectEvents();

    // Single dispatch handler for all captured events (called via Connect).
    void OnDispatchEvent(wxEvent& event);

    // Records one entry in the log.
    void OnEvent(wxEvent& event, const wxString& typeName,
                 const wxString& category);
    wxString FormatEventData(wxEvent& event, const wxString& typeName);

    wxCheckListBox* m_eventTypeList;
    wxButton* m_startBtn;
    wxButton* m_stopBtn;
    wxButton* m_clearBtn;
    wxDataViewListCtrl* m_logView;

    wxWindow* m_targetWindow;
    bool m_isCapturing;

    wxVector<EventLogEntry> m_entries;
    wxVector<int> m_boundEventTypes;  // event-type ints that are currently bound

    static const int MAX_ENTRIES = 500;
};

} // namespace wxInspector

#endif // WX_INSPECTOR_EVENTS_H
