#ifndef WX_INSPECTOR_TREE_H
#define WX_INSPECTOR_TREE_H

#include <wx/panel.h>
#include <wx/treectrl.h>
#include <wx/toolbar.h>
#include <wx/eventfilter.h>
#include "wx/inspector/object.h"

namespace wxInspector {

wxDECLARE_EVENT(wxEVT_INSPECT_TREE_SEL_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_INSPECT_TREE_HIGHLIGHT, wxCommandEvent);

class InspectionTree : public wxPanel {
public:
    InspectionTree(wxWindow* parent);
    ~InspectionTree();

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
    bool IsFindWidgetCapture() const { return m_findWidgetCapture; }

private:
    class FindWidgetEventFilter : public wxEventFilter {
    public:
        explicit FindWidgetEventFilter(InspectionTree* tree) : m_tree(tree) {}
        int FilterEvent(wxEvent& event) override;
    private:
        InspectionTree* m_tree;
    };

    void OnTreeSelChanged(wxTreeEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnToggleSizers(wxCommandEvent& event);
    void OnExpandAll(wxCommandEvent& event);
    void OnCollapseAll(wxCommandEvent& event);
    void OnFindWidget(wxCommandEvent& event);
    void OnHighlight(wxCommandEvent& event);
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
    void EndFindWidget();

    wxTreeCtrl* m_tree;
    wxToolBar* m_toolbar;
    bool m_showSizers;
    bool m_findWidgetCapture;
    FindWidgetEventFilter m_findWidgetFilter;
    wxDECLARE_EVENT_TABLE();
};

} // namespace wxInspector

#endif // WX_INSPECTOR_TREE_H
