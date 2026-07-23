#include "wx/inspector/tree.h"
#include <wx/toplevel.h>
#include <wx/sizer.h>
#include <wx/artprov.h>
#include <wx/utils.h>
#include <wx/menu.h>

#ifdef __WXGTK3__
#include <gdk/gdk.h>
#include <gtk/gtk.h>

namespace {

// Wrapper that adapts gtk_main_do_event (which takes only GdkEvent*) to
// GdkEventFunc (which takes GdkEvent* + gpointer).
void wxInspectorGtkMainDoEvent(GdkEvent* event, gpointer /* data */)
{
    gtk_main_do_event(event);
}

// Recursively search the wxWindow tree for a window whose GTK handle
// matches the given GtkWidget. Returns nullptr if no match found.
wxWindow* wxInspectorFindWxWindowByGtkWidget(wxWindow* win, GtkWidget* target)
{
    if (win->GetHandle() == target)
        return win;

    const wxWindowList& children = win->GetChildren();
    for (size_t i = 0; i < children.GetCount(); i++)
    {
        wxWindow* found = wxInspectorFindWxWindowByGtkWidget(children[i], target);
        if (found) return found;
    }
    return nullptr;
}

// Find the deepest wxWindow child at the given (x,y) coordinates relative
// to the parent window. Returns the child itself if no deeper child matches.
wxWindow* wxInspectorFindDeepestChildAt(wxWindow* parent, int x, int y)
{
    // Search children in reverse order — last child is topmost on screen
    const wxWindowList& children = parent->GetChildren();
    for (int i = children.GetCount() - 1; i >= 0; i--)
    {
        wxWindow* child = children[i];
        if (!child->IsShown()) continue;

        wxPoint pos = child->GetPosition();
        wxSize  sz  = child->GetSize();

        if (x >= pos.x && y >= pos.y &&
            x < pos.x + sz.x && y < pos.y + sz.y)
        {
            // Convert to child-local coordinates and recurse
            wxWindow* deeper = wxInspectorFindDeepestChildAt(
                child, x - pos.x, y - pos.y);
            return deeper ? deeper : child;
        }
    }
    return nullptr;
}

void wxInspectorGdkEventHandler(GdkEvent* gdkEvent, gpointer data)
{
    wxInspector::InspectionTree* tree =
        static_cast<wxInspector::InspectionTree*>(data);

    if (gdkEvent->type == GDK_BUTTON_PRESS && tree->IsFindWidgetCapture())
    {
        GdkWindow* gdkWin = gdkEvent->button.window;
        wxWindow* baseWin = nullptr;
        GtkWidget* eventWidget = nullptr;

        if (gdkWin)
        {
            // Get the GtkWidget associated with the clicked GdkWindow
            gpointer userData = nullptr;
            gdk_window_get_user_data(gdkWin, &userData);
            if (userData && GTK_IS_WIDGET(userData))
                eventWidget = GTK_WIDGET(userData);

            // Walk up the GtkWidget parent chain. For composite controls
            // (wxComboBox, etc.), the click lands on an internal child
            // (e.g. the toggle button), so we walk up to find the ancestor
            // GtkWidget corresponding to a wxWindow.
            GtkWidget* gtkWidget = eventWidget;
            while (gtkWidget && !baseWin)
            {
                for (wxWindowList::iterator it = wxTopLevelWindows.begin();
                     it != wxTopLevelWindows.end(); ++it)
                {
                    baseWin = wxInspectorFindWxWindowByGtkWidget(*it, gtkWidget);
                    if (baseWin) break;
                }
                gtkWidget = gtk_widget_get_parent(gtkWidget);
            }
        }

        if (baseWin)
        {
            // baseWin is the lowest clickable wxWindow ancestor.  Now walk
            // back DOWN the wxWindow tree to find the deepest child at the
            // click position.  This is needed for controls like wxStaticText
            // that don't have their own GdkWindow and draw on their parent's
            // surface — they won't be found by the GtkWidget walk alone.
            wxWindow* deepest = nullptr;

            if (eventWidget)
            {
                // Translate event coordinates from the event's GtkWidget
                // space to the base wxWindow's GtkWidget space, avoiding
                // any screen-coordinate dependency.
                GtkWidget* baseWidget = GTK_WIDGET(baseWin->GetHandle());
                gint outX = (gint)gdkEvent->button.x;
                gint outY = (gint)gdkEvent->button.y;

                bool coordsValid = (eventWidget == baseWidget);
                if (!coordsValid)
                {
                    coordsValid = gtk_widget_translate_coordinates(
                        eventWidget, baseWidget,
                        (gint)gdkEvent->button.x, (gint)gdkEvent->button.y,
                        &outX, &outY);
                }

                if (coordsValid)
                {
                    deepest = wxInspectorFindDeepestChildAt(
                        baseWin, (int)outX, (int)outY);
                }
            }

            if (deepest)
                tree->SelectObject(deepest);
            else
                tree->SelectObject(baseWin);
        }
        else
            wxBell();

        tree->EndFindWidget();
        return; // Swallow event so GTK native widgets don't process it
    }

    // Forward unhandled events to the default GTK event handler
    wxInspectorGtkMainDoEvent(gdkEvent, nullptr);
}

} // anonymous namespace
#endif // __WXGTK3__

