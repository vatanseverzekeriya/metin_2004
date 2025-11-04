-- PvP Eğitim Quest'i
-- Oyunculara PvP mekaniklerini öğretir

quest pvp_tutorial begin
    state start begin
        when login begin
            if pc.get_level() >= 10 then
                pc.notice("[PvP Eğitimi] Yeni bir görev mevcut!")
            end
        end

        when button or click begin
            say("PvP Eğitimi")
            say("")
            say("Merhaba " .. pc.get_name() .. "!")
            say("PvP savaşları hakkında bilgi almak ister misin?")
            say("")

            local selection = select(
                "Evet, öğrenmek istiyorum",
                "Hayır, teşekkürler"
            )

            if selection == 1 then
                say("Harika! Başlayalım.")
                say("")
                say("PvP (Player vs Player) savaşları")
                say("en heyecanlı özelliklerimizden biri.")
                say("")
                set_state(learning)
            end
        end
    end

    state learning begin
        when button or click begin
            say("PvP Modları")
            say("")
            say("1. Normal Mod: Tüm oyuncularla savaşabilirsin")
            say("2. Guild Modu: Sadece düşman guild'lerle")
            say("3. Party Modu: Party dışındakilerle")
            say("")

            local selection = select(
                "Savaş İstatistiklerimi Gör",
                "PvP İpuçları",
                "Ödüllerimi Al",
                "Kapat"
            )

            if selection == 1 then
                say("Savaş İstatistiklerin")
                say("")
                say("Seviye: " .. pc.get_level())
                say("Can: " .. pc.get_hp())
                say("Yang: " .. pc.get_gold())
                say("")
                say("Güçlenmek için seviye atla!")

            elseif selection == 2 then
                say("PvP İpuçları")
                say("")
                say("1. Her zaman tam canlı savaş")
                say("2. Düşmanının seviyesine dikkat et")
                say("3. Ekipmanlarını geliştir")
                say("4. Becerilerini akıllıca kullan")
                say("5. Pozisyonunu iyi ayarla")
                say("")

            elseif selection == 3 then
                say("Eğitim Ödülleri")
                say("")
                if pc.get_level() >= 15 then
                    say("Tebrikler! Seviye 15'e ulaştın!")
                    say("")
                    pc.change_gold(100000)
                    pc.give_exp(5000)
                    say("100,000 Yang kazandın!")
                    say("5,000 Deneyim puanı kazandın!")
                    say("")
                    set_state(completed)
                else
                    say("Seviye 15'e ulaşmalısın!")
                    say("Şu anki seviye: " .. pc.get_level())
                    say("Daha " .. (15 - pc.get_level()) .. " seviye!")
                end
            end
        end
    end

    state completed begin
        when login begin
            -- Quest tamamlandı
        end

        when button or click begin
            say("PvP Eğitimi Tamamlandı!")
            say("")
            say("Artık gerçek bir savaşçısın!")
            say("Arena'da başarılar dilerim.")
        end
    end
end
