#include "wx/inspector/events.h"
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/datetime.h>
#include <wx/tokenzr.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>

namespace wxInspector {

enum {
    ID_EVENT_TYPE_LIST = wxID_HIGHEST + 300,
    ID_START_BTN,
    ID_STOP_BTN,
    ID_CLEAR_BTN,
    ID_LOG_VIEW
};

// --- Event type lookup table ---

struct EventTypeInfo {
    int eventType;
    const char* name;
    const char* category;
};

// --- Lazy-initialized event type tables ---
//
// Event type values (wxEVT_LEFT_DOWN etc.) are assigned at runtime by
// wxNewEventType(). File-scope static tables would capture them at static
// init time — before wxWidgets assigns them — resulting in all zeros.
// Function-local statics are initialized on first call, which happens
// after main(), when all wx event types are guaranteed to be valid.

static const EventTypeInfo* GetWindowEvents(size_t& count)
{
    static const EventTypeInfo table[] = {
        { wxEVT_LEFT_DOWN,           "LeftDown",           "Mouse" },
        { wxEVT_LEFT_UP,             "LeftUp",             "Mouse" },
        { wxEVT_LEFT_DCLICK,         "LeftDClick",         "Mouse" },
        { wxEVT_MIDDLE_DOWN,         "MiddleDown",         "Mouse" },
        { wxEVT_MIDDLE_UP,           "MiddleUp",           "Mouse" },
        { wxEVT_MIDDLE_DCLICK,       "MiddleDClick",       "Mouse" },
        { wxEVT_RIGHT_DOWN,          "RightDown",          "Mouse" },
        { wxEVT_RIGHT_UP,            "RightUp",            "Mouse" },
        { wxEVT_RIGHT_DCLICK,        "RightDClick",        "Mouse" },
        { wxEVT_MOTION,              "Motion",             "Mouse" },
        { wxEVT_ENTER_WINDOW,        "EnterWindow",        "Mouse" },
        { wxEVT_LEAVE_WINDOW,        "LeaveWindow",        "Mouse" },
        { wxEVT_MOUSEWHEEL,          "MouseWheel",         "Mouse" },
        { wxEVT_MOUSE_CAPTURE_LOST,  "MouseCaptureLost",   "Mouse" },
        { wxEVT_KEY_DOWN,            "KeyDown",            "Keyboard" },
        { wxEVT_KEY_UP,              "KeyUp",              "Keyboard" },
        { wxEVT_CHAR,                "Char",               "Keyboard" },
        { wxEVT_SET_FOCUS,           "SetFocus",           "Focus" },
        { wxEVT_KILL_FOCUS,          "KillFocus",          "Focus" },
        { wxEVT_SIZE,                "Size",               "Size" },
        { wxEVT_MOVE,                "Move",               "Size" },
        { wxEVT_PAINT,               "Paint",              "Misc" },
        { wxEVT_ERASE_BACKGROUND,    "EraseBackground",    "Misc" },
        { wxEVT_IDLE,                "Idle",               "Misc" },
        { wxEVT_ACTIVATE,            "Activate",           "Focus" },
        { wxEVT_SHOW,                "Show",               "Misc" },
        { wxEVT_BUTTON,              "Button",             "Command" },
        { wxEVT_CHECKBOX,            "CheckBox",           "Command" },
        { wxEVT_CHOICE,              "Choice",             "Command" },
        { wxEVT_TEXT,                "Text",               "Command" },
        { wxEVT_TEXT_ENTER,          "TextEnter",          "Command" },
        { wxEVT_COMBOBOX,            "ComboBox",           "Command" },
        { wxEVT_RADIOBUTTON,         "RadioButton",        "Command" },
        { wxEVT_SLIDER,              "Slider",             "Command" },
        { wxEVT_SPINCTRL,            "SpinCtrl",           "Command" },
        { wxEVT_SCROLL_TOP,          "ScrollTop",          "Command" },
        { wxEVT_SCROLL_BOTTOM,       "ScrollBottom",       "Command" },
        { wxEVT_SCROLL_LINEUP,       "ScrollLineUp",       "Command" },
        { wxEVT_SCROLL_LINEDOWN,     "ScrollLineDown",     "Command" },
        { wxEVT_SCROLL_PAGEUP,       "ScrollPageUp",       "Command" },
        { wxEVT_SCROLL_PAGEDOWN,     "ScrollPageDown",     "Command" },
        { wxEVT_SCROLL_THUMBTRACK,   "ScrollThumbTrack",   "Command" },
        { wxEVT_SCROLL_THUMBRELEASE, "ScrollThumbRelease", "Command" },
        { wxEVT_SCROLL_CHANGED,      "ScrollChanged",      "Command" },
    };
    count = sizeof(table) / sizeof(table[0]);
    return table;
}

static const EventTypeInfo* GetTextCtrlEvents(size_t& count)
{
    static const EventTypeInfo table[] = {
        { wxEVT_TEXT,        "Text",        "Command" },
        { wxEVT_TEXT_ENTER,  "TextEnter",   "Command" },
        { wxEVT_TEXT_MAXLEN, "TextMaxLen",  "Command" },
    };
    count = sizeof(table) / sizeof(table[0]);
    return table;
}

// Helper: look up event info by event-type int. Returns true if found.
static bool FindEventInfo(int evtType, const char*& outName, const char*& outCategory)
{
    size_t count;
    const EventTypeInfo* events;

    // Search window-level events
    events = GetWindowEvents(count);
    for (size_t i = 0; i < count; i++) {
        if (events[i].eventType == evtType) {
            outName = events[i].name;
            outCategory = events[i].category;
            return true;
        }
    }

    // Search extra per-class events
    events = GetTextCtrlEvents(count);
    for (size_t i = 0; i < count; i++) {
        if (events[i].eventType == evtType) {
            outName = events[i].name;
            outCategory = events[i].category;
            return true;
        }
    }

    return false;
}

EventLoggerPanel::EventLoggerPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY), m_targetWindow(nullptr), m_isCapturing(false)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // --- Control buttons ---
    wxBoxSizer* btnRow = new wxBoxSizer(wxHORIZONTAL);
    m_startBtn = new wxButton(this, ID_START_BTN, "Start Capture");
    m_stopBtn  = new wxButton(this, ID_STOP_BTN,  "Stop");
    m_clearBtn = new wxButton(this, ID_CLEAR_BTN, "Clear");
    m_stopBtn->Disable();
    btnRow->Add(m_startBtn, 0, wxRIGHT, 4);
    btnRow->Add(m_stopBtn,  0, wxRIGHT, 4);
    btnRow->Add(m_clearBtn, 0);
    mainSizer->Add(btnRow, 0, wxEXPAND | wxALL, 4);

    // --- Event type checklist ---
    wxStaticBox* typeBox = new wxStaticBox(this, wxID_ANY, "Event Types");
    wxStaticBoxSizer* typeSizer = new wxStaticBoxSizer(typeBox, wxVERTICAL);
    m_eventTypeList = new wxCheckListBox(this, ID_EVENT_TYPE_LIST);
    m_eventTypeList->SetMinSize(wxSize(-1, 100));
    typeSizer->Add(m_eventTypeList, 1, wxEXPAND);
    mainSizer->Add(typeSizer, 0, wxEXPAND | wxALL, 4);

    // --- Log view ---
    m_logView = new wxDataViewListCtrl(this, ID_LOG_VIEW);
    m_logView->AppendTextColumn("Timestamp", wxDATAVIEW_CELL_INERT, 140);
    m_logView->AppendTextColumn("Event",     wxDATAVIEW_CELL_INERT, 120);
    m_logView->AppendTextColumn("Data",      wxDATAVIEW_CELL_INERT, 300);
    mainSizer->Add(m_logView, 1, wxEXPAND | wxALL, 4);

    SetSizer(mainSizer);

    // Wire up the control buttons
    m_startBtn->Bind(wxEVT_BUTTON, &EventLoggerPanel::OnStart, this);
    m_stopBtn ->Bind(wxEVT_BUTTON, &EventLoggerPanel::OnStop,  this);
    m_clearBtn->Bind(wxEVT_BUTTON, &EventLoggerPanel::OnClear, this);
}

