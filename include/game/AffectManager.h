#ifndef __INC_GAME_AFFECT_MANAGER_H__
#define __INC_GAME_AFFECT_MANAGER_H__

#include "../common/types.h"
#include <string>
#include <vector>
#include <chrono>

// Forward declaration
class CCharacter;

// Affect (Buff/Debuff) türleri
enum EAffectType
{
    AFFECT_NONE = 0,

    // Stat buffs
    AFFECT_HP_REGEN = 1,        // HP rejenerasyonu
    AFFECT_SP_REGEN = 2,        // SP rejenerasyonu
    AFFECT_ATTACK_BOOST = 3,    // Saldırı artışı
    AFFECT_DEFENSE_BOOST = 4,   // Savunma artışı
    AFFECT_SPEED_BOOST = 5,     // Hız artışı

    // Debuffs
    AFFECT_POISON = 10,         // Zehir (DoT)
    AFFECT_SLOW = 11,           // Yavaşlama
    AFFECT_STUN = 12,           // Sersemletme
    AFFECT_SILENCE = 13,        // Sessizlik (skill kullanılamaz)
    AFFECT_BLIND = 14,          // Kör (miss chance artışı)

    // Özel effectler
    AFFECT_SHIELD = 20,         // Kalkan (hasar emme)
    AFFECT_INVISIBILITY = 21,   // Görünmezlik
    AFFECT_INVINCIBLE = 22,     // Ölümsüzlük (kısa süre)

    AFFECT_MAX_NUM
};

// Affect bilgisi
struct TAffect
{
    EAffectType type;
    DWORD value;                // Etki değeri (örn: +50 attack)
    DWORD duration_ms;          // Toplam süre
    std::chrono::steady_clock::time_point start_time;
    CCharacter* caster;         // Kim uyguladı

    TAffect()
        : type(AFFECT_NONE)
        , value(0)
        , duration_ms(0)
        , caster(nullptr)
    {}

    // Kalan süre
    DWORD GetRemainingTime() const
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - start_time).count();

        if (elapsed >= duration_ms)
            return 0;

        return duration_ms - (DWORD)elapsed;
    }

    // Süresi doldu mu?
    bool IsExpired() const
    {
        return GetRemainingTime() == 0;
    }
};

// Rejenerasyon bilgisi
struct TRegenInfo
{
    DWORD hp_regen_per_sec;     // Saniyede HP rejeni
    DWORD sp_regen_per_sec;     // Saniyede SP rejeni
    float hp_regen_percent;     // % HP rejeni
    float sp_regen_percent;     // % SP rejeni

    std::chrono::steady_clock::time_point last_regen_time;

    TRegenInfo()
        : hp_regen_per_sec(10)
        , sp_regen_per_sec(5)
        , hp_regen_percent(0.01f)   // %1
        , sp_regen_percent(0.02f)   // %2
    {
        last_regen_time = std::chrono::steady_clock::now();
    }
};

// Affect Manager sınıfı
class CAffectManager
{
public:
    CAffectManager(CCharacter* owner);
    ~CAffectManager();

    // Affect işlemleri
    bool AddAffect(EAffectType type, DWORD value, DWORD duration_ms, CCharacter* caster = nullptr);
    bool RemoveAffect(EAffectType type);
    void RemoveAllAffects();

    // Affect kontrolü
    bool HasAffect(EAffectType type) const;
    const TAffect* GetAffect(EAffectType type) const;
    const std::vector<TAffect>& GetAllAffects() const { return m_vecAffects; }

    // Güncelleme
    void Update(float delta_time);

    // Rejenerasyon
    void UpdateRegeneration(float delta_time);
    void SetRegenRate(DWORD hp_per_sec, DWORD sp_per_sec);

    // Stat modifierleri
    LONG GetAttackBonus() const { return m_lAttackBonus; }
    LONG GetDefenseBonus() const { return m_lDefenseBonus; }
    float GetSpeedMultiplier() const { return m_fSpeedMultiplier; }

    // Durum kontrolleri
    bool IsStunned() const { return HasAffect(AFFECT_STUN); }
    bool IsSilenced() const { return HasAffect(AFFECT_SILENCE); }
    bool IsInvisible() const { return HasAffect(AFFECT_INVISIBILITY); }
    bool IsInvincible() const { return HasAffect(AFFECT_INVINCIBLE); }

private:
    CCharacter* m_owner;
    std::vector<TAffect> m_vecAffects;
    TRegenInfo m_regenInfo;

    // Hesaplanan bonuslar (cache)
    LONG m_lAttackBonus;
    LONG m_lDefenseBonus;
    float m_fSpeedMultiplier;

    void RecalculateBonuses();
    void ApplyAffectEffect(const TAffect& affect);
    void RemoveAffectEffect(const TAffect& affect);

    void ProcessPoison(const TAffect& affect, float delta_time);
    void ProcessDoT(const TAffect& affect, float delta_time);
};

#endif // __INC_GAME_AFFECT_MANAGER_H__
