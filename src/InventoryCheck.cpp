#include "Global.h"

#include <filesystem>

#include <unordered_map>

bool saveInventory(Player& player) {
    auto& pl   = static_cast<gmlib::GMPlayer&>(player);
    auto  uuid = pl.getUuid();
    ll::file_utils::writeFile(
        "./plugins/InventoryChecker/save/" + uuid.asString() + ".dat",
        pl.getNbt()->toBinaryNbt(),
        true
    );
    return false;
}

bool resumeInventory(Player& player) {
    auto&       pl   = static_cast<gmlib::GMPlayer&>(player);
    auto        uuid = pl.getUuid();
    std::string path = "./plugins/InventoryChecker/save/" + uuid.asString() + ".dat";
    if (auto binaryData = ll::file_utils::readFile(path, true)) {
        std::filesystem::remove(path);
        if (auto nbt = CompoundTag::fromBinaryNbt(binaryData.value())) {
            auto playerNbt=pl.getNbt();
            playerNbt->mTags["Armor"] =nbt->at("Armor");
            playerNbt->mTags["EnderChestInventory"] = nbt->at("EnderChestInventory");
            playerNbt->mTags["Offhand"] = nbt->at("Offhand");
            playerNbt->mTags["Inventory"] = nbt->at("Inventory");
            pl.setNbt(*playerNbt);
            pl.refreshInventory();


            return true;
        }
    }
    return false;
}


void mainForm(Player& pl) {
    auto fm = ll::form::SimpleForm(tr("form.main.title"), tr("form.main.content"));
    fm.appendButton(tr("form.main.checkOnline"), [](Player& pl) { return checkOnlineForm(pl); });
    fm.appendButton(tr("form.main.checkAll"), [](Player& pl) { return checkAllForm(pl); });
    fm.appendButton(tr("form.main.searchPlayer"), [](Player& pl) { return searchPlayerForm(pl); });
    fm.appendButton(tr("form.main.resumeInventory"), [](Player& pl) {
        auto res = resumeInventory(pl);
        if (res) {
            return pl.sendMessage(tr("resumeInventory.success"));
        }
        return pl.sendMessage(tr("resumeInventory.failed"));
    });
    fm.sendTo(pl);
}

void checkOnlineForm(Player& pl) {
    auto fm = ll::form::SimpleForm(tr("form.checkList.title"), tr("form.checkList.content"));
    std::unordered_map<mce::UUID, std::string> onlineList;
    gmlib::GMLevel::getInstance()->forEachPlayer([&onlineList](Player& pl) -> bool {
        if (!pl.isSimulated()) {
            onlineList[pl.getUuid()] = pl.getRealName();
        }
        return true;
    });
    for (auto& [uuid, name] : onlineList) {
        if (uuid != pl.getUuid()) {
            fm.appendButton(name, [uuid](Player& pl) { return checkPlayerForm(pl, uuid); });
        }
    }
    if (onlineList.size() == 1) {
        return pl.sendMessage(tr("checkOnline.empty"));
    }
    fm.sendTo(pl, [](Player& pl, int index, ll::form::FormCancelReason) {
        if (index == -1) {
            return mainForm(pl);
        }
    });
}
std::string getNameFormUuid(mce::UUID const& uuid) {
    auto        player = ll::service::getLevel()->getPlayer(uuid);
    std::string name;
    if (player) {
        name = player->getRealName();
    } else {
        // 获取 UserCache 实例
        auto userCache = gmlib::UserCache::getInstance();
        if (!userCache) {
            return uuid.asString(); // UserCache 未初始化或不可用
        }

        // 查询用户信息
        auto entry = userCache->from(uuid);


        if (!entry) {
            return uuid.asString(); // 未找到对应 UUID 的用户
        }
        name = entry->mName;
    }
    return name;
}

std::unordered_map<mce::UUID, std::string> generateUuidMap() {
    auto userCache = gmlib::UserCache::getInstance();
    if (!userCache) {
        // 处理 UserCache 未初始化的情况
    }

    std::vector<mce::UUID> allUuids;
    for (const auto& entry : userCache->entries()) {
        allUuids.push_back(entry.mUuid);
    }
    for (const auto& player : ll::service::getLevel()->getPlayerList()) {
        allUuids.push_back(player.first);
    }
    std::unordered_map<mce::UUID, std::string> allList;
    for (auto& uuid : allUuids) {
        auto name     = getNameFormUuid(uuid);
        allList[uuid] = name;
    }
    return std::move(allList);
}

void checkAllForm(Player& pl) {
    auto fm      = ll::form::SimpleForm(tr("form.checkList.title"), tr("form.checkList.content"));
    auto allList = generateUuidMap();
    for (auto& [uuid, name] : allList) {
        if (uuid != pl.getUuid()) {
            fm.appendButton(name, [uuid](Player& pl) { return checkPlayerForm(pl, uuid); });
        }
    }
    if (allList.size() == 1) {
        return pl.sendMessage(tr("checkAll.empty"));
    }
    fm.sendTo(pl, [](Player& pl, int index, ll::form::FormCancelReason) {
        if (index == -1) {
            return mainForm(pl);
        }
    });
}

