#include "wx/inspector/highlighter.h"
#include <wx/window.h>
#include <wx/sizer.h>
#include <wx/dcscreen.h>
#include <wx/toplevel.h>
#include <wx/gbsizer.h>
#include <wx/wrapsizer.h>

namespace wxInspector {

static const int HIGHLIGHT_DURATION_MS = 3000;
static const int FLICKER_INTERVAL_MS = 500;
static const int FLICKER_COUNT = 6;

wxBEGIN_EVENT_TABLE(InspectionHighlighter, wxEvtHandler)
    EVT_TIMER(wxID_ANY, InspectionHighlighter::OnTimer)
wxEND_EVENT_TABLE()

InspectionHighlighter::InspectionHighlighter()
    : m_highlightedWindow(nullptr), m_flickerCount(0)
{
    m_timer.SetOwner(this);
}

void InspectionHighlighter::Highlight(InspectableObject& obj)
{
    ClearHighlight();

    switch (obj.GetKind()) {
    case InspectableObject::Kind::Window:
        if (obj.AsWindow()) {
            DrawWindowHighlight(obj.AsWindow());
        }
        break;
    case InspectableObject::Kind::Sizer:
        if (obj.AsSizer()) {
            wxSizer* sizer = obj.AsSizer();
            wxWindow* refWin = sizer->GetContainingWindow();
            if (refWin) {
                DrawSizerHighlight(sizer, refWin);
            }
        }
        break;
    case InspectableObject::Kind::SizerItem:
        if (obj.AsSizerItem()) {
            DrawSizerItemHighlight(obj.AsSizerItem(), nullptr);
        }
        break;
    }

    // DrawWindowHighlight manages its own timer for windows.
    // For sizer / sizer-item highlights we start a one-shot clear timer.
    if (obj.GetKind() != InspectableObject::Kind::Window) {
        m_timer.Start(HIGHLIGHT_DURATION_MS, true);
    }
}

void InspectionHighlighter::ClearHighlight()
{
    m_timer.Stop();
    m_highlightedWindow = nullptr;
    m_flickerCount = 0;
}

void InspectionHighlighter::DrawWindowHighlight(wxWindow* win)
{
    wxTopLevelWindow* tlw = wxDynamicCast(win, wxTopLevelWindow);
    if (tlw) {
        // Flicker effect for top-level windows
        m_highlightedWindow = tlw;
        m_flickerCount = 0;
        m_timer.Start(FLICKER_INTERVAL_MS);
    } else {
        // Green outline for non-TLW windows, drawn directly on screen DC
        wxScreenDC dc;
        dc.SetPen(wxPen(*wxGREEN, 3));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        wxRect rect = win->GetScreenRect();
        dc.DrawRectangle(rect);

        // Schedule clearing via one-shot timer
        m_timer.Start(HIGHLIGHT_DURATION_MS, true);
    }
}

void InspectionHighlighter::DrawSizerHighlight(wxSizer* sizer, wxWindow* relativeTo)
{
    wxScreenDC dc;

    // Green outline for sizer boundary
    dc.SetPen(wxPen(*wxGREEN, 2));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    wxSize sz = sizer->GetSize();
    wxPoint pos = sizer->GetPosition();
    wxPoint screenPos = relativeTo->ClientToScreen(pos);
    wxRect sizerRect(screenPos, sz);
    dc.DrawRectangle(sizerRect);

    // Draw each item with dark blue fill per sizer type
    if (wxBoxSizer* bs = wxDynamicCast(sizer, wxBoxSizer)) {
        wxSizerItemList& items = bs->GetChildren();
        for (size_t i = 0; i < items.GetCount(); i++) {
            wxSizerItem* item = items[i];
            if (!item) continue;
            wxSize itemSz = item->GetSize();
            wxPoint itemPos = item->GetPosition();
            wxPoint screenItemPos = relativeTo->ClientToScreen(itemPos);

            dc.SetBrush(wxBrush(wxColour(0, 0, 139, 64))); // dark blue with alpha
            dc.DrawRectangle(wxRect(screenItemPos, itemSz));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
        }
    } else if (wxGridSizer* gs = wxDynamicCast(sizer, wxGridSizer)) {
        wxSizerItemList& items = gs->GetChildren();
        for (size_t i = 0; i < items.GetCount(); i++) {
            wxSizerItem* item = items[i];
            if (!item) continue;
            wxPoint itemPos = item->GetPosition();
            wxSize itemSz = item->GetSize();
            wxPoint screenItemPos = relativeTo->ClientToScreen(itemPos);

            dc.SetBrush(wxBrush(wxColour(0, 0, 139, 64)));
            dc.DrawRectangle(wxRect(screenItemPos, itemSz));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
        }
    } else if (wxFlexGridSizer* fgs = wxDynamicCast(sizer, wxFlexGridSizer)) {
        wxSizerItemList& items = fgs->GetChildren();
        for (size_t i = 0; i < items.GetCount(); i++) {
            wxSizerItem* item = items[i];
            if (!item) continue;
            wxPoint itemPos = item->GetPosition();
            wxSize itemSz = item->GetSize();
            wxPoint screenItemPos = relativeTo->ClientToScreen(itemPos);

            dc.SetBrush(wxBrush(wxColour(0, 0, 139, 64)));
            dc.DrawRectangle(wxRect(screenItemPos, itemSz));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
        }
    } else if (wxWrapSizer* ws = wxDynamicCast(sizer, wxWrapSizer)) {
        wxSizerItemList& items = ws->GetChildren();
        for (size_t i = 0; i < items.GetCount(); i++) {
            wxSizerItem* item = items[i];
            if (!item) continue;
            wxPoint itemPos = item->GetPosition();
            wxSize itemSz = item->GetSize();
            wxPoint screenItemPos = relativeTo->ClientToScreen(itemPos);

            dc.SetBrush(wxBrush(wxColour(0, 0, 139, 64)));
            dc.DrawRectangle(wxRect(screenItemPos, itemSz));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
        }
    }
}

void InspectionHighlighter::DrawSizerItemHighlight(wxSizerItem* item, wxWindow* relativeTo)
{
    if (item->IsWindow()) {
        // Highlight the child window directly
        DrawWindowHighlight(item->GetWindow());
    } else if (item->IsSpacer()) {
        // Draw the spacer rectangle on screen DC
        wxRect rect = item->GetRect();
        wxScreenDC dc;
        dc.SetPen(wxPen(*wxGREEN, 2));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        if (relativeTo) {
            rect.SetPosition(relativeTo->ClientToScreen(rect.GetPosition()));
        }
        dc.DrawRectangle(rect);
    }
}

void InspectionHighlighter::OnTimer(wxTimerEvent&)
{
    if (m_highlightedWindow) {
        // Flicker: toggle visibility
        m_highlightedWindow->Show(m_flickerCount % 2 == 0);
        m_flickerCount++;
        if (m_flickerCount >= FLICKER_COUNT) {
            m_highlightedWindow->Show(true);
            m_highlightedWindow = nullptr;
            m_flickerCount = 0;
            m_timer.Stop();
        }
    } else {
        ClearHighlight();
    }
}

} // namespace wxInspector