void EventLoggerPanel::ShowObject(InspectableObject& obj)
{
    StopCapture();
    m_targetWindow = obj.AsWindow();
    PopulateEventTypes(obj.GetObject()->GetClassInfo());
}

void EventLoggerPanel::PopulateEventTypes(wxClassInfo* classInfo)
{
    m_eventTypeList->Clear();
    if (!classInfo) return;

    size_t count;
    const EventTypeInfo* events;

    // Always add window-level events
    events = GetWindowEvents(count);
    for (size_t i = 0; i < count; i++) {
        int idx = m_eventTypeList->Append(
            wxString(events[i].name) + wxS(" (") +
            events[i].category + wxS(")"));
        m_eventTypeList->Check(static_cast<unsigned int>(idx));
    }

    // Add control-specific extra events
    if (classInfo->IsKindOf(wxCLASSINFO(wxTextCtrl))) {
        events = GetTextCtrlEvents(count);
        for (size_t i = 0; i < count; i++) {
            int idx = m_eventTypeList->Append(
                wxString(events[i].name) + wxS(" (") +
                events[i].category + wxS(")"));
            m_eventTypeList->Check(static_cast<unsigned int>(idx));
        }
    }
}

void EventLoggerPanel::StartCapture()
{
    if (!m_targetWindow || m_isCapturing) return;

    wxEvtHandler::AddFilter(this);
    m_isCapturing = true;
    m_startBtn->Disable();
    m_stopBtn->Enable();
}

