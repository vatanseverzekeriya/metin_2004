#ifndef __INC_GAME_COMBAT_H__
#define __INC_GAME_COMBAT_H__

#include "../common/types.h"
#include <string>
#include <vector>
#include <map>
#include <chrono>

// Forward declaration
class CCharacter;

// Hasar türleri
enum EDamageType
{
    DAMAGE_TYPE_PHYSICAL = 0,   // Fiziksel hasar
    DAMAGE_TYPE_MAGICAL = 1,    // Büyü hasarı
    DAMAGE_TYPE_PURE = 2,       // Saf hasar (savunma etkisiz)
    DAMAGE_TYPE_CRITICAL = 3    // Kritik vuruş
};

// Skill türleri
enum ESkillType
{
    SKILL_TYPE_ATTACK = 0,      // Saldırı skilli
    SKILL_TYPE_BUFF = 1,        // Buff skilli
    SKILL_TYPE_HEAL = 2,        // İyileştirme skilli
    SKILL_TYPE_DEBUFF = 3       // Debuff skilli
};

// Skill hedef türleri
enum ESkillTargetType
{
    SKILL_TARGET_ENEMY = 0,     // Düşman
    SKILL_TARGET_SELF = 1,      // Kendisi
    SKILL_TARGET_ALLY = 2,      // Müttefik
    SKILL_TARGET_AREA = 3       // Alan etkisi
};

// Hasar bilgisi
struct TDamageInfo
{
    CCharacter* attacker;
    CCharacter* victim;
    DWORD damage;
    EDamageType damage_type;
    bool is_critical;
    DWORD skill_vnum;  // 0 = normal attack

    TDamageInfo()
        : attacker(nullptr)
        , victim(nullptr)
        , damage(0)
        , damage_type(DAMAGE_TYPE_PHYSICAL)
        , is_critical(false)
        , skill_vnum(0)
    {}
};

// Skill proto
struct TSkillProto
{
    DWORD vnum;
    std::string name;
    ESkillType type;
    ESkillTargetType target_type;

    DWORD damage_base;          // Temel hasar
    float damage_multiplier;    // Hasar çarpanı
    DWORD sp_cost;              // SP maliyeti
    DWORD cooldown_ms;          // Cooldown (milisaniye)
    DWORD cast_time_ms;         // Cast süresi
    DWORD range;                // Menzil
    DWORD area_range;           // Alan etkisi yarıçapı (0 = tek hedef)

    // Buff/Debuff için
    DWORD duration_ms;          // Etki süresi
    DWORD affect_value;         // Etki değeri

    TSkillProto()
        : vnum(0)
        , type(SKILL_TYPE_ATTACK)
        , target_type(SKILL_TARGET_ENEMY)
        , damage_base(0)
        , damage_multiplier(1.0f)
        , sp_cost(0)
        , cooldown_ms(1000)
        , cast_time_ms(0)
        , range(150)
        , area_range(0)
        , duration_ms(0)
        , affect_value(0)
    {}
};

// Skill cooldown bilgisi
struct TSkillCooldown
{
    DWORD vnum;
    std::chrono::steady_clock::time_point last_use_time;
    DWORD cooldown_ms;

    bool IsReady() const
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_use_time).count();
        return elapsed >= cooldown_ms;
    }

    DWORD GetRemainingCooldown() const
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_use_time).count();

        if (elapsed >= cooldown_ms)
            return 0;

        return cooldown_ms - (DWORD)elapsed;
    }
};

// Combat sistem sınıfı
class CCombatSystem
{
public:
    static CCombatSystem& Instance();

    // Skill sistemi
    bool LoadSkillProtos(const std::string& filename);
    const TSkillProto* GetSkillProto(DWORD vnum) const;
    void RegisterSkill(const TSkillProto& skill);

    // Hasar hesaplama
    TDamageInfo CalculateDamage(CCharacter* attacker, CCharacter* victim, DWORD skill_vnum = 0);
    void ApplyDamage(const TDamageInfo& damage_info);

    // Kritik hesaplama
    bool IsCriticalHit(CCharacter* attacker, CCharacter* victim);
    float GetCriticalMultiplier() const { return 1.5f; }

    // Skill kullanımı
    bool CanUseSkill(CCharacter* ch, DWORD skill_vnum, CCharacter* target);
    bool UseSkill(CCharacter* ch, DWORD skill_vnum, CCharacter* target);

    // Cooldown yönetimi
    void SetCooldown(CCharacter* ch, DWORD skill_vnum, DWORD cooldown_ms);
    bool IsOnCooldown(CCharacter* ch, DWORD skill_vnum);
    DWORD GetRemainingCooldown(CCharacter* ch, DWORD skill_vnum);

    // Alan hasarı
    void ApplyAreaDamage(CCharacter* attacker, const TPosition& center, DWORD range, DWORD skill_vnum);

private:
    CCombatSystem();
    ~CCombatSystem();

    CCombatSystem(const CCombatSystem&) = delete;
    CCombatSystem& operator=(const CCombatSystem&) = delete;

    std::map<DWORD, TSkillProto> m_mapSkillProtos;
    std::map<CCharacter*, std::map<DWORD, TSkillCooldown>> m_mapCooldowns;

    DWORD CalculatePhysicalDamage(CCharacter* attacker, CCharacter* victim);
    DWORD CalculateMagicalDamage(CCharacter* attacker, CCharacter* victim);

    void ApplySkillEffect(CCharacter* caster, CCharacter* target, const TSkillProto* skill);
};

#endif // __INC_GAME_COMBAT_H__
