-- Hoş Geldin Quest'i
-- Yeni oyuncular için başlangıç görevi

quest welcome_quest begin
    state start begin
        when login begin
            local player_name = pc.get_name()
            local player_level = pc.get_level()

            pc.notice("Hoş geldin " .. player_name .. "!")
            pc.notice("Metin2 PvP Sunucusuna katıldınız!")

            if player_level == 1 then
                -- Başlangıç ödülleri
                pc.notice("Başlangıç ödüllerinizi alıyorsunuz...")
                pc.change_gold(10000)
                pc.give_exp(500)

                pc.notice("10,000 Yang kazandınız!")
                pc.notice("500 Deneyim puanı kazandınız!")

                set_state(tutorial)
            end
        end
    end

    state tutorial begin
        when info begin
            pc.notice("=== PvP Eğitimi ===")
            pc.notice("Bu sunucu PvP odaklıdır.")
            pc.notice("Diğer oyuncularla savaşarak seviye atlayabilirsiniz!")
            pc.notice("Hazır olduğunuzda arena bölgesine gidin.")
        end

        when button or click begin
            local selection = select(
                "PvP Eğitimi",
                "Arena'ya Işınlan",
                "Ödül Al",
                "Kapat"
            )

            if selection == 1 then
                say("PvP Eğitimi")
                say("")
                say("Bu sunucuda oyuncular birbirleriyle")
                say("savaşarak deneyim ve ödül kazanır.")
                say("")
                say("Savaşırken:")
                say("- Düşmanınıza yaklaşın")
                say("- Saldırı tuşuna basın")
                say("- Can barınızı takip edin")
                say("")

            elseif selection == 2 then
                say("Arena'ya ışınlanıyorsunuz...")
                pc.teleport(957200, 244900)
                pc.notice("PvP Arena'ya hoş geldiniz!")

            elseif selection == 3 then
                if pc.get_level() >= 5 then
                    pc.notice("Tebrikler! Seviye 5'e ulaştınız!")
                    pc.change_gold(50000)
                    pc.give_exp(2000)
                    pc.notice("Ödül: 50,000 Yang ve 2,000 EXP!")
                    set_state(completed)
                else
                    pc.notice("Seviye 5'e ulaşmanız gerekiyor!")
                    pc.notice("Şu anki seviyeniz: " .. pc.get_level())
                end
            end
        end
    end

    state completed begin
        when info begin
            pc.notice("Hoşgeldin quest'ini tamamladınız!")
            pc.notice("İyi oyunlar!")
        end
    end
end