namespace wxInspector {

wxDEFINE_EVENT(wxEVT_INSPECT_TREE_SEL_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_INSPECT_TREE_HIGHLIGHT, wxCommandEvent);

// TreeItemData subclass to store the wxObject* properly
class ObjectTreeItemData : public wxTreeItemData {
public:
    wxObject* m_object;
    InspectableObject::Kind m_kind;
    ObjectTreeItemData(wxObject* obj, InspectableObject::Kind kind)
        : m_object(obj), m_kind(kind) {}
};

// IDs
enum {
    ID_TREE_REFRESH = wxID_HIGHEST + 100,
    ID_TREE_TOGGLE_SIZERS,
    ID_TREE_EXPAND_ALL,
    ID_TREE_COLLAPSE_ALL,
    ID_TREE_FIND_WIDGET,
    ID_TREE_HIGHLIGHT,
    ID_TREE_LAYOUT,
    ID_TREE_LAYOUT_PARENT
};

wxBEGIN_EVENT_TABLE(InspectionTree, wxPanel)
    EVT_TREE_SEL_CHANGED(wxID_ANY, InspectionTree::OnTreeSelChanged)
    EVT_TREE_ITEM_MENU(wxID_ANY, InspectionTree::OnContextMenu)
wxEND_EVENT_TABLE()

InspectionTree::InspectionTree(wxWindow* parent)
    : wxPanel(parent, wxID_ANY), m_showSizers(false), m_findWidgetCapture(false),
      m_findWidgetFilter(this)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    m_toolbar = new wxToolBar(this, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxTB_FLAT | wxTB_NODIVIDER);
    m_toolbar->AddTool(ID_TREE_REFRESH, "Refresh",
        wxArtProvider::GetBitmap(wxART_REDO), "Refresh (F1)");
    m_toolbar->AddTool(ID_TREE_TOGGLE_SIZERS, "Sizers",
        wxArtProvider::GetBitmap(wxART_FIND), "Toggle Sizers (F3)",
        wxITEM_CHECK);
    m_toolbar->AddTool(ID_TREE_EXPAND_ALL, "Expand All",
        wxArtProvider::GetBitmap(wxART_PLUS), "Expand All (F4)");
    m_toolbar->AddTool(ID_TREE_COLLAPSE_ALL, "Collapse All",
        wxArtProvider::GetBitmap(wxART_MINUS), "Collapse All (F5)");
    m_toolbar->AddTool(ID_TREE_FIND_WIDGET, "Find",
        wxArtProvider::GetBitmap(wxART_CROSS_MARK), "Find Widget (F2)");
    m_toolbar->AddTool(ID_TREE_HIGHLIGHT, "Highlight",
        wxArtProvider::GetBitmap(wxART_TIP), "Highlight (F6)");
    m_toolbar->Realize();
    mainSizer->Add(m_toolbar, 0, wxEXPAND);

