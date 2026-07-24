#ifndef WX_INSPECTOR_HIGHLIGHTER_H
#define WX_INSPECTOR_HIGHLIGHTER_H

#include <wx/object.h>
#include <wx/timer.h>
#include <wx/overlay.h>
#include <memory>
#include "wx/inspector/object.h"

namespace wxInspector {

class InspectionHighlighter : public wxEvtHandler {
public:
    InspectionHighlighter();

    void Highlight(InspectableObject& obj);
    void ClearHighlight();

private:
    void OnTimer(wxTimerEvent& event);
    void DrawWindowHighlight(wxWindow* win);
    void DrawSizerHighlight(wxSizer* sizer, wxWindow* relativeTo);
    void DrawSizerItemHighlight(wxSizerItem* item, wxWindow* relativeTo);

    std::unique_ptr<wxOverlay> m_overlay;
    wxTimer m_timer;
    wxWindow* m_highlightedWindow; // for TLW flicker
    int m_flickerCount;            // flicker toggle counter

    wxDECLARE_EVENT_TABLE();
};

} // namespace wxInspector

#endif // WX_INSPECTOR_HIGHLIGHTER_H
