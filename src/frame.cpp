#include "wx/inspector/frame.h"
#include "wx/inspector/tree.h"
#include "wx/inspector/info.h"
#include "wx/inspector/invoker.h"
#include "wx/inspector/events.h"
#include "wx/inspector/highlighter.h"
#include "wx/inspector/property_provider.h"
#include "wx/inspector/method_registry.h"
#include <wx/menu.h>
#include <wx/toolbar.h>
#include <wx/artprov.h>
#include <wx/accel.h>
#include <wx/msgdlg.h>

namespace wxInspector {

enum {
    ID_HIGHLIGHT = wxID_HIGHEST + 400,
    ID_FIND_WIDGET,
    ID_REFRESH_TREE,
    ID_TOGGLE_SIZERS,
    ID_EXPAND_ALL,
    ID_COLLAPSE_ALL,
    ID_LAYOUT,
    ID_ABOUT,
    ID_QUIT,
    ID_VIEW_TREE,
    ID_VIEW_INFO,
    ID_VIEW_INVOKER,
    ID_VIEW_EVENTS,
};

wxBEGIN_EVENT_TABLE(InspectionFrame, wxFrame)
    EVT_CLOSE(InspectionFrame::OnClose)
    EVT_COMMAND(wxID_ANY, wxEVT_INSPECT_TREE_SEL_CHANGED,
                InspectionFrame::OnTreeSelectionChanged)
wxEND_EVENT_TABLE()

InspectionFrame::InspectionFrame(wxWindow* parent, wxPoint pos, wxSize size)
    : wxFrame(parent, wxID_ANY, "wxInspector", pos, size,
              wxDEFAULT_FRAME_STYLE | wxFRAME_NO_TASKBAR),
      m_auiMgr(this)
{
    SetIcon(wxArtProvider::GetIcon(wxART_FIND));

    // Initialize registries (idempotent — safe to call multiple times)
    PropertyProvider::Get().RegisterBuiltinProviders();
    MethodRegistry::Get().RegisterBuiltinMethods();

    // Create panels
    m_tree = new InspectionTree(this);
    m_info = new ObjectInfoPanel(this);
    m_invoker = new MethodInvokerPanel(this);
    m_events = new EventLoggerPanel(this);
    m_highlighter = new InspectionHighlighter();

    SetupMenuBar();
    SetupToolBar();
    SetupAUI();

    // Keyboard accelerators
    wxAcceleratorEntry entries[] = {
        { wxACCEL_NORMAL, WXK_F1, ID_REFRESH_TREE },
        { wxACCEL_NORMAL, WXK_F2, ID_FIND_WIDGET },
        { wxACCEL_NORMAL, WXK_F3, ID_TOGGLE_SIZERS },
        { wxACCEL_NORMAL, WXK_F4, ID_EXPAND_ALL },
        { wxACCEL_NORMAL, WXK_F5, ID_COLLAPSE_ALL },
        { wxACCEL_NORMAL, WXK_F6, ID_HIGHLIGHT },
        { wxACCEL_CTRL,   'I',   ID_QUIT },
    };
    wxAcceleratorTable accel(sizeof(entries) / sizeof(entries[0]), entries);
    SetAcceleratorTable(accel);
}

InspectionFrame::~InspectionFrame()
{
    m_auiMgr.UnInit();
}

void InspectionFrame::SetupMenuBar()
{
    wxMenuBar* mb = new wxMenuBar();

    wxMenu* fileMenu = new wxMenu();
    fileMenu->Append(ID_REFRESH_TREE, "&Refresh Tree\tF1");
    fileMenu->AppendCheckItem(ID_TOGGLE_SIZERS, "&Show Sizers\tF3");
    fileMenu->AppendSeparator();
    fileMenu->Append(ID_QUIT, "&Close\tCtrl+I");
    mb->Append(fileMenu, "&File");

    wxMenu* viewMenu = new wxMenu();
    viewMenu->AppendCheckItem(ID_VIEW_TREE, "&Widget Tree");
    viewMenu->AppendCheckItem(ID_VIEW_INFO, "&Object Info");
    viewMenu->AppendCheckItem(ID_VIEW_INVOKER, "M&ethod Invoker");
    viewMenu->AppendCheckItem(ID_VIEW_EVENTS, "&Event Logger");
    mb->Append(viewMenu, "&View");

    wxMenu* toolsMenu = new wxMenu();
    toolsMenu->Append(ID_HIGHLIGHT, "&Highlight Widget\tF6");
    toolsMenu->Append(ID_FIND_WIDGET, "&Find Widget\tF2");
    mb->Append(toolsMenu, "&Tools");

    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(ID_ABOUT, "&About");
    mb->Append(helpMenu, "&Help");

    SetMenuBar(mb);

    Bind(wxEVT_MENU, [this](wxCommandEvent&) { m_tree->RebuildTree(); },
         ID_REFRESH_TREE);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { m_tree->ToggleSizers(); },
         ID_TOGGLE_SIZERS);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { m_tree->ExpandAll(); },
         ID_EXPAND_ALL);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { m_tree->CollapseAll(); },
         ID_COLLAPSE_ALL);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(); }, ID_QUIT);
    Bind(wxEVT_MENU, &InspectionFrame::OnHighlight, this, ID_HIGHLIGHT);
    Bind(wxEVT_MENU, &InspectionFrame::OnFindWidget, this, ID_FIND_WIDGET);
    Bind(wxEVT_MENU, &InspectionFrame::OnLayout, this, ID_LAYOUT);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        wxMessageBox(
            wxString::FromUTF8("wxInspector — Widget Inspection Tool\n") +
            "Ported from wxPython wx.lib.inspection\n\n"
            "Ctrl+Shift+I to toggle",
            "About wxInspector", wxOK | wxICON_INFORMATION, this);
    }, ID_ABOUT);

    // View menu — panel toggles
    auto togglePane = [this](const wxString& name) {
        wxAuiPaneInfo& pane = m_auiMgr.GetPane(name);
        pane.Show(!pane.IsShown());
        m_auiMgr.Update();
    };
    Bind(wxEVT_MENU, [togglePane](wxCommandEvent&) { togglePane("Tree"); }, ID_VIEW_TREE);
    Bind(wxEVT_MENU, [togglePane](wxCommandEvent&) { togglePane("Info"); }, ID_VIEW_INFO);
    Bind(wxEVT_MENU, [togglePane](wxCommandEvent&) { togglePane("Invoker"); }, ID_VIEW_INVOKER);
    Bind(wxEVT_MENU, [togglePane](wxCommandEvent&) { togglePane("Events"); }, ID_VIEW_EVENTS);

    Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) {
        e.Check(m_auiMgr.GetPane("Tree").IsShown());
    }, ID_VIEW_TREE);
    Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) {
        e.Check(m_auiMgr.GetPane("Info").IsShown());
    }, ID_VIEW_INFO);
    Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) {
        e.Check(m_auiMgr.GetPane("Invoker").IsShown());
    }, ID_VIEW_INVOKER);
    Bind(wxEVT_UPDATE_UI, [this](wxUpdateUIEvent& e) {
        e.Check(m_auiMgr.GetPane("Events").IsShown());
    }, ID_VIEW_EVENTS);
}