void searchPlayerForm(Player& pl) {
    auto fm = ll::form::CustomForm(tr("form.searchPlayer.title"));
    fm.appendLabel(tr("form.searchPlayer.description"));
    fm.appendInput("name", tr("form.searchPlayer.input"), tr("form.searchPlayer.inputName"));
    fm.appendDropdown(
        "mode",
        tr("form.searchForm.searchMode"),
        {tr("form.searchPlayer.fuzzySearch"), tr("form.searchPlayer.preciseSearch")},
        0
    );
    fm.sendTo(pl, [](Player& pl, ll::form::CustomFormResult const& result, ll::form::FormCancelReason) {
        if (!result.has_value()) {
            return mainForm(pl);
        }
        std::string inputName;
        if (std::holds_alternative<std::string>(result->at("name"))) {
            inputName = std::get<std::string>(result->at("name"));
        }
        auto searchMode = std::get<std::string>(result->at("mode"));
        if (inputName.empty()) {
            return invalidInputForm(pl);
        }
        auto allList = generateUuidMap();
        if (searchMode == tr("form.searchPlayer.preciseSearch")) {
            for (auto& [uuid, name] : allList) {
                if (name == inputName) {
                    return checkPlayerForm(pl, uuid);
                }
            }
        } else {
            std::unordered_map<mce::UUID, std::string> resultList;
            for (auto& [uuid, name] : allList) {
                if (name.find(inputName) != std::string::npos) {
                    resultList[uuid] = name;
                }
            }
            if (!resultList.empty()) {
                return searchResultForm(pl, resultList);
            }
        }
        return searchNotFoundForm(pl, inputName);
    });
}

void searchResultForm(Player& pl, std::unordered_map<mce::UUID, std::string> const& resultList) {
    auto fm = ll::form::SimpleForm(tr("form.checkList.title"), tr("form.checkList.content"));
    for (auto& [uuid, name] : resultList) {
        fm.appendButton(name, [uuid](Player& pl) { checkPlayerForm(pl, uuid); });
    }
    fm.sendTo(pl, [](Player& pl, int index, ll::form::FormCancelReason) {
        if (index == -1) {
            return searchPlayerForm(pl);
        }
    });
}

void searchNotFoundForm(Player& pl, std::string const& name) {
    auto fm = ll::form::ModalForm(
        tr("form.notFound.title"),
        tr("form.notFound.content", {name}),
        tr("form.notFound.returnSearch"),
        tr("form.returnMainForm")
    );
    fm.sendTo(pl, [](Player& pl, ll::form::ModalFormResult result, ll::form::FormCancelReason) {
        if (result == ll::form::ModalFormSelectedButton::Upper) {
            return searchPlayerForm(pl);
        }
        return mainForm(pl);
    });
}
std::unique_ptr<CompoundTag> nbt_Getter(mce::UUID uuid) {

    if (auto player = ll::service::getLevel()->getPlayer(uuid)) {
        auto& pl=static_cast<gmlib::GMPlayer&>(*player);
        return pl.getNbt();
    }
    else {
        auto db   = ll::service::getDBStorage();
        if (db && db->hasKey("player_" + uuid.asString(), DBHelpers::Category::Player)) {
            std::unique_ptr<CompoundTag> playerTag =
                db->getCompoundTag("player_" + uuid.asString(), DBHelpers::Category::Player);
            if (playerTag) {
                std::string serverId = playerTag->at("ServerId");
                if (!serverId.empty() && db->hasKey(serverId, DBHelpers::Category::Player)) {
                    return db->getCompoundTag(serverId, DBHelpers::Category::Player);
                }
            }
        }

    }
    return nullptr;
}
void checkPlayerForm(Player& pl, mce::UUID const& uuid) {
    std::string name = getNameFormUuid(uuid);
    auto        fm   = ll::form::SimpleForm(tr("form.checkPlayer.title"), tr("form.checkPlayer.content", {name}));
    fm.appendButton(tr("form.checkPlayer.copyInventory"), [uuid, name](Player& pl) {
        saveInventory(pl);
        auto& pl1= (static_cast<gmlib::GMPlayer&>(pl));
        auto nbt =nbt_Getter(uuid);
        if (!nbt) {
            return pl.sendMessage(tr("checkPlayer.copyInventory.failed", {name}));
        }
        auto playerNbt=pl1.getNbt();
        playerNbt->mTags["Armor"] = nbt->at("Armor");
        playerNbt->mTags["EnderChestInventory"] = nbt->at("EnderChestInventory");
        playerNbt->mTags["Offhand"] = nbt->at("Offhand");
        playerNbt->mTags["Inventory"] = nbt->at("Inventory");
        pl1.setNbt(*playerNbt);
            pl.refreshInventory();

        return pl.sendMessage(tr("checkPlayer.copyInventory.success", {name}));
    });
    fm.appendButton(tr("form.checkPlayer.writeInventory"), [uuid, name](Player& pl) {
        return confirmWriteForm(pl, uuid, name);
    });
    fm.appendButton(tr("form.checkPlayer.deleteData"), [uuid, name](Player& pl) {
        return confirmDeleteForm(pl, uuid, name);
    });
    fm.sendTo(pl, [](Player& pl, int index, ll::form::FormCancelReason) {
        if (index == -1) {
            return mainForm(pl);
        }
    });
}