void EventLoggerPanel::StopCapture()
{
    if (!m_isCapturing) return;

    wxEvtHandler::RemoveFilter(this);
    m_isCapturing = false;
    m_startBtn->Enable();
    m_stopBtn->Disable();
}

int EventLoggerPanel::FilterEvent(wxEvent& event)
{
    // Quick reject: ensure we have a target and are capturing
    if (!m_targetWindow)
        return Event_Skip;

    // Only capture events for the target window.
    // Note: GetEventObject() returns the window that ORIGINATED the event.
    // For mouse/keyboard events on the target itself this matches directly.
    // Command events from child controls have the child as the event object
    // and will not be captured here — this matches the original per-window
    // Bind behaviour which also only captures events dispatched through the
    // target window's own event handler.
    if (event.GetEventObject() != m_targetWindow)
        return Event_Skip;

    const char* name = nullptr;
    const char* category = nullptr;
    if (FindEventInfo(event.GetEventType(), name, category)) {
        OnEvent(event, name, category);
    }

    // Never consume the event — always let it propagate normally.
    return Event_Skip;
}

void EventLoggerPanel::OnEvent(wxEvent& event, const wxString& typeName,
                                const wxString& category)
{
    EventLogEntry entry;
    entry.timestamp = wxDateTime::Now().FormatISOTime();
    entry.eventType = typeName;
    entry.category   = category;
    entry.eventData  = FormatEventData(event, typeName);

    m_entries.push_back(entry);
    if (m_entries.size() > MAX_ENTRIES)
        m_entries.erase(m_entries.begin());

    // Update the visible log
    wxVector<wxVariant> rowData;
    rowData.push_back(wxVariant(entry.timestamp));
    rowData.push_back(wxVariant(entry.eventType));
    rowData.push_back(wxVariant(entry.eventData));
    m_logView->AppendItem(rowData);

    // Auto-scroll to the latest entry
    if (m_logView->GetItemCount() > 0) {
        wxDataViewItem lastItem =
            m_logView->RowToItem(m_logView->GetItemCount() - 1);
        if (lastItem.IsOk())
            m_logView->EnsureVisible(lastItem);
    }

    // Trim excess rows
    while (m_logView->GetItemCount() > MAX_ENTRIES)
        m_logView->DeleteItem(0);

    // Let normal handling proceed
    event.Skip();
}