void InspectionFrame::SetupToolBar()
{
    wxToolBar* tb = CreateToolBar(wxTB_FLAT | wxTB_NODIVIDER);
    tb->AddTool(ID_REFRESH_TREE, "Refresh",
                wxArtProvider::GetBitmap(wxART_REDO), "Refresh (F1)");
    tb->AddTool(ID_FIND_WIDGET, "Find",
                wxArtProvider::GetBitmap(wxART_CROSS_MARK), "Find Widget (F2)");
    tb->AddCheckTool(ID_TOGGLE_SIZERS, "Sizers",
                     wxArtProvider::GetBitmap(wxART_FIND), wxNullBitmap,
                     "Toggle Sizers (F3)");
    tb->AddTool(ID_HIGHLIGHT, "Highlight",
                wxArtProvider::GetBitmap(wxART_TIP), "Highlight (F6)");
    tb->AddTool(ID_LAYOUT, "Layout",
                wxArtProvider::GetBitmap(wxART_EXECUTABLE_FILE), "Layout");
    tb->Realize();
}

void InspectionFrame::SetupAUI()
{
    m_auiMgr.SetManagedWindow(this);

    wxAuiPaneInfo treePane;
    treePane.Name("Tree").Caption("Widget Tree").Left().Layer(0)
            .Row(0).Position(0).BestSize(250, 400).MinSize(150, 200);
    m_auiMgr.AddPane(m_tree, treePane);

    wxAuiPaneInfo infoPane;
    infoPane.Name("Info").Caption("Object Info").Center().Layer(0)
            .Row(0).Position(0).BestSize(400, 400).MinSize(200, 200);
    m_auiMgr.AddPane(m_info, infoPane);

    wxAuiPaneInfo invokerPane;
    invokerPane.Name("Invoker").Caption("Method Invoker").Bottom().Layer(0)
               .Row(0).Position(0).BestSize(600, 200).MinSize(300, 150);
    m_auiMgr.AddPane(m_invoker, invokerPane);

    wxAuiPaneInfo eventPane;
    eventPane.Name("Events").Caption("Event Logger").Bottom().Layer(0)
             .Row(1).Position(0).BestSize(600, 200).MinSize(300, 150);
    m_auiMgr.AddPane(m_events, eventPane);

    m_auiMgr.Update();
}