void invalidInputForm(Player& pl) {
    auto fm = ll::form::ModalForm(
        tr("form.invalidInput.title"),
        tr("form.invalidInput.content"),
        tr("form.invalidInput.returnSearch"),
        tr("form.returnMainForm")
    );
    fm.sendTo(pl, [](Player& pl, ll::form::ModalFormResult result, ll::form::FormCancelReason) {
        if (result == ll::form::ModalFormSelectedButton::Upper) {
            return searchPlayerForm(pl);
        }
        return mainForm(pl);
    });
}
namespace MoreGlobal {

DefaultDataLoadHelper  helper;
DefaultDataLoadHelper& defaultDataLoadHelper() { return helper; }

} // namespace lse::api::MoreGlobal
bool Nbt_Setter(mce::UUID uuid, std::unique_ptr<CompoundTag> nbt) {
    try {


        Player*   player = ll::service::getLevel()->getPlayer(uuid);
        if (player && nbt) {
            player->load(*nbt, MoreGlobal::defaultDataLoadHelper());
            player->refreshInventory();
            return true;
        } else if (nbt) {
            auto db = ll::service::getDBStorage();
            if (db && db->hasKey("player_" + uuid.asString(), DBHelpers::Category::Player)) {
                std::unique_ptr<CompoundTag> playerTag =
                    db->getCompoundTag("player_" + uuid.asString(), DBHelpers::Category::Player);
                if (playerTag) {
                    std::string serverId = playerTag->at("ServerId");
                    if (!serverId.empty()) {
                        db->saveData(serverId, nbt->toBinaryNbt(), DBHelpers::Category::Player);
                        return true;
                    }
                }
            }
        }
        return false;
    }
    catch (...) {

        return false;
    }
}

void confirmWriteForm(Player& pl, mce::UUID const& uuid, std::string const& name) {
    auto fm = ll::form::ModalForm(
        tr("form.confirmWrite.title"),
        tr("form.confirmWrite.content", {name}),
        tr("form.confirmAction"),
        tr("form.cancelAction")
    );
    fm.sendTo(pl, [uuid, name](Player& pl, ll::form::ModalFormResult result, ll::form::FormCancelReason) {
        if (result == ll::form::ModalFormSelectedButton::Upper) {
            auto& self      = static_cast<gmlib::GMPlayer&>(pl);
            auto  nbt       = self.getNbt();
            auto  targetNbt = nbt_Getter(uuid);
            if (!targetNbt) {
                return pl.sendMessage(tr("checkPlayer.writeInventory.failed", {name}));
            }
            targetNbt->mTags["Armor"] = nbt->at("Armor");
            targetNbt->mTags["EnderChestInventory"] = nbt->at("EnderChestInventory");
            targetNbt->mTags["Offhand"] = nbt->at("Offhand");
            targetNbt->mTags["Inventory"] = nbt->at("Inventory");
            bool success= Nbt_Setter(uuid, std::move(targetNbt));
            pl.refreshInventory();
            if (!success) {
                return pl.sendMessage(tr("checkPlayer.writeInventory.failed", {name}));
            }

            return pl.sendMessage(tr("checkPlayer.writeInventory.success", {name}));
        }
        return checkPlayerForm(pl, uuid);
    });
}

void confirmDeleteForm(Player& pl, mce::UUID const& uuid, std::string const& name) {
    auto fm = ll::form::ModalForm(
        tr("form.confirmDelete.title"),
        tr("form.confirmDelete.content", {name}),
        tr("form.confirmAction"),
        tr("form.cancelAction")
    );
    fm.sendTo(pl, [uuid, name](Player& pl, ll::form::ModalFormResult result, ll::form::FormCancelReason) {
        if (result == ll::form::ModalFormSelectedButton::Upper) {
            if (auto player = ll::service::getLevel()->getPlayer(uuid)) {
                return deleteFailedForm(pl, uuid, name);
            }
        gmlib::OfflinePlayer::deletePlayerNbt(uuid);

            return pl.sendMessage(tr("checkPlayer.deletePlayer.success", {name}));
        }
        return checkPlayerForm(pl, uuid);
    });
}

void deleteFailedForm(Player& pl, mce::UUID const& uuid, std::string const& name) {
    auto fm = ll::form::ModalForm(
        tr("form.deleteFailed.title"),
        tr("form.deleteFailed.content", {name}),
        tr("form.deleteFailed.returnCheck"),
        tr("form.returnMainForm")
    );
    fm.sendTo(pl, [uuid](Player& pl, ll::form::ModalFormResult result, ll::form::FormCancelReason) {
        if (result == ll::form::ModalFormSelectedButton::Upper) {
            return checkPlayerForm(pl, uuid);
        }
        return mainForm(pl);
    });
}