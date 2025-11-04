--[[
    Başlangıç Görevi
    Yeni oyuncular için basit görev zinciri
]]--

quest beginner_quest begin
    state start begin
        -- Quest başlangıç
        when login begin
            local level = GetPlayerLevel(player)

            if level <= 5 and GetQuestFlag(player, "beginner_quest_started") == 0 then
                SendMessage(player, "========================================")
                SendMessage(player, "  YENİ GÖREV: İlk Adımlar")
                SendMessage(player, "========================================")
                SendMessage(player, "Hoşgeldin genç savaşçı!")
                SendMessage(player, "Görev: 5 canavar öldür")

                SetQuestFlag(player, "beginner_quest_started", 1)
                SetQuestFlag(player, "beginner_kills", 0)
            end
        end

        -- Canavar öldürme
        when kill begin
            local quest_started = GetQuestFlag(player, "beginner_quest_started")
            local quest_completed = GetQuestFlag(player, "beginner_quest_completed")

            if quest_started == 1 and quest_completed == 0 then
                local kills = GetQuestFlag(player, "beginner_kills") + 1
                SetQuestFlag(player, "beginner_kills", kills)

                SendMessage(player, "İlerleme: " .. kills .. "/5 canavar öldürüldü")

                if kills >= 5 then
                    -- Görev tamamlandı
                    SendMessage(player, "========================================")
                    SendMessage(player, "  GÖREV TAMAMLANDI!")
                    SendMessage(player, "========================================")
                    SendMessage(player, "Tebrikler! İlk görevini tamamladın!")

                    -- Ödüller
                    GiveExp(player, 5000)
                    GiveGold(player, 50000)
                    GiveItem(player, 27001, 20)  -- İksir x20
                    GiveItem(player, 27002, 20)  -- Azrael'in Gözyaşı x20

                    SendMessage(player, "Ödüller:")
                    SendMessage(player, "+5000 EXP")
                    SendMessage(player, "+50000 Yang")
                    SendMessage(player, "+20 HP İksiri")
                    SendMessage(player, "+20 SP İksiri")

                    SetQuestFlag(player, "beginner_quest_completed", 1)

                    -- Sonraki görevi aç
                    set_state(next_quest)
                end
            end
        end
    end

    state next_quest begin
        when login begin
            local next_started = GetQuestFlag(player, "next_quest_started")

            if next_started == 0 then
                SendMessage(player, "========================================")
                SendMessage(player, "  YENİ GÖREV: Güçlenme Zamanı")
                SendMessage(player, "========================================")
                SendMessage(player, "Görev: Seviye 10'a ulaş")

                SetQuestFlag(player, "next_quest_started", 1)
            end
        end

        when level_up begin
            local level = GetPlayerLevel(player)

            if level >= 10 then
                SendMessage(player, "========================================")
                SendMessage(player, "  GÖREV TAMAMLANDI!")
                SendMessage(player, "========================================")

                -- Büyük ödül
                GiveExp(player, 50000)
                GiveGold(player, 500000)
                GiveItem(player, 11209, 1)  -- +9 Silah

                SendMessage(player, "Ödüller:")
                SendMessage(player, "+50000 EXP")
                SendMessage(player, "+500000 Yang")
                SendMessage(player, "+9 Silah")

                -- Özel buff
                AddAffect(player, 3, 100, 3600000)  -- Attack +100, 1 saat
                SendMessage(player, "1 saatlik saldırı bonusu!")

                set_state(final_quest)
            end
        end
    end

    state final_quest begin
        when login begin
            local final_started = GetQuestFlag(player, "final_quest_started")

            if final_started == 0 then
                SendMessage(player, "========================================")
                SendMessage(player, "  FINAL GÖREVI: PvP Deneyimi")
                SendMessage(player, "========================================")
                SendMessage(player, "Görev: 3 oyuncu öldür")

                SetQuestFlag(player, "final_quest_started", 1)
                SetQuestFlag(player, "pvp_kills", 0)
            end
        end

        when player_kill begin
            local kills = GetQuestFlag(player, "pvp_kills") + 1
            SetQuestFlag(player, "pvp_kills", kills)

            SendMessage(player, "PvP İlerleme: " .. kills .. "/3")

            if kills >= 3 then
                SendMessage(player, "========================================")
                SendMessage(player, "  TÜM GÖREVLER TAMAMLANDI!")
                SendMessage(player, "  Artık gerçek bir savaşçısın!")
                SendMessage(player, "========================================")

                -- Final ödülleri
                GiveExp(player, 100000)
                GiveGold(player, 2000000)
                GiveItem(player, 11429, 1)  -- +9 Zırh
                GiveItem(player, 70024, 10)  -- Ejderha Kitabı x10

                SendMessage(player, "Final Ödülleri:")
                SendMessage(player, "+100000 EXP")
                SendMessage(player, "+2000000 Yang")
                SendMessage(player, "+9 Zırh")
                SendMessage(player, "+10 Ejderha Kitabı")

                -- Özel unvan
                SetQuestFlag(player, "title_beginner_complete", 1)
                SendMessage(player, "'Acemi Savaşçı' unvanı kazandın!")

                set_state(__COMPLETE__)
            end
        end
    end

    state __COMPLETE__ begin
        -- Quest tamamen tamamlandı
        when login begin
            -- Hiçbir şey yapma, quest bitti
        end
    end
end
