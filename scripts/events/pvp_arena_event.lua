--[[
    PvP Arena Eventi
    PvP arenasında özel kurallar ve ödüller
]]--

-- Arena koordinatları
ARENA_CENTER_X = 950000
ARENA_CENTER_Y = 250000
ARENA_RADIUS = 5000

-- Ödül tablosu (kill sayısına göre)
rewards = {
    [1]  = {gold = 5000, exp = 500},
    [5]  = {gold = 30000, exp = 3000},
    [10] = {gold = 100000, exp = 10000},
    [20] = {gold = 300000, exp = 30000},
    [50] = {gold = 1000000, exp = 100000}
}

-- Arena'ya giriş
function on_arena_enter(player)
    SendMessage(player, "========================================")
    SendMessage(player, "  PvP ARENA'YA HOŞGELDİN!")
    SendMessage(player, "========================================")
    SendMessage(player, "Kurallar:")
    SendMessage(player, "- Ölüm cezası yok")
    SendMessage(player, "- Her kill için ödül kazanırsın")
    SendMessage(player, "- Belirli kill sayılarında bonus!")

    -- HP ve SP doldur
    local max_hp = GetPlayerHP(player)  -- Max HP almak için önce current HP'yi kullanıyoruz
    SetPlayerHP(player, max_hp)

    -- Buff ver
    AddAffect(player, 3, 100, 3600000)  -- Attack boost, +100, 1 saat
    AddAffect(player, 4, 100, 3600000)  -- Defense boost, +100, 1 saat
    SendMessage(player, "Arena buffları verildi!")
end

-- Arena'dan çıkış
function on_arena_leave(player)
    SendMessage(player, "Arena'dan ayrıldın!")

    -- Buffları kaldır
    RemoveAffect(player, 3)  -- Attack boost
    RemoveAffect(player, 4)  -- Defense boost
end

-- Kill eventi
function on_arena_kill(killer, victim, kill_count)
    SendMessage(killer, "Kill! Toplam: " .. kill_count)

    -- Her kill için temel ödül
    GiveGold(killer, 5000)
    GiveExp(killer, 500)

    -- Özel ödüller
    if rewards[kill_count] then
        local reward = rewards[kill_count]
        SendMessage(killer, "========================================")
        SendMessage(killer, "  " .. kill_count .. " KILL BONUSU!")
        SendMessage(killer, "========================================")
        GiveGold(killer, reward.gold)
        GiveExp(killer, reward.exp)
        SendMessage(killer, "+" .. reward.gold .. " Yang, +" .. reward.exp .. " EXP!")

        -- 10 kill'de özel item
        if kill_count == 10 then
            GiveItem(killer, 50300, 1)  -- Savunma potası
            SendMessage(killer, "+1 Savunma Potası!")
        end

        -- 20 kill'de daha güçlü buff
        if kill_count == 20 then
            AddAffect(killer, 5, 50, 600000)  -- Speed boost
            SendMessage(killer, "10 dakikalık hız bonusu!")
        end

        -- 50 kill'de şampiyonluk
        if kill_count == 50 then
            SendMessage(killer, "========================================")
            SendMessage(killer, "  ARENA ŞAMPİYONU!")
            SendMessage(killer, "========================================")
            AddAffect(killer, 22, 1, 300000)  -- Invincible, 5 dakika
            SendMessage(killer, "5 dakika ölümsüzlük kazandın!")
        end
    end

    -- Kill streak mesajları
    if kill_count % 5 == 0 then
        SendMessage(killer, ">>> " .. kill_count .. " KILL STREAK! <<<")
    end

    -- Victim'e teselli
    SendMessage(victim, "Arenada öldün, ama endişelenme ceza yok!")
    SendMessage(victim, "Hemen yeniden dene!")
end

-- Respawn eventi
function on_arena_respawn(player)
    -- HP ve SP doldur
    local max_hp = GetPlayerHP(player)
    SetPlayerHP(player, max_hp)

    -- Spawn noktasına ışınla
    TeleportPlayer(player, ARENA_CENTER_X, ARENA_CENTER_Y)

    SendMessage(player, "Yeniden canlandın!")

    -- Kısa süreli koruma
    AddAffect(player, 22, 1, 5000)  -- 5 saniye ölümsüzlük
    SendMessage(player, "5 saniye koruma süren var!")
end

-- Daily quest: 10 kill
function on_arena_daily_quest(player, kill_count)
    if kill_count >= 10 then
        SendMessage(player, "Günlük görev tamamlandı!")
        SetQuestFlag(player, "arena_daily_done", 1)

        -- Günlük ödül
        GiveGold(player, 500000)
        GiveExp(player, 50000)
        GiveItem(player, 70024, 1)  -- Ejderha Tanrı Kitabı
        SendMessage(player, "Günlük ödül: +500K Yang, +50K EXP, +1 Kitap!")
    end
end

-- Boss spawn eventi (her saat)
function on_arena_boss_spawn()
    SendMessage(nil, "========================================")
    SendMessage(nil, "  ARENA BOSS SPAWN!")
    SendMessage(nil, "  Lokasyon: Arena Merkez")
    SendMessage(nil, "========================================")

    SpawnMonster(2493, ARENA_CENTER_X, ARENA_CENTER_Y)  -- Azrael

    -- Boss öldüren oyuncuya büyük ödül verilecek
end

-- Yardımcı fonksiyon: Oyuncu arenada mı?
function is_in_arena(player, x, y)
    local dx = x - ARENA_CENTER_X
    local dy = y - ARENA_CENTER_Y
    local distance = math.sqrt(dx*dx + dy*dy)

    return distance <= ARENA_RADIUS
end

print("pvp_arena_event.lua loaded successfully")
