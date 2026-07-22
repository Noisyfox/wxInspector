#include "wx/inspector/tree.h"
#include <wx/toplevel.h>
#include <wx/sizer.h>
#include <wx/artprov.h>
#include <wx/utils.h>

namespace wxInspector {

wxDEFINE_EVENT(wxEVT_INSPECT_TREE_SEL_CHANGED, wxCommandEvent);

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
    ID_TREE_FIND_WIDGET
};

wxBEGIN_EVENT_TABLE(InspectionTree, wxPanel)
    EVT_TREE_SEL_CHANGED(wxID_ANY, InspectionTree::OnTreeSelChanged)
wxEND_EVENT_TABLE()

InspectionTree::InspectionTree(wxWindow* parent)
    : wxPanel(parent, wxID_ANY), m_showSizers(false), m_findWidgetCapture(false)
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
    m_tree->Bind(wxEVT_KEY_DOWN, &InspectionTree::OnKeyDown, this);

    Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& evt) {
        if (!m_findWidgetCapture) {
            evt.Skip();
            return;
        }
        ReleaseMouse();
        SetCursor(wxNullCursor);
        m_findWidgetCapture = false;
        wxPoint pt = ::wxGetMousePosition();
        wxWindow* win = wxFindWindowAtPointer(pt);
        if (win) SelectObject(win);
        else wxBell();
    }, wxID_ANY, wxID_ANY);

    RebuildTree();
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
    CaptureMouse();
    SetCursor(wxCURSOR_CROSS);
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

void InspectionTree::OnKeyDown(wxKeyEvent& event)
{
    switch (event.GetKeyCode()) {
    case WXK_F1: RebuildTree(); return;
    case WXK_F2: FindWidget(); return;
    case WXK_F3: ToggleSizers(); return;
    case WXK_F4: ExpandAll(); return;
    case WXK_F5: CollapseAll(); return;
    default: break;
    }
    event.Skip();
}

} // namespace wxInspector
