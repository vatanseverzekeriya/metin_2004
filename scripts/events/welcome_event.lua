--[[
    Hoşgeldin Eventi
    Oyuncu ilk giriş yaptığında çalışır
]]--

-- Event başlangıç fonksiyonu
function on_login(player)
    SendMessage(player, "Metin2 PvP Server'a hoş geldin!")
    SendMessage(player, "Seviye: " .. GetPlayerLevel(player))

    -- Başlangıç bonusu
    local level = GetPlayerLevel(player)
    if level == 1 then
        SendMessage(player, "Yeni oyuncu bonusu alıyorsun!")
        GiveExp(player, 1000)
        GiveGold(player, 10000)
        GiveItem(player, 27001, 10)  -- İksir x10
        SendMessage(player, "+1000 EXP, +10000 Yang, +10 İksir kazandın!")
    end

    -- Speed buff ver
    AddAffect(player, 5, 30, 300000)  -- AFFECT_SPEED_BOOST, %30, 5 dakika
    SendMessage(player, "5 dakikalık hız bonusu aldın!")
end

-- Seviye atlama eventi
function on_level_up(player, new_level)
    SendMessage(player, "Tebrikler! Seviye " .. new_level .. " oldun!")

    -- Her 10 seviyede özel bonus
    if new_level % 10 == 0 then
        local bonus_gold = new_level * 1000
        GiveGold(player, bonus_gold)
        SendMessage(player, "Seviye " .. new_level .. " bonusu: " .. bonus_gold .. " Yang!")

        -- Attack boost buff
        AddAffect(player, 3, 50, 600000)  -- AFFECT_ATTACK_BOOST, +50, 10 dakika
        SendMessage(player, "10 dakikalık saldırı bonusu aldın!")
    end

    -- Seviye 50'de özel ödül
    if new_level == 50 then
        SendMessage(player, "========================================")
        SendMessage(player, "  SEVİYE 50 ÖZEL ÖDÜLÜ!")
        SendMessage(player, "========================================")
        GiveGold(player, 1000000)
        GiveItem(player, 11209, 1)  -- +9 Kılıç
        SendMessage(player, "+1.000.000 Yang ve +9 Kılıç kazandın!")
    end
end

print("welcome_event.lua loaded successfully")
