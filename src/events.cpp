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

// Core wxWindow events
static const EventTypeInfo s_windowEvents[] = {
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
static const size_t s_windowEventCount = sizeof(s_windowEvents) / sizeof(s_windowEvents[0]);

// Extra per-class events (beyond the generic window events above)
struct ExtraEventInfo {
    wxClassInfo* classInfo;
    const EventTypeInfo* events;
    size_t count;
};

static const EventTypeInfo s_textEvents[] = {
    { wxEVT_TEXT,        "Text",        "Command" },
    { wxEVT_TEXT_ENTER,  "TextEnter",   "Command" },
    { wxEVT_TEXT_MAXLEN, "TextMaxLen",  "Command" },
};

static const ExtraEventInfo s_extraEvents[] = {
    { wxCLASSINFO(wxTextCtrl), s_textEvents,
      sizeof(s_textEvents) / sizeof(s_textEvents[0]) },
};

// Helper: look up event info by event-type int. Returns true if found.
static bool FindEventInfo(int evtType, const char*& outName, const char*& outCategory)
{
    for (size_t i = 0; i < s_windowEventCount; i++) {
        if (s_windowEvents[i].eventType == evtType) {
            outName = s_windowEvents[i].name;
            outCategory = s_windowEvents[i].category;
            return true;
        }
    }
    for (auto& extra : s_extraEvents) {
        for (size_t i = 0; i < extra.count; i++) {
            if (extra.events[i].eventType == evtType) {
                outName = extra.events[i].name;
                outCategory = extra.events[i].category;
                return true;
            }
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

    // Always add window-level events
    for (size_t i = 0; i < s_windowEventCount; i++) {
        int idx = m_eventTypeList->Append(
            wxString(s_windowEvents[i].name) + wxS(" (") +
            s_windowEvents[i].category + wxS(")"));
        m_eventTypeList->Check(static_cast<unsigned int>(idx));
    }

    // Add control-specific extra events
    for (auto& extra : s_extraEvents) {
        if (classInfo->IsKindOf(extra.classInfo)) {
            for (size_t i = 0; i < extra.count; i++) {
                int idx = m_eventTypeList->Append(
                    wxString(extra.events[i].name) + wxS(" (") +
                    extra.events[i].category + wxS(")"));
                m_eventTypeList->Check(static_cast<unsigned int>(idx));
            }
        }
    }
}

void EventLoggerPanel::StartCapture()
{
    if (!m_targetWindow || m_isCapturing) return;

    ConnectEvents();
    m_isCapturing = true;
    m_startBtn->Disable();
    m_stopBtn->Enable();
}

void EventLoggerPanel::StopCapture()
{
    if (!m_isCapturing) return;

    DisconnectEvents();
    m_isCapturing = false;
    m_startBtn->Enable();
    m_stopBtn->Disable();
}

void EventLoggerPanel::ConnectEvents()
{
    if (!m_targetWindow) return;

    wxEvtHandler* handler = m_targetWindow->GetEventHandler();

    // Connect checked window-level events
    for (size_t i = 0; i < s_windowEventCount; i++) {
        if (m_eventTypeList->IsChecked(static_cast<unsigned int>(i))) {
            handler->Connect(s_windowEvents[i].eventType,
                (wxObjectEventFunction)&EventLoggerPanel::OnDispatchEvent,
                nullptr, this);
            m_boundEventTypes.push_back(s_windowEvents[i].eventType);
        }
    }

    // Connect checked extra events
    unsigned int baseIdx = static_cast<unsigned int>(s_windowEventCount);
    for (auto& extra : s_extraEvents) {
        if (m_targetWindow->GetClassInfo()->IsKindOf(extra.classInfo)) {
            for (size_t i = 0; i < extra.count; i++) {
                unsigned int idx = baseIdx + static_cast<unsigned int>(i);
                if (m_eventTypeList->IsChecked(idx)) {
                    handler->Connect(extra.events[i].eventType,
                        (wxObjectEventFunction)&EventLoggerPanel::OnDispatchEvent,
                        nullptr, this);
                    m_boundEventTypes.push_back(extra.events[i].eventType);
                }
            }
        }
        baseIdx += static_cast<unsigned int>(extra.count);
    }
}

void EventLoggerPanel::DisconnectEvents()
{
    if (!m_targetWindow) return;

    wxEvtHandler* handler = m_targetWindow->GetEventHandler();

    // Disconnect by exact event type + method pointer + event sink.
    // Passing nullptr for the function would match all handlers for each
    // event type, but we use the precise form to be safe.
    for (int evtType : m_boundEventTypes) {
        handler->Disconnect(wxID_ANY, wxID_ANY, evtType,
            (wxObjectEventFunction)&EventLoggerPanel::OnDispatchEvent,
            nullptr, this);
    }
    m_boundEventTypes.clear();
}

void EventLoggerPanel::OnDispatchEvent(wxEvent& event)
{
    const char* name = nullptr;
    const char* category = nullptr;
    if (FindEventInfo(event.GetEventType(), name, category)) {
        OnEvent(event, name, category);
    }
    // Unknown events fall through without Skip() here — they will propagate
    // naturally because Connect doesn't consume the event.
    // We still call Skip() inside OnEvent for the known ones.
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