    m_tree = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT);
    m_tree->AddRoot("(root)");
    mainSizer->Add(m_tree, 1, wxEXPAND);

    SetSizer(mainSizer);

    Bind(wxEVT_TOOL, &InspectionTree::OnRefresh, this, ID_TREE_REFRESH);
    Bind(wxEVT_TOOL, &InspectionTree::OnToggleSizers, this, ID_TREE_TOGGLE_SIZERS);
    Bind(wxEVT_TOOL, &InspectionTree::OnExpandAll, this, ID_TREE_EXPAND_ALL);
    Bind(wxEVT_TOOL, &InspectionTree::OnCollapseAll, this, ID_TREE_COLLAPSE_ALL);
    Bind(wxEVT_TOOL, &InspectionTree::OnFindWidget, this, ID_TREE_FIND_WIDGET);
    Bind(wxEVT_TOOL, &InspectionTree::OnHighlight, this, ID_TREE_HIGHLIGHT);
    Bind(wxEVT_MENU, &InspectionTree::OnLayout, this, ID_TREE_LAYOUT);
    Bind(wxEVT_MENU, &InspectionTree::OnLayoutParent, this, ID_TREE_LAYOUT_PARENT);
    m_tree->Bind(wxEVT_KEY_DOWN, &InspectionTree::OnKeyDown, this);

    RebuildTree();
}

InspectionTree::~InspectionTree()
{
    if (m_findWidgetCapture)
    {
#ifdef __WXGTK3__
        gdk_event_handler_set(wxInspectorGtkMainDoEvent, nullptr, nullptr);
#else
        wxEvtHandler::RemoveFilter(&m_findWidgetFilter);
#endif
    }
}

void InspectionTree::RebuildTree()
{
    wxTreeItemId selectedId = m_tree->GetSelection();
    wxObject* previouslySelected = nullptr;
    if (selectedId.IsOk()) {
        ObjectTreeItemData* d = dynamic_cast<ObjectTreeItemData*>(
            m_tree->GetItemData(selectedId));
        if (d) previouslySelected = d->m_object;
    }

    m_tree->DeleteAllItems();
    wxTreeItemId root = m_tree->AddRoot("(root)");

    wxWindowList& topWindows = wxTopLevelWindows;
    for (size_t n = 0; n < topWindows.GetCount(); n++) {
        wxWindow* win = topWindows[n];
        if (!win) continue;
        wxTreeItemId nodeId = DoAddWindow(root, win);

        if (m_showSizers) {
            wxSizer* sz = win->GetSizer();
            if (sz) {
                wxTreeItemId sizerNode = DoAddSizer(nodeId, sz);
                AddSizerItems(sizerNode, sz);
            }
        }

        AddChildren(nodeId, win);
    }

    if (!m_tree->GetChildrenCount(root, false))
        m_tree->AppendItem(root, "No top-level windows");

    m_tree->Expand(root);

    if (previouslySelected)
        SelectObject(previouslySelected);
}

void InspectionTree::AddChildren(wxTreeItemId parentId, wxWindow* window)
{
    const wxWindowList& children = window->GetChildren();
    for (size_t n = 0; n < children.GetCount(); n++) {
        wxWindow* child = children[n];
        if (!child) continue;
        wxTreeItemId nodeId = DoAddWindow(parentId, child);

        if (m_showSizers) {
            wxSizer* sz = child->GetSizer();
            if (sz) {
                wxTreeItemId sizerNode = DoAddSizer(nodeId, sz);
                AddSizerItems(sizerNode, sz);
            }
        }

        AddChildren(nodeId, child);
    }
}

