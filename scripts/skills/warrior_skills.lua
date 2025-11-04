--[[
    Savaşçı Yetenekleri (Warrior Skills)
    Metin2 savaşçı sınıfı için skill tanımları
]]--

-- Skill ID'leri
SKILL_THREE_WAY_CUT = 1      -- Üçlü Kesim
SKILL_SWORD_STRIKE = 2       -- Kılıç Darbesi
SKILL_BASH = 3               -- Sarsıntı
SKILL_DASH = 4               -- Hücum
SKILL_STRONG_BODY = 16       -- Güçlü Beden (Pasif)

-- Skill Three Way Cut (Üçlü Kesim)
function skill_three_way_cut(player, target)
    local level = GetPlayerLevel(player)

    -- SP kontrolü
    local sp_cost = 30
    local current_sp = GetPlayerSP(player)

    if current_sp < sp_cost then
        SendMessage(player, "Yetersiz SP!")
        return false
    end

    -- Menzil kontrolü
    if not is_in_range(player, target, 200) then
        SendMessage(player, "Hedef çok uzak!")
        return false
    end

    -- SP tüket
    -- DecreaseSP(player, sp_cost)

    -- Hasar hesapla
    local base_damage = 150
    local level_bonus = level * 5
    local total_damage = base_damage + level_bonus

    -- Hasarı uygula
    -- DealDamage(player, target, total_damage)

    SendMessage(player, "Üçlü Kesim! " .. total_damage .. " hasar!")

    -- Alan hasarı (yakındaki düşmanlara)
    -- ApplyAreaDamage(player, target_pos, 300, total_damage * 0.7)

    -- Cooldown
    -- SetSkillCooldown(player, SKILL_THREE_WAY_CUT, 3000)  -- 3 saniye

    return true
end

-- Skill Sword Strike (Kılıç Darbesi)
function skill_sword_strike(player, target)
    local sp_cost = 50

    -- Güçlü tek hedef saldırısı
    local base_damage = 300
    local critical_chance = 30  -- %30 kritik şansı

    local roll = math.random(1, 100)
    local is_critical = roll <= critical_chance

    local damage = base_damage
    if is_critical then
        damage = damage * 2
        SendMessage(player, "KRİTİK VURUŞ!")
    end

    SendMessage(player, "Kılıç Darbesi! " .. damage .. " hasar!")

    -- Hedefi 2 saniye yavaşlat
    AddAffect(target, 11, 50, 2000)  -- AFFECT_SLOW, %50, 2 saniye

    return true
end

-- Skill Bash (Sarsıntı)
function skill_bash(player, target)
    local sp_cost = 40

    -- Hasar + Stun
    local damage = 100

    SendMessage(player, "Sarsıntı! " .. damage .. " hasar!")

    -- 2 saniye stun
    AddAffect(target, 12, 1, 2000)  -- AFFECT_STUN, 2 saniye
    SendMessage(target, "Sersemledin!")

    return true
end

-- Skill Dash (Hücum)
function skill_dash(player, target)
    local sp_cost = 35

    -- Hedefe dash yap ve hasar ver
    -- TeleportToTarget(player, target, 100)  -- 100 birim uzaklıkta

    local damage = 200

    SendMessage(player, "Hücum! " .. damage .. " hasar!")

    -- Kısa süreli hız bonusu
    AddAffect(player, 5, 50, 3000)  -- AFFECT_SPEED_BOOST, %50, 3 saniye

    return true
end

-- Skill Strong Body (Güçlü Beden - Pasif)
function skill_strong_body_passive(player)
    local level = GetPlayerLevel(player)
    local skill_level = GetSkillLevel(player, SKILL_STRONG_BODY)

    if skill_level > 0 then
        -- Her skill seviyesi için %2 HP artışı
        local hp_bonus_percent = skill_level * 2

        -- Sürekli defense bonusu
        local defense_bonus = skill_level * 10

        -- Buff ekle (permanent)
        AddAffect(player, 4, defense_bonus, 999999000)  -- AFFECT_DEFENSE_BOOST

        SendMessage(player, "Güçlü Beden aktif: +" .. defense_bonus .. " Savunma")
    end
end

-- Combo sistemi
combo_state = {}

function register_combo_hit(player, skill_id)
    if not combo_state[player] then
        combo_state[player] = {
            skills = {},
            last_time = 0
        }
    end

    local state = combo_state[player]
    local current_time = os.time()

    -- 3 saniye içinde combo devam eder
    if current_time - state.last_time > 3 then
        -- Combo sıfırla
        state.skills = {}
    end

    table.insert(state.skills, skill_id)
    state.last_time = current_time

    -- Combo kontrol et
    check_combo(player, state.skills)
end

function check_combo(player, skills)
    local combo_length = #skills

    -- 3 hit combo
    if combo_length == 3 then
        SendMessage(player, "========================================")
        SendMessage(player, "  3 HIT COMBO!")
        SendMessage(player, "  +%50 Hasar bonusu!")
        SendMessage(player, "========================================")

        -- Bonus ver
        AddAffect(player, 3, 50, 5000)  -- +%50 Attack, 5 saniye
    end

    -- 5 hit combo
    if combo_length == 5 then
        SendMessage(player, "========================================")
        SendMessage(player, "  5 HIT MEGA COMBO!")
        SendMessage(player, "  +%100 Hasar bonusu!")
        SendMessage(player, "========================================")

        AddAffect(player, 3, 100, 10000)  -- +%100 Attack, 10 saniye

        -- Bonus ödül
        GiveExp(player, 1000)
        SendMessage(player, "+1000 EXP (Combo Bonusu)")
    end

    -- 10 hit combo
    if combo_length == 10 then
        SendMessage(player, "========================================")
        SendMessage(player, "  10 HIT ULTRA COMBO!!!")
        SendMessage(player, "  EFSANE BONUS!")
        SendMessage(player, "========================================")

        -- Çılgın bonuslar
        AddAffect(player, 3, 200, 20000)   -- +%200 Attack
        AddAffect(player, 5, 100, 20000)   -- +%100 Speed
        AddAffect(player, 1, 500, 20000)   -- +500 HP Regen

        GiveExp(player, 10000)
        GiveGold(player, 100000)

        SendMessage(player, "+10000 EXP, +100000 Yang!")
    end
end

-- Yardımcı fonksiyonlar
function is_in_range(player, target, range)
    -- Gerçek implementasyonda mesafe hesaplanır
    return true
end

function GetPlayerSP(player)
    -- SP değerini döndür
    return 1000
end

function GetSkillLevel(player, skill_id)
    -- Skill seviyesini döndür
    local flag_name = "skill_level_" .. skill_id
    return GetQuestFlag(player, flag_name)
end

print("warrior_skills.lua loaded successfully")
