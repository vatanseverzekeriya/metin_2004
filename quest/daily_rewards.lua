-- Günlük Ödüller Quest'i
-- Oyuncular her gün giriş yaptığında ödül alır

quest daily_rewards begin
    state start begin
        when login begin
            local today = os.date("%Y-%m-%d")
            local last_reward = get_global_time("last_daily_reward")

            if last_reward ~= today then
                pc.notice("===================")
                pc.notice("Günlük ödülün hazır!")
                pc.notice("===================")
                set_state(claim)
            else
                pc.notice("Günlük ödülünü zaten aldın!")
            end
        end
    end

    state claim begin
        when button or click begin
            say("Günlük Ödüller")
            say("")
            say("Her gün giriş yaparak ödül kazanabilirsin!")
            say("")

            local player_level = pc.get_level()
            local gold_reward = player_level * 1000
            local exp_reward = player_level * 100

            say("Bugünkü Ödüllerin:")
            say("Yang: " .. gold_reward)
            say("Deneyim: " .. exp_reward)
            say("")

            local selection = select(
                "Ödülü Al",
                "Sonra Al"
            )

            if selection == 1 then
                -- Ödülleri ver
                pc.change_gold(gold_reward)
                pc.give_exp(exp_reward)

                pc.notice("Günlük ödüllerinizi aldınız!")
                pc.notice(gold_reward .. " Yang kazandınız!")
                pc.notice(exp_reward .. " Deneyim kazandınız!")

                -- Son ödül zamanını kaydet
                local today = os.date("%Y-%m-%d")
                set_global_time("last_daily_reward", today)

                -- Bonus ödül kontrolü (her 7 günde bir)
                local login_days = get_global_count("total_login_days") or 0
                login_days = login_days + 1
                set_global_count("total_login_days", login_days)

                if login_days % 7 == 0 then
                    say("=== ÖZEL ÖDÜL ===")
                    say("")
                    say("7 gün üst üste giriş yaptın!")
                    say("Bonus ödül kazandın!")
                    say("")

                    local bonus_gold = gold_reward * 5
                    local bonus_exp = exp_reward * 5

                    pc.change_gold(bonus_gold)
                    pc.give_exp(bonus_exp)

                    pc.notice("BONUS: " .. bonus_gold .. " Yang!")
                    pc.notice("BONUS: " .. bonus_exp .. " Deneyim!")
                end

                set_state(start)
            end
        end
    end
end

-- Yardımcı fonksiyonlar
function get_global_time(key)
    -- Gerçek implementasyonda database'den okunur
    -- Şimdilik basit bir çözüm
    return nil
end

function set_global_time(key, value)
    -- Gerçek implementasyonda database'e yazılır
end

function get_global_count(key)
    -- Gerçek implementasyonda database'den okunur
    return 0
end

function set_global_count(key, value)
    -- Gerçek implementasyonda database'e yazılır
end
