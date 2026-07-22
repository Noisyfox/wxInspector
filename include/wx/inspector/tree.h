#ifndef WX_INSPECTOR_TREE_H
#define WX_INSPECTOR_TREE_H

#include <wx/panel.h>
#include <wx/treectrl.h>
#include <wx/toolbar.h>
#include "wx/inspector/object.h"

namespace wxInspector {

wxDECLARE_EVENT(wxEVT_INSPECT_TREE_SEL_CHANGED, wxCommandEvent);

class InspectionTree : public wxPanel {
public:
    InspectionTree(wxWindow* parent);

    void RebuildTree();
    void ToggleSizers();
    void ExpandAll();
    void CollapseAll();
    void SelectObject(wxObject* obj);
    void FindWidget();
    InspectableObject GetSelectedObject() const;
    wxWindow* GetContainerWindow() const;
    wxTreeCtrl* GetTreeCtrl() { return m_tree; }

    bool IsSizerModeEnabled() const { return m_showSizers; }

private:
    void OnTreeSelChanged(wxTreeEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnToggleSizers(wxCommandEvent& event);
    void OnExpandAll(wxCommandEvent& event);
    void OnCollapseAll(wxCommandEvent& event);
    void OnFindWidget(wxCommandEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnContextMenu(wxTreeEvent& event);
    void OnLayout(wxCommandEvent& event);
    void OnLayoutParent(wxCommandEvent& event);

    void AddChildren(wxTreeItemId parentId, wxWindow* window);
    void AddSizerItems(wxTreeItemId sizerNodeId, wxSizer* sizer);
    wxTreeItemId DoAddWindow(wxTreeItemId parentId, wxWindow* window);
    wxTreeItemId DoAddSizer(wxTreeItemId parentId, wxSizer* sizer);
    wxTreeItemId DoAddSizerItem(wxTreeItemId parentId, wxSizerItem* item);

    void ExpandAllChildren(wxTreeItemId item);
    void CollapseAllChildren(wxTreeItemId item);
    wxTreeItemId FindItemForObject(wxTreeItemId parent, wxObject* target);

    wxTreeCtrl* m_tree;
    wxToolBar* m_toolbar;
    bool m_showSizers;
    bool m_findWidgetCapture;
    wxDECLARE_EVENT_TABLE();
};

} // namespace wxInspector

#endif // WX_INSPECTOR_TREE_H
