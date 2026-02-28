#include "globals.hpp"
#include "ColumnsAlgorithm.hpp"

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hypr-columns:max_columns", Hyprlang::INT{3});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hypr-columns:spawn_direction", Hyprlang::STRING{"right"});

    HyprlandAPI::addTiledAlgo(PHANDLE, "columns", &typeid(CColumnsAlgorithm),
        []() -> UP<Layout::ITiledAlgorithm> { return makeUnique<CColumnsAlgorithm>(); });

    return {"hypr-columns", "Equal-width column layout", "Andreas Bjoru", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    HyprlandAPI::removeAlgo(PHANDLE, "columns");
}