void InspectionFrame::Show(wxObject* selectObj)
{
    // Always rebuild — the tree was initially built during Init() before
    // the application's main window existed, so it only contains the
    // inspector frame itself. Rebuilding on each show ensures an up-to-date
    // view without requiring the user to press F1.
    m_tree->RebuildTree();
    if (selectObj) SelectObject(selectObj);
    wxFrame::Show(true);
    Raise();
}

void InspectionFrame::SelectObject(wxObject* obj)
{
    m_tree->SelectObject(obj);
}

void InspectionFrame::RefreshTree()
{
    m_tree->RebuildTree();
}

void InspectionFrame::OnTreeSelectionChanged(wxCommandEvent& event)
{
    wxObject* obj = static_cast<wxObject*>(event.GetClientData());
    InspectableObject inspObj = InspectableObject::FromWindow(nullptr);

    if (obj)
    {
        wxClassInfo* info = obj->GetClassInfo();
        if (info->IsKindOf(CLASSINFO(wxWindow)))
            inspObj = InspectableObject::FromWindow(
                static_cast<wxWindow*>(obj));
        else if (info->IsKindOf(CLASSINFO(wxSizer)))
            inspObj = InspectableObject::FromSizer(
                static_cast<wxSizer*>(obj));
        else if (info->IsKindOf(CLASSINFO(wxSizerItem)))
            inspObj = InspectableObject::FromSizerItem(
                static_cast<wxSizerItem*>(obj));
    }

    if (!obj)
    {
        m_info->Clear();
        m_invoker->Clear();
    }
    else
    {
        m_info->ShowObject(inspObj);
        m_invoker->ShowObject(inspObj);
        // Skip auto-highlight for top-level windows — the flicker
        // effect is disruptive. F6 still works for manual highlight.
        if (inspObj.GetKind() != InspectableObject::Kind::Window ||
            !wxDynamicCast(inspObj.AsWindow(), wxTopLevelWindow))
        {
            m_highlighter->Highlight(inspObj);
        }
    }
    m_events->ShowObject(inspObj);
}

void InspectionFrame::OnHighlight(wxCommandEvent&)
{
    InspectableObject obj = m_tree->GetSelectedObject();
    if (obj.IsValid())
        m_highlighter->Highlight(obj);
}

void InspectionFrame::OnFindWidget(wxCommandEvent&)
{
    m_tree->FindWidget();
}

void InspectionFrame::OnLayout(wxCommandEvent&)
{
    wxWindow* win = m_tree->GetContainerWindow();
    if (win)
        win->Layout();
}

void InspectionFrame::OnClose(wxCloseEvent& event)
{
    // Hide instead of destroying — the inspector is a persistent singleton
    // that can be reshown via Show(). Use Hide() rather than Show(false)
    // because InspectionFrame::Show(wxObject*,bool) hides wxWindow::Show(bool),
    // and the wxTopLevelWindow::Show override (needed on macOS) would be skipped.
    // Veto the event to prevent default destruction.
    event.Veto(true);
    Hide();
}

} // namespace wxInspector
