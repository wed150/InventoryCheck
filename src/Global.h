#pragma once

#include <span>
#include "mc/world/events/LevelTickingAreaFinishedLoadingEvent.h"

#include "gmlib/include_mc.h"

#include "gmlib/include_ll.h"
#include "gmlib/include_static_lib.h"
#include "gmlib/include_shared_lib.h"
#include <string>
#include <vector>

#define MOD_NAME     "InventoryCheck"
#define TARGET_PROTOCOL 898

extern ll::io::Logger logger;

extern std::string tr(std::string const& key, std::vector<std::string> const& data = {});

extern void RegisterCommand();

extern void mainForm(Player& pl);
extern void checkOnlineForm(Player& pl);
extern void checkAllForm(Player& pl);
extern void searchPlayerForm(Player& pl);
extern void searchResultForm(Player& pl, std::unordered_map<mce::UUID, std::string> const& resultList);
extern void searchNotFoundForm(Player& pl, std::string const& name);
extern void checkPlayerForm(Player& pl, mce::UUID const& uuid);
extern void invalidInputForm(Player& pl);
extern void confirmWriteForm(Player& pl, mce::UUID const& uuid, std::string const& name);
extern void confirmDeleteForm(Player& pl, mce::UUID const& uuid, std::string const& name);
extern void deleteFailedForm(Player& pl, mce::UUID const& uuid, std::string const& name);