void InspectionTree::AddSizerItems(wxTreeItemId sizerNodeId, wxSizer* sizer)
{
    wxSizerItemList& items = sizer->GetChildren();
    for (size_t n = 0; n < items.GetCount(); n++) {
        wxSizerItem* item = items[n];
        if (!item) continue;

        wxTreeItemId itemNode = DoAddSizerItem(sizerNodeId, item);

        if (item->IsWindow()) {
            wxWindow* win = item->GetWindow();
            if (m_showSizers) {
                wxSizer* childSz = win->GetSizer();
                if (childSz) {
                    wxTreeItemId csNode = DoAddSizer(itemNode, childSz);
                    AddSizerItems(csNode, childSz);
                }
            }
            AddChildren(itemNode, win);
        } else if (item->IsSizer()) {
            AddSizerItems(itemNode, item->GetSizer());
        }
    }
}

wxTreeItemId InspectionTree::DoAddWindow(wxTreeItemId parentId, wxWindow* window)
{
    InspectableObject obj = InspectableObject::FromWindow(window);
    wxTreeItemId id = m_tree->AppendItem(parentId, obj.GetDisplayName());
    m_tree->SetItemData(id,
        new ObjectTreeItemData(window, InspectableObject::Kind::Window));
    return id;
}

wxTreeItemId InspectionTree::DoAddSizer(wxTreeItemId parentId, wxSizer* sizer)
{
    InspectableObject obj = InspectableObject::FromSizer(sizer);
    wxTreeItemId id = m_tree->AppendItem(parentId, obj.GetDisplayName());
    m_tree->SetItemTextColour(id, *wxBLUE);
    m_tree->SetItemData(id,
        new ObjectTreeItemData(sizer, InspectableObject::Kind::Sizer));
    return id;
}

wxTreeItemId InspectionTree::DoAddSizerItem(wxTreeItemId parentId, wxSizerItem* item)
{
    InspectableObject obj = InspectableObject::FromSizerItem(item);
    wxTreeItemId id = m_tree->AppendItem(parentId, obj.GetDisplayName());
    m_tree->SetItemTextColour(id, *wxBLUE);
    m_tree->SetItemData(id,
        new ObjectTreeItemData(item, InspectableObject::Kind::SizerItem));
    return id;
}

void InspectionTree::ToggleSizers()
{
    m_showSizers = !m_showSizers;
    m_toolbar->ToggleTool(ID_TREE_TOGGLE_SIZERS, m_showSizers);
    RebuildTree();
}

void InspectionTree::ExpandAll()
{
    ExpandAllChildren(m_tree->GetRootItem());
}

void InspectionTree::CollapseAll()
{
    CollapseAllChildren(m_tree->GetRootItem());
}

void InspectionTree::ExpandAllChildren(wxTreeItemId item)
{
    m_tree->Expand(item);
    wxTreeItemIdValue cookie;
    wxTreeItemId child = m_tree->GetFirstChild(item, cookie);
    while (child.IsOk()) {
        ExpandAllChildren(child);
        child = m_tree->GetNextChild(item, cookie);
    }
}

void InspectionTree::CollapseAllChildren(wxTreeItemId item)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId child = m_tree->GetFirstChild(item, cookie);
    while (child.IsOk()) {
        CollapseAllChildren(child);
        child = m_tree->GetNextChild(item, cookie);
    }
    if (item != m_tree->GetRootItem())
        m_tree->Collapse(item);
}

void InspectionTree::SelectObject(wxObject* obj)
{
    wxTreeItemId found = FindItemForObject(m_tree->GetRootItem(), obj);
    if (found.IsOk()) {
        m_tree->SelectItem(found);
        m_tree->EnsureVisible(found);
    }
}

