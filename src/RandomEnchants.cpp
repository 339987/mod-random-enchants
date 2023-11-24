/*
* Converted from the original LUA script to a module for Azerothcore(Sunwell) :D
*/
#include "ScriptMgr.h"
#include "Player.h"
#include "Configuration/Config.h"
#include "Chat.h"

// Enum for item qualities
enum ItemQuality {
    GREY = 0,
    WHITE,
    GREEN,
    BLUE,
    PURPLE,
    ORANGE
};

class RandomEnchantsPlayer : public PlayerScript {
public:
    RandomEnchantsPlayer() : PlayerScript("RandomEnchantsPlayer") { }

    void OnLogin(Player* player) override {
       if (sConfigMgr->GetOption<bool>("RandomEnchants.AnnounceOnLogin", true) && sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true))
            ChatHandler(player->GetSession()).SendSysMessage(sConfigMgr->GetOption<std::string>("RandomEnchants.OnLoginMessage", "This server is running a RandomEnchants Module.").c_str());
    }

    void OnLootItem(Player* player, Item* item, uint32 /*count*/, ObjectGuid /*lootguid*/) override {
        if (sConfigMgr->GetOption<bool>("RandomEnchants.OnLoot", true) && sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true))
            RollPossibleEnchant(player, item);
    }

    void OnCreateItem(Player* player, Item* item, uint32 /*count*/) override {
        if (sConfigMgr->GetOption<bool>("RandomEnchants.OnCreate", true) && sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true))
            RollPossibleEnchant(player, item);
    }

    void OnQuestRewardItem(Player* player, Item* item, uint32 /*count*/) override {
        if (sConfigMgr->GetOption<bool>("RandomEnchants.OnQuestReward", true) && sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true))
            RollPossibleEnchant(player, item);
    }

    void OnGroupRollRewardItem(Player* player, Item* item, uint32 /*count*/, RollVote /*voteType*/, Roll* /*roll*/) override {
        if (sConfigMgr->GetOption<bool>("RandomEnchants.OnGroupRoll", true) && sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true))
            RollPossibleEnchant(player, item);
    }

    std::string GetItemLink(uint32 entry, WorldSession* session) const
    {
        if (entry == HIDDEN_ITEM_ID) {
            std::ostringstream oss;
            oss << "(Hidden)";
            return oss.str();
        }
        const ItemTemplate* temp = sObjectMgr->GetItemTemplate(entry);
        int loc_idx = session->GetSessionDbLocaleIndex();
        std::string name = temp->Name1;
        if (ItemLocale const* il = sObjectMgr->GetItemLocale(entry))
            ObjectMgr::GetLocaleString(il->Name, loc_idx, name);

        std::ostringstream oss;
        oss << "|c" << std::hex << ItemQualityColors[temp->Quality] << std::dec <<
            "|Hitem:" << entry << ":0:0:0:0:0:0:0:0:0|h[" << name << "]|h|r";

        return oss.str();
    }

    void RollPossibleEnchant(Player* player, Item* item) {
        // Check global enable option
        if (!sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true)) {
            return;
        }
        uint32 Quality = item->GetTemplate()->Quality;
        uint32 Class = item->GetTemplate()->Class;

        if (
            (Quality > 5 || Quality < 1) /* eliminates enchanting anything that isn't a recognized quality */ ||
            (Class != 2 && Class != 4) /* eliminates enchanting anything but weapons/armor */) {
            return;
        }

        int slotRand[3] = { -1, -1, -1 };
        uint32 slotEnch[3] = { 0, 1, 5 };

        // Fetching the configuration values as float
        float enchantChance1 = sConfigMgr->GetOption<float>("RandomEnchants.EnchantChance1", 70.0f);
        float enchantChance2 = sConfigMgr->GetOption<float>("RandomEnchants.EnchantChance2", 65.0f);
        float enchantChance3 = sConfigMgr->GetOption<float>("RandomEnchants.EnchantChance3", 60.0f);

        if (rand_chance() < enchantChance1)
            slotRand[0] = getRandEnchantment(item);
        if (slotRand[0] != -1 && rand_chance() < enchantChance2)
            slotRand[1] = getRandEnchantment(item);
        if (slotRand[1] != -1 && rand_chance() < enchantChance3)
            slotRand[2] = getRandEnchantment(item);

        for (int i = 0; i < 3; i++) {
            if (slotRand[i] != -1) {
                if (sSpellItemEnchantmentStore.LookupEntry(slotRand[i])) { //Make sure enchantment id exists
                    player->ApplyEnchantment(item, EnchantmentSlot(slotEnch[i]), false);
                    item->SetEnchantment(EnchantmentSlot(slotEnch[i]), slotRand[i], 0, 0);
                    player->ApplyEnchantment(item, EnchantmentSlot(slotEnch[i]), true);
                }
            }
        }
        ChatHandler chathandle = ChatHandler(player->GetSession());
        if (slotRand[2] != -1)
            chathandle.PSendSysMessage("%s 获得|cffFF0000 3 |r条随机附魔!", GetItemLink(item->GetEntry(), player->GetSession()));
        else if (slotRand[1] != -1)
            chathandle.PSendSysMessage("%s 获得|cffFF0000 2 |r条随机附魔!", GetItemLink(item->GetEntry(), player->GetSession()));
        else if (slotRand[0] != -1)
            chathandle.PSendSysMessage("%s 获得|cffFF0000 1 |r条随机附魔!", GetItemLink(item->GetEntry(), player->GetSession()));
    }

    int getRandEnchantment(Item* item) {
        uint32 Class = item->GetTemplate()->Class;
        std::string ClassQueryString = "";
        switch (Class) {
        case 2:
            ClassQueryString = "WEAPON";
            break;
        case 4:
            ClassQueryString = "ARMOR";
            break;
        }
        if (ClassQueryString == "")
            return -1;
        uint32 Quality = item->GetTemplate()->Quality;
        int rarityRoll = -1;
        switch (Quality) {
        case GREY:
            rarityRoll = rand_norm() * 25;
            break;
        case WHITE:
            rarityRoll = rand_norm() * 50;
            break;
        case GREEN:
            rarityRoll = 45 + (rand_norm() * 20);
            break;
        case BLUE:
            rarityRoll = 65 + (rand_norm() * 15);
            break;
        case PURPLE:
            rarityRoll = 80 + (rand_norm() * 14);
            break;
        case ORANGE:
            rarityRoll = 93;
            break;
        }
        if (rarityRoll < 0)
            return -1;
        int tier = 0;
        if (rarityRoll <= 44)
            tier = 1;
        else if (rarityRoll <= 64)
            tier = 2;
        else if (rarityRoll <= 79)
            tier = 3;
        else if (rarityRoll <= 92)
            tier = 4;
        else
            tier = 5;

        QueryResult qr = WorldDatabase.Query("SELECT enchantID FROM item_enchantment_random_tiers WHERE tier='{}' AND exclusiveSubClass=NULL AND class='{}' OR exclusiveSubClass='{}' OR class='ANY' ORDER BY RAND() LIMIT 1", tier, ClassQueryString, item->GetTemplate()->SubClass);
        return qr->Fetch()[0].Get<uint32>();
    }
};

void AddRandomEnchantsScripts() {
    new RandomEnchantsPlayer();
}