wxString EventLoggerPanel::FormatEventData(wxEvent& event,
                                            const wxString& typeName)
{
    wxString data;
    int evtType = event.GetEventType();

    // -- Mouse events --
    if (evtType == wxEVT_LEFT_DOWN || evtType == wxEVT_LEFT_UP ||
        evtType == wxEVT_RIGHT_DOWN || evtType == wxEVT_RIGHT_UP ||
        evtType == wxEVT_MIDDLE_DOWN || evtType == wxEVT_MIDDLE_UP ||
        evtType == wxEVT_MOTION || evtType == wxEVT_LEFT_DCLICK ||
        evtType == wxEVT_RIGHT_DCLICK || evtType == wxEVT_MIDDLE_DCLICK) {
        wxMouseEvent& me = static_cast<wxMouseEvent&>(event);
        data = wxString::Format(wxS("Pos=(%d,%d)"), me.GetX(), me.GetY());
    }
    // -- Keyboard (raw) --
    else if (evtType == wxEVT_KEY_DOWN || evtType == wxEVT_KEY_UP) {
        wxKeyEvent& ke = static_cast<wxKeyEvent&>(event);
        data = wxString::Format(wxS("KeyCode=%d Modifiers=%d"),
                                ke.GetKeyCode(), ke.GetModifiers());
    }
    // -- Keyboard (character) --
    else if (evtType == wxEVT_CHAR) {
        wxKeyEvent& ke = static_cast<wxKeyEvent&>(event);
        wxChar uc = ke.GetUnicodeKey();
        if (uc != WXK_NONE && uc >= 32)
            data = wxString::Format(wxS("Char='%c' KeyCode=%d"),
                                    static_cast<char>(uc), ke.GetKeyCode());
        else
            data = wxString::Format(wxS("KeyCode=%d"), ke.GetKeyCode());
    }
    // -- Size / Move --
    else if (evtType == wxEVT_SIZE) {
        wxSizeEvent& se = static_cast<wxSizeEvent&>(event);
        data = wxString::Format(wxS("Size=(%d,%d)"),
                                se.GetSize().x, se.GetSize().y);
    }
    else if (evtType == wxEVT_MOVE) {
        wxMoveEvent& me = static_cast<wxMoveEvent&>(event);
        data = wxString::Format(wxS("Pos=(%d,%d)"),
                                me.GetPosition().x, me.GetPosition().y);
    }
    // -- Mouse wheel --
    else if (evtType == wxEVT_MOUSEWHEEL) {
        wxMouseEvent& mwe = static_cast<wxMouseEvent&>(event);
        data = wxString::Format(wxS("WheelRotation=%d Axis=%d"),
                                mwe.GetWheelRotation(), mwe.GetWheelAxis());
    }
    // -- Command events --
    else if (evtType == wxEVT_BUTTON ||
             evtType == wxEVT_CHECKBOX ||
             evtType == wxEVT_CHOICE ||
             evtType == wxEVT_COMBOBOX ||
             evtType == wxEVT_RADIOBUTTON ||
             evtType == wxEVT_SLIDER ||
             evtType == wxEVT_SPINCTRL ||
             evtType == wxEVT_TEXT ||
             evtType == wxEVT_TEXT_ENTER) {
        wxCommandEvent& ce = static_cast<wxCommandEvent&>(event);
        data = wxString::Format(wxS("ID=%d"), ce.GetId());
        if (!ce.GetString().IsEmpty())
            data += wxS(" String=\"") + ce.GetString() + wxS("\"");
        if (ce.GetInt() != 0)
            data += wxString::Format(wxS(" Int=%d"), ce.GetInt());
    }
    // -- Scroll events --
    else if (evtType == wxEVT_SCROLL_TOP ||
             evtType == wxEVT_SCROLL_BOTTOM ||
             evtType == wxEVT_SCROLL_LINEUP ||
             evtType == wxEVT_SCROLL_LINEDOWN ||
             evtType == wxEVT_SCROLL_PAGEUP ||
             evtType == wxEVT_SCROLL_PAGEDOWN ||
             evtType == wxEVT_SCROLL_THUMBTRACK ||
             evtType == wxEVT_SCROLL_THUMBRELEASE ||
             evtType == wxEVT_SCROLL_CHANGED) {
        wxScrollEvent& se = static_cast<wxScrollEvent&>(event);
        data = wxString::Format(wxS("Position=%d Orientation=%d"),
                                se.GetPosition(), se.GetOrientation());
    }
    // -- Idle / Paint --
    else if (evtType == wxEVT_IDLE) {
        data = wxS("Idle");
    }
    else if (evtType == wxEVT_PAINT) {
        data = wxS("Paint");
    }
    // -- Erase background --
    else if (evtType == wxEVT_ERASE_BACKGROUND) {
        data = wxS("EraseBackground");
    }
    // -- Mouse capture lost --
    else if (evtType == wxEVT_MOUSE_CAPTURE_LOST) {
        data = wxS("CaptureLost");
    }
    // -- Focus --
    else if (evtType == wxEVT_SET_FOCUS || evtType == wxEVT_KILL_FOCUS) {
        wxFocusEvent& fe = static_cast<wxFocusEvent&>(event);
        wxWindow* win = fe.GetWindow();
        data = wxString::Format(wxS("Window=%p"), static_cast<void*>(win));
    }
    // -- Activate / Show / Enter / Leave --
    else if (evtType == wxEVT_ACTIVATE || evtType == wxEVT_SHOW ||
             evtType == wxEVT_ENTER_WINDOW || evtType == wxEVT_LEAVE_WINDOW) {
        data = wxString::Format(wxS("ID=%d"), event.GetId());
    }
    // -- Fallback --
    else {
        data = wxString::Format(wxS("ID=%d"), event.GetId());
    }

    return data;
}

void EventLoggerPanel::Clear()
{
    m_entries.clear();
    m_logView->DeleteAllItems();
}

void EventLoggerPanel::OnStart(wxCommandEvent&) { StartCapture(); }
void EventLoggerPanel::OnStop(wxCommandEvent&)  { StopCapture(); }
void EventLoggerPanel::OnClear(wxCommandEvent&) { Clear(); }

} // namespace wxInspector