InspectableObject InspectionTree::GetSelectedObject() const
{
    wxTreeItemId sel = m_tree->GetSelection();
    if (!sel.IsOk())
        return InspectableObject::FromWindow(nullptr);

    ObjectTreeItemData* d = dynamic_cast<ObjectTreeItemData*>(
        m_tree->GetItemData(sel));
    if (!d || !d->m_object)
        return InspectableObject::FromWindow(nullptr);

    switch (d->m_kind) {
    case InspectableObject::Kind::Window:
        return InspectableObject::FromWindow(
            static_cast<wxWindow*>(d->m_object));
    case InspectableObject::Kind::Sizer:
        return InspectableObject::FromSizer(
            static_cast<wxSizer*>(d->m_object));
    case InspectableObject::Kind::SizerItem:
        return InspectableObject::FromSizerItem(
            static_cast<wxSizerItem*>(d->m_object));
    }
    return InspectableObject::FromWindow(nullptr);
}

wxWindow* InspectionTree::GetContainerWindow() const
{
    wxTreeItemId item = m_tree->GetSelection();
    if (!item.IsOk())
        return nullptr;

    // Walk up from the selected item to find the nearest window ancestor
    while (item.IsOk())
    {
        ObjectTreeItemData* d = dynamic_cast<ObjectTreeItemData*>(
            m_tree->GetItemData(item));
        if (d && d->m_object && d->m_kind == InspectableObject::Kind::Window)
        {
            return static_cast<wxWindow*>(d->m_object);
        }
        item = m_tree->GetItemParent(item);
    }
    return nullptr;
}

wxTreeItemId InspectionTree::FindItemForObject(wxTreeItemId parent, wxObject* target)
{
    wxTreeItemIdValue cookie;
    wxTreeItemId child = m_tree->GetFirstChild(parent, cookie);
    while (child.IsOk()) {
        ObjectTreeItemData* d = dynamic_cast<ObjectTreeItemData*>(
            m_tree->GetItemData(child));
        if (d && d->m_object == target)
            return child;
        wxTreeItemId found = FindItemForObject(child, target);
        if (found.IsOk()) return found;
        child = m_tree->GetNextChild(parent, cookie);
    }
    return wxTreeItemId();
}

void InspectionTree::FindWidget()
{
    if (m_findWidgetCapture) return;

    m_findWidgetCapture = true;
#ifdef __WXGTK3__
    // Intercept GDK events before GTK native widgets consume them.
    // Otherwise composite widgets like wxComboBox handle button presses
    // internally and never generate wxEVT_LEFT_DOWN for the event filter.
    //
    // Note: gdk_event_handler_set() returns void, so we cannot save and
    // restore the previous handler.  We rely on the fact that GTK always
    // installs gtk_main_do_event during gtk_init() and no other code in
    // a wxWidgets application replaces it.
    gdk_event_handler_set(wxInspectorGdkEventHandler, this, nullptr);
#else
    wxEvtHandler::AddFilter(&m_findWidgetFilter);
#endif
    SetCursor(wxCURSOR_CROSS);
}

void InspectionTree::EndFindWidget()
{
    if (!m_findWidgetCapture) return;
    m_findWidgetCapture = false;
#ifdef __WXGTK3__
    // Restore the default GDK event handler
    gdk_event_handler_set(wxInspectorGtkMainDoEvent, nullptr, nullptr);
#else
    wxEvtHandler::RemoveFilter(&m_findWidgetFilter);
#endif
    SetCursor(wxNullCursor);
}

int InspectionTree::FindWidgetEventFilter::FilterEvent(wxEvent& event)
{
    if (event.GetEventType() == wxEVT_LEFT_DOWN && m_tree->m_findWidgetCapture)
    {
        wxWindow* win = dynamic_cast<wxWindow*>(event.GetEventObject());
        if (win)
        {
            // For composite controls (wxComboBox, wxSpinCtrl, etc.), the
            // click may land on an internal child window that is not in the
            // inspection tree. Walk up the parent chain until we find a
            // window that is registered in the tree, so the composite
            // control itself is selected rather than its internal child.
            wxWindow* target = win;
            while (target && !m_tree->FindItemForObject(
                        m_tree->m_tree->GetRootItem(), target).IsOk())
            {
                target = target->GetParent();
            }
            if (target)
                m_tree->SelectObject(target);
            else
                wxBell();
        }
        else
            wxBell();
        m_tree->EndFindWidget();
        return wxEventFilter::Event_Processed;
    }
    return wxEventFilter::Event_Skip;
}

