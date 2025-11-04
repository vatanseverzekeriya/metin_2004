--[[
    Boss Spawn Eventi
    Belirli bosslar için özel mekanikler ve ödüller
]]--

-- Boss bilgileri
bosses = {
    -- Azrael (Karanlık Ejderha)
    [2493] = {
        name = "Azrael",
        announce_spawn = true,
        announce_death = true,
        rewards = {
            gold = 10000000,
            exp = 1000000,
            items = {
                {vnum = 70024, count = 5},   -- Ejderha Tanrı Kitabı x5
                {vnum = 11209, count = 1},   -- +9 Kılıç
                {vnum = 50300, count = 10}   -- Savunma Potası x10
            }
        },
        special_drops = {
            {vnum = 90001, chance = 5},   -- Efsanevi Kalkan (%5)
            {vnum = 90002, chance = 2}    -- Ölümsüzlük Yüzüğü (%2)
        }
    },

    -- Tanrı Varyağı
    [6091] = {
        name = "Tanrı Varyağı",
        announce_spawn = true,
        announce_death = true,
        rewards = {
            gold = 50000000,
            exp = 5000000,
            items = {
                {vnum = 72723, count = 1},   -- Berserker Ruhları
                {vnum = 11429, count = 1}    -- +9 Zırh
            }
        }
    },

    -- Nemere
    [2094] = {
        name = "Nemere",
        announce_spawn = true,
        announce_death = true,
        rewards = {
            gold = 100000000,
            exp = 10000000,
            items = {
                {vnum = 71084, count = 10},  -- Nemere Kristali x10
                {vnum = 71124, count = 5}    -- Ejderha Pulu x5
            }
        },
        special_drops = {
            {vnum = 91001, chance = 1}   -- Nemere'nin Tacı (%1)
        }
    }
}

-- Boss spawn
function on_boss_spawn(monster_vnum, x, y)
    local boss = bosses[monster_vnum]
    if not boss then
        return
    end

    if boss.announce_spawn then
        -- Tüm sunucuya duyur
        SendMessage(nil, "========================================")
        SendMessage(nil, "  ⚔️  " .. boss.name .. " SPAWN OLDU!")
        SendMessage(nil, "  📍 Lokasyon: (" .. x .. ", " .. y .. ")")
        SendMessage(nil, "========================================")
    end

    -- Boss'a özel bufflar ver (gerçek implementasyonda)
    -- AddMonsterBuff(monster_vnum, ...)
end

-- Boss öldürme
function on_boss_kill(killer, monster_vnum)
    local boss = bosses[monster_vnum]
    if not boss then
        return
    end

    if boss.announce_death then
        SendMessage(nil, "========================================")
        SendMessage(nil, "  🏆 " .. boss.name .. " YENİLDİ!")
        SendMessage(nil, "  Öldüren: " .. GetPlayerName(killer))
        SendMessage(nil, "========================================")
    end

    -- Ödülleri ver
    SendMessage(killer, "========================================")
    SendMessage(killer, "  BOSS ÖDÜLLERİ")
    SendMessage(killer, "========================================")

    local rewards = boss.rewards

    -- Yang ve EXP
    GiveGold(killer, rewards.gold)
    GiveExp(killer, rewards.exp)
    SendMessage(killer, "+" .. rewards.gold .. " Yang")
    SendMessage(killer, "+" .. rewards.exp .. " EXP")

    -- Itemler
    for _, item in ipairs(rewards.items) do
        GiveItem(killer, item.vnum, item.count)
        SendMessage(killer, "+Item [" .. item.vnum .. "] x" .. item.count)
    end

    -- Özel droplar (şansa bağlı)
    if boss.special_drops then
        for _, drop in ipairs(boss.special_drops) do
            local roll = math.random(1, 100)
            if roll <= drop.chance then
                GiveItem(killer, drop.vnum, 1)
                SendMessage(killer, "========================================")
                SendMessage(killer, "  🌟 NADİR EŞYA DÜŞTÜ!")
                SendMessage(killer, "  Item: [" .. drop.vnum .. "]")
                SendMessage(killer, "========================================")

                -- Sunucuya duyur
                SendMessage(nil, "🌟 " .. GetPlayerName(killer) .. " boss'tan nadir eşya kazandı!")
            end
        end
    end

    -- Başarı sistemi
    local boss_kill_count = GetQuestFlag(killer, "boss_kill_" .. monster_vnum) + 1
    SetQuestFlag(killer, "boss_kill_" .. monster_vnum, boss_kill_count)

    if boss_kill_count == 1 then
        SendMessage(killer, "İlk " .. boss.name .. " kill'in!")
    elseif boss_kill_count == 10 then
        SendMessage(killer, "10. " .. boss.name .. " kill'in! Bonus ödül:")
        GiveGold(killer, rewards.gold * 2)
        SendMessage(killer, "+" .. (rewards.gold * 2) .. " Yang!")
    elseif boss_kill_count == 100 then
        SendMessage(killer, "100. " .. boss.name .. " kill'in! EFSANE ÖDÜL:")
        GiveItem(killer, 99001, 1)  -- Özel unvanı
        SendMessage(killer, "'" .. boss.name .. " Avcısı' unvanı kazandın!")
    end

    -- Party ödülleri (varsa)
    distribute_party_rewards(killer, rewards)
end

-- Party üyelerine ödül dağıt
function distribute_party_rewards(leader, rewards)
    local party_size = GetPartyMemberCount(leader)

    if party_size > 1 then
        SendMessage(leader, "Party ödülleri dağıtılıyor...")

        -- Her party üyesine düşük ödül
        local party_gold = math.floor(rewards.gold * 0.3)
        local party_exp = math.floor(rewards.exp * 0.3)

        -- Gerçek implementasyonda tüm party üyelerini döneriz
        -- for member in GetPartyMembers(leader) do
        --     GiveGold(member, party_gold)
        --     GiveExp(member, party_exp)
        -- end

        SendMessage(leader, "Party üyeleri ödül aldı!")
    end
end

-- Boss spawn zamanlayıcı (sunucu başlangıcında çalışacak)
function schedule_boss_spawns()
    -- Her 2 saatte Azrael
    schedule_event("spawn_azrael", 7200)  -- 2 saat = 7200 saniye

    -- Her 4 saatte Tanrı Varyağı
    schedule_event("spawn_deity", 14400)

    -- Her 6 saatte Nemere
    schedule_event("spawn_nemere", 21600)
end

-- Yardımcı fonksiyon
function GetPlayerName(player)
    -- Gerçek implementasyonda player'dan isim alınır
    return "Player"
end

-- Test fonksiyonu
function test_boss_spawn()
    on_boss_spawn(2493, 950000, 250000)
end

print("boss_spawn_event.lua loaded successfully")
