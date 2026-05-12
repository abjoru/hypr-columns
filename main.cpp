#include "globals.hpp"
#include "ColumnsAlgorithm.hpp"
#include <hyprland/src/config/values/types/IntValue.hpp>
#include <hyprland/src/config/values/types/StringValue.hpp>

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    HyprlandAPI::addConfigValueV2(PHANDLE,
        makeShared<Config::Values::CIntValue>(
            "plugin:hypr-columns:max_columns",
            "Maximum number of columns before stacking",
            Config::INTEGER{3},
            Config::Values::SIntValueOptions{.min = 1}));
    HyprlandAPI::addConfigValueV2(PHANDLE,
        makeShared<Config::Values::CStringValue>(
            "plugin:hypr-columns:spawn_direction",
            "Direction new columns spawn (left or right)",
            Config::STRING{"right"}));

    HyprlandAPI::addTiledAlgo(PHANDLE, "columns", &typeid(CColumnsAlgorithm),
        []() -> UP<Layout::ITiledAlgorithm> { return makeUnique<CColumnsAlgorithm>(); });

    return {"hypr-columns", "Equal-width column layout", "Andreas Bjoru", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    HyprlandAPI::removeAlgo(PHANDLE, "columns");
}