// --- Event handlers ---

void InspectionTree::OnTreeSelChanged(wxTreeEvent& event)
{
    wxTreeItemId id = event.GetItem();
    if (!id.IsOk()) return;

    ObjectTreeItemData* d = dynamic_cast<ObjectTreeItemData*>(
        m_tree->GetItemData(id));
    if (!d) return;

    wxCommandEvent selEvent(wxEVT_INSPECT_TREE_SEL_CHANGED, GetId());
    selEvent.SetClientData(d->m_object);
    selEvent.SetEventObject(this);
    ProcessWindowEvent(selEvent);
}

void InspectionTree::OnRefresh(wxCommandEvent&) { RebuildTree(); }
void InspectionTree::OnToggleSizers(wxCommandEvent&) { ToggleSizers(); }
void InspectionTree::OnExpandAll(wxCommandEvent&) { ExpandAll(); }
void InspectionTree::OnCollapseAll(wxCommandEvent&) { CollapseAll(); }
void InspectionTree::OnFindWidget(wxCommandEvent&) { FindWidget(); }
void InspectionTree::OnHighlight(wxCommandEvent&)
{
    wxCommandEvent evt(wxEVT_INSPECT_TREE_HIGHLIGHT, GetId());
    evt.SetEventObject(this);
    ProcessWindowEvent(evt);
}

void InspectionTree::OnKeyDown(wxKeyEvent& event)
{
    switch (event.GetKeyCode()) {
    case WXK_F1: RebuildTree(); return;
    case WXK_F2: FindWidget(); return;
    case WXK_F3: ToggleSizers(); return;
    case WXK_F4: ExpandAll(); return;
    case WXK_F5: CollapseAll(); return;
    case WXK_F6: {
        wxCommandEvent evt(wxEVT_INSPECT_TREE_HIGHLIGHT, GetId());
        evt.SetEventObject(this);
        ProcessWindowEvent(evt);
        return;
    }
    case WXK_ESCAPE: EndFindWidget(); return;
    default: break;
    }
    event.Skip();
}

void InspectionTree::OnContextMenu(wxTreeEvent& event)
{
    wxTreeItemId item = event.GetItem();
    if (!item.IsOk() || item == m_tree->GetRootItem())
        return;

    // Select the right-clicked item
    m_tree->SelectItem(item);

    ObjectTreeItemData* d = dynamic_cast<ObjectTreeItemData*>(
        m_tree->GetItemData(item));
    if (!d || !d->m_object)
        return;

    bool isWindow = (d->m_kind == InspectableObject::Kind::Window);

    wxMenu menu;

    menu.Append(ID_TREE_LAYOUT, "Layout");

    if (isWindow)
    {
        wxWindow* container = GetContainerWindow();
        wxWindow* parent = container ? container->GetParent() : nullptr;
        wxMenuItem* layoutParentItem = menu.Append(ID_TREE_LAYOUT_PARENT, "Layout Parent");
        if (!parent)
            layoutParentItem->Enable(false);
    }

    PopupMenu(&menu);
}

void InspectionTree::OnLayout(wxCommandEvent&)
{
    wxWindow* win = GetContainerWindow();
    if (win)
        win->Layout();
}

void InspectionTree::OnLayoutParent(wxCommandEvent&)
{
    wxWindow* win = GetContainerWindow();
    if (win)
    {
        wxWindow* parent = win->GetParent();
        if (parent)
            parent->Layout();
    }
}

} // namespace wxInspector
