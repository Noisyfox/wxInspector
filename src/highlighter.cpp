#include "wx/inspector/highlighter.h"
#include <wx/window.h>
#include <wx/sizer.h>
#include <wx/toplevel.h>

namespace wxInspector {

static const int HIGHLIGHT_DURATION_MS = 3000;
static const int FLICKER_INTERVAL_MS = 500;
static const int FLICKER_COUNT = 6;

wxBEGIN_EVENT_TABLE(InspectionHighlighter, wxEvtHandler)
    EVT_TIMER(wxID_ANY, InspectionHighlighter::OnTimer)
wxEND_EVENT_TABLE()

InspectionHighlighter::InspectionHighlighter()
    : m_overlay(new wxOverlay()),
      m_highlightedWindow(nullptr), m_flickerCount(0)
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
            wxSizerItem* item = obj.AsSizerItem();
            // Try to find a reference window for coordinate conversion
            wxWindow* refWin = nullptr;
            if (item->IsWindow()) {
                // Highlight the window directly
                DrawWindowHighlight(item->GetWindow());
                return;
            } else if (item->IsSizer()) {
                wxSizer* childSizer = item->GetSizer();
                refWin = childSizer->GetContainingWindow();
            }
            // For spacers: we need the containing window of the parent sizer.
            // Since wxSizerItem doesn't know its parent, use the sizer's
            // containing window from the item's own sizer if we can reach it.
            DrawSizerItemHighlight(item, refWin);
        }
        break;
    }

    // DrawWindowHighlight manages its own timer for TLW flicker.
    // For sizer / sizer-item highlights we start a one-shot clear timer.
    if (obj.GetKind() != InspectableObject::Kind::Window) {
        m_timer.Start(HIGHLIGHT_DURATION_MS, true);
    }
}

void InspectionHighlighter::ClearHighlight()
{
    m_timer.Stop();
    // On Wayland, wxOverlayImpl caches the GTK popup window and its
    // gtk_window_set_transient_for() parent after the first Init() call.
    // When the next highlight targets a widget in a different top-level
    // window, the cached popup's transient-for still points at the old
    // TLW, causing the overlay to appear on the wrong window. Destroy and
    // recreate the wxOverlay to force a fresh native popup with the
    // correct transient-for parent.
    m_overlay.reset(new wxOverlay());
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
        // Use wxOverlayDC for persistent highlighting on non-TLW windows.
        // Unlike bare wxScreenDC (whose drawing is immediately overwritten
        // by the window system), wxOverlayDC draws through a transparent
        // overlay window that stays on top until m_overlay.Reset().
        wxWindow* tlwParent = wxGetTopLevelParent(win);
        wxOverlayDC dc(*m_overlay, tlwParent);
        dc.Clear();
        dc.SetPen(wxPen(*wxGREEN, 3));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        wxPoint origin = tlwParent->ScreenToClient(win->GetScreenPosition());
        wxSize sz = win->GetSize();
        dc.DrawRectangle(origin.x, origin.y, sz.x - 1, sz.y - 1);

        // wxOverlayDC destructor commits the overlay; start clear timer
        m_timer.Start(HIGHLIGHT_DURATION_MS, true);
    }
}

void InspectionHighlighter::DrawSizerHighlight(wxSizer* sizer, wxWindow* relativeTo)
{
    // Use wxOverlayDC on the containing window — sizer positions are
    // already relative to the containing window's client area, so no
    // coordinate conversion is needed.
    wxOverlayDC dc(*m_overlay, relativeTo);
	dc.Clear();

    // Green outline for sizer boundary
    dc.SetPen(wxPen(*wxGREEN, 2));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    wxSize sz = sizer->GetSize();
    wxPoint pos = sizer->GetPosition();
    dc.DrawRectangle(pos.x, pos.y, sz.x, sz.y);

    // Dark blue fill for each child item
    dc.SetPen(wxNullPen);
    dc.SetBrush(wxBrush(wxColour(0, 0, 139, 64)));

    wxSizerItemList& items = sizer->GetChildren();
    for (size_t i = 0; i < items.GetCount(); i++) {
        wxSizerItem* item = items[i];
        if (!item) continue;
        wxPoint itemPos = item->GetPosition();
        wxSize itemSz = item->GetSize();
        dc.DrawRectangle(itemPos.x, itemPos.y, itemSz.x, itemSz.y);
    }
    // wxOverlayDC destructor commits the overlay
}

void InspectionHighlighter::DrawSizerItemHighlight(wxSizerItem* item, wxWindow* relativeTo)
{
    if (item->IsWindow()) {
        // Delegate to window highlighting
        DrawWindowHighlight(item->GetWindow());
    } else if (item->IsSpacer()) {
        // Draw the spacer rectangle using overlay for persistence
        if (!relativeTo) return;

        wxRect rect = item->GetRect();
        wxOverlayDC dc(*m_overlay, relativeTo);
        dc.Clear();
        dc.SetPen(wxPen(*wxGREEN, 2));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(rect);
        // wxOverlayDC destructor commits the overlay
    } else if (item->IsSizer()) {
        // Highlight the sizer item's own rectangle, and let the child
        // sizer's contents be drawn as part of the parent sizer highlight
        // (drawn in DrawSizerHighlight) when the sizer itself is selected.
        if (!relativeTo) return;

        wxRect rect = item->GetRect();
        wxOverlayDC dc(*m_overlay, relativeTo);
        dc.Clear();
        dc.SetPen(wxPen(*wxGREEN, 2));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
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
