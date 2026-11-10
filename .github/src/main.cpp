#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>

using namespace geode::prelude;

namespace {
    // remember who we already blocked this session so we don't spam the server
    int s_alreadyBlocked = -1;

    bool triggerEnabled(char const* which) {
        auto trigger = Mod::get()->getSettingValue<std::string>("trigger");
        return trigger == which || trigger == "Both";
    }

    void blockTarget() {
        auto id = static_cast<int>(Mod::get()->getSettingValue<int64_t>("target-account-id"));
        auto name = Mod::get()->getSettingValue<std::string>("target-username");

        if (id <= 0) {
            log::warn("Insta Block: no target account ID set in settings.");
            return;
        }
        if (id == s_alreadyBlocked) return;

        if (GJAccountManager::get()->m_accountID <= 0) {
            log::warn("Insta Block: you must be logged in to block users.");
            return;
        }

        GameLevelManager::get()->blockUser(id);
        s_alreadyBlocked = id;
        log::info("Insta Block: blocked {} ({})", name, id);

        if (Mod::get()->getSettingValue<bool>("show-notification")) {
            Notification::create(fmt::format("Blocked {}", name), NotificationIcon::Success)->show();
        }
    }
}

class $modify(InstaBlockPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        bool wasDead = player->m_isDead;
        PlayLayer::destroyPlayer(player, object);

        // ignore the anticheat spike and repeated calls
        if (object == m_anticheatSpike) return;
        if (!wasDead && player->m_isDead && triggerEnabled("On Death")) {
            blockTarget();
        }
    }
};

class $modify(InstaBlockPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        if (triggerEnabled("On Pause")) {
            blockTarget();
        }
    }
};
