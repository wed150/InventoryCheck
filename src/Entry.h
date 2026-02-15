#pragma once
#include <span>

#include "Config.h"
#include "Global.h"

#include <memory>

namespace InventoryCheck {

using namespace gmlib::i18n;

class Entry {

public:
    static Entry& getInstance();

    Entry() : mSelf((*ll::mod::NativeMod::current())) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    /// @return True if the mod is loaded successfully.
    bool load();

    /// @return True if the mod is enabled successfully.
    bool enable();

    /// @return True if the mod is disabled successfully.
    bool disable();

    /// @return True if the mod is unloaded successfully.

    Config1& getConfig();

    LangI18n& getI18n();

private:
    ll::mod::NativeMod& mSelf;
    std::optional<Config1>     mConfig;
    std::optional<LangI18n>   mI18n;
};

} // namespace InventoryCheck
