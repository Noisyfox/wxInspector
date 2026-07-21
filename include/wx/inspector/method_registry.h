#ifndef WX_INSPECTOR_METHOD_REGISTRY_H
#define WX_INSPECTOR_METHOD_REGISTRY_H

#include <wx/vector.h>
#include "wx/inspector/plugin.h"
#include "wx/inspector/object.h"

namespace wxInspector {

class MethodRegistry {
public:
    static MethodRegistry& Get();

    void RegisterPlugin(wxInspectorPlugin* plugin);
    void RegisterBuiltinMethods();

    wxVector<MethodInfo> GetMethods(InspectableObject& obj);

private:
    MethodRegistry() {}
    wxVector<wxInspectorPlugin*> m_plugins;
};

} // namespace wxInspector

#endif // WX_INSPECTOR_METHOD_REGISTRY_H
