#include "../../include/game/Combat.h"
#include "../../include/game/Character.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>

CCombatSystem::CCombatSystem()
{
    // Varsayılan skilleri kaydet
    TSkillProto basic_attack;
    basic_attack.vnum = 0;
    basic_attack.name = "Basic Attack";
    basic_attack.type = SKILL_TYPE_ATTACK;
    basic_attack.target_type = SKILL_TARGET_ENEMY;
    basic_attack.damage_multiplier = 1.0f;
    basic_attack.cooldown_ms = 1000;
    basic_attack.range = 150;
    RegisterSkill(basic_attack);
}

CCombatSystem::~CCombatSystem()
{
}

CCombatSystem& CCombatSystem::Instance()
{
    static CCombatSystem instance;
    return instance;
}

void CCombatSystem::RegisterSkill(const TSkillProto& skill)
{
    m_mapSkillProtos[skill.vnum] = skill;
    std::cout << "Skill registered: " << skill.name << " (vnum: " << skill.vnum << ")" << std::endl;
}

const TSkillProto* CCombatSystem::GetSkillProto(DWORD vnum) const
{
    auto it = m_mapSkillProtos.find(vnum);
    if (it == m_mapSkillProtos.end())
        return nullptr;

    return &it->second;
}

DWORD CCombatSystem::CalculatePhysicalDamage(CCharacter* attacker, CCharacter* victim)
{
    if (!attacker || !victim)
        return 0;

    // Temel hasar: Saldırı - Savunma
    LONG damage = (LONG)attacker->GetAttack() - (LONG)victim->GetDefense();

    // Minimum hasar
    if (damage < 10)
        damage = 10;

    // Seviye farkı bonusu
    int level_diff = (int)attacker->GetLevel() - (int)victim->GetLevel();
    if (level_diff > 0)
        damage += level_diff * 5;

    // Rastgelelik (%80-120)
    int variance = (rand() % 41) - 20;
    damage = damage * (100 + variance) / 100;

    return (DWORD)damage;
}

DWORD CCombatSystem::CalculateMagicalDamage(CCharacter* attacker, CCharacter* victim)
{
    if (!attacker || !victim)
        return 0;

    // Büyü hasarı: Magic Attack - Magic Defense
    LONG damage = (LONG)attacker->GetMagicAttack() - (LONG)victim->GetMagicDefense();

    // Minimum hasar
    if (damage < 15)
        damage = 15;

    // Rastgelelik (%90-110)
    int variance = (rand() % 21) - 10;
    damage = damage * (100 + variance) / 100;

    return (DWORD)damage;
}

bool CCombatSystem::IsCriticalHit(CCharacter* attacker, CCharacter* victim)
{
    // Basit kritik hesaplama: %20 şans
    // Gelişmiş versiyonda level, stat vs. kullanılabilir
    int chance = rand() % 100;
    return chance < 20;
}

TDamageInfo CCombatSystem::CalculateDamage(CCharacter* attacker, CCharacter* victim, DWORD skill_vnum)
{
    TDamageInfo dmg_info;
    dmg_info.attacker = attacker;
    dmg_info.victim = victim;
    dmg_info.skill_vnum = skill_vnum;

    if (!attacker || !victim)
        return dmg_info;

    const TSkillProto* skill = GetSkillProto(skill_vnum);
    if (!skill)
        skill = GetSkillProto(0); // Basic attack

    // Hasar tipine göre hesapla
    DWORD base_damage = 0;

    if (skill->type == SKILL_TYPE_ATTACK)
    {
        // Fiziksel veya büyü hasarı belirle (skillden)
        if (skill_vnum == 0 || skill->damage_base == 0)
        {
            base_damage = CalculatePhysicalDamage(attacker, victim);
            dmg_info.damage_type = DAMAGE_TYPE_PHYSICAL;
        }
        else
        {
            // Skill hasarı
            base_damage = skill->damage_base;

            // Magic attack skill mi?
            if (skill->vnum >= 100 && skill->vnum < 200)
            {
                base_damage += attacker->GetMagicAttack();
                base_damage -= victim->GetMagicDefense() / 2;
                dmg_info.damage_type = DAMAGE_TYPE_MAGICAL;
            }
            else
            {
                base_damage += attacker->GetAttack();
                base_damage -= victim->GetDefense() / 2;
                dmg_info.damage_type = DAMAGE_TYPE_PHYSICAL;
            }
        }

        // Skill multiplier uygula
        base_damage = (DWORD)(base_damage * skill->damage_multiplier);

        // Kritik kontrolü
        if (IsCriticalHit(attacker, victim))
        {
            base_damage = (DWORD)(base_damage * GetCriticalMultiplier());
            dmg_info.is_critical = true;
            dmg_info.damage_type = DAMAGE_TYPE_CRITICAL;
        }
    }

    dmg_info.damage = base_damage;
    return dmg_info;
}

void CCombatSystem::ApplyDamage(const TDamageInfo& damage_info)
{
    if (!damage_info.victim)
        return;

    damage_info.victim->OnDamage(damage_info.attacker, damage_info.damage);

    // Log
    if (damage_info.attacker)
    {
        std::cout << damage_info.attacker->GetName();
    }
    else
    {
        std::cout << "Unknown";
    }

    std::cout << " dealt " << damage_info.damage << " ";

    if (damage_info.is_critical)
        std::cout << "CRITICAL ";

    switch (damage_info.damage_type)
    {
        case DAMAGE_TYPE_PHYSICAL: std::cout << "physical"; break;
        case DAMAGE_TYPE_MAGICAL: std::cout << "magical"; break;
        case DAMAGE_TYPE_PURE: std::cout << "pure"; break;
        case DAMAGE_TYPE_CRITICAL: std::cout << "critical"; break;
    }

    std::cout << " damage to " << damage_info.victim->GetName() << std::endl;
}

bool CCombatSystem::CanUseSkill(CCharacter* ch, DWORD skill_vnum, CCharacter* target)
{
    if (!ch)
        return false;

    const TSkillProto* skill = GetSkillProto(skill_vnum);
    if (!skill)
        return false;

    // Ölü mü?
    if (ch->IsDead())
        return false;

    // SP yeterli mi?
    if (ch->GetSP() < skill->sp_cost)
        return false;

    // Cooldown'da mı?
    if (IsOnCooldown(ch, skill_vnum))
        return false;

    // Hedef kontrolü
    if (skill->target_type == SKILL_TARGET_ENEMY)
    {
        if (!target || target == ch)
            return false;

        if (target->IsDead())
            return false;

        // Menzil kontrolü
        if (ch->GetDistance(target) > skill->range)
            return false;
    }

    return true;
}

bool CCombatSystem::UseSkill(CCharacter* ch, DWORD skill_vnum, CCharacter* target)
{
    if (!CanUseSkill(ch, skill_vnum, target))
        return false;

    const TSkillProto* skill = GetSkillProto(skill_vnum);
    if (!skill)
        return false;

    std::cout << ch->GetName() << " uses skill: " << skill->name << std::endl;

    // SP tüket
    if (skill->sp_cost > 0)
    {
        DWORD new_sp = ch->GetSP() - skill->sp_cost;
        // Character sınıfına SetSP fonksiyonu eklenecek
    }

    // Cooldown başlat
    SetCooldown(ch, skill_vnum, skill->cooldown_ms);

    // Skill türüne göre işlem yap
    switch (skill->type)
    {
        case SKILL_TYPE_ATTACK:
        {
            if (skill->area_range > 0 && target)
            {
                // Alan hasarı
                ApplyAreaDamage(ch, target->GetPosition(), skill->area_range, skill_vnum);
            }
            else if (target)
            {
                // Tek hedef hasar
                TDamageInfo dmg = CalculateDamage(ch, target, skill_vnum);
                ApplyDamage(dmg);
            }
            break;
        }

        case SKILL_TYPE_HEAL:
        {
            // İyileştirme
            CCharacter* heal_target = (skill->target_type == SKILL_TARGET_SELF) ? ch : target;
            if (heal_target)
            {
                DWORD heal_amount = skill->damage_base;
                heal_target->IncreaseHP(heal_amount);
                std::cout << heal_target->GetName() << " healed for " << heal_amount << " HP" << std::endl;
            }
            break;
        }

        case SKILL_TYPE_BUFF:
        case SKILL_TYPE_DEBUFF:
        {
            // Buff/Debuff uygula
            ApplySkillEffect(ch, target, skill);
            break;
        }
    }

    return true;
}

void CCombatSystem::ApplySkillEffect(CCharacter* caster, CCharacter* target, const TSkillProto* skill)
{
    if (!caster || !target || !skill)
        return;

    // Buff/Debuff sistemi için affect manager'a eklenecek
    std::cout << "Skill effect applied: " << skill->name
              << " on " << target->GetName()
              << " for " << skill->duration_ms / 1000.0f << " seconds" << std::endl;
}

void CCombatSystem::ApplyAreaDamage(CCharacter* attacker, const TPosition& center, DWORD range, DWORD skill_vnum)
{
    if (!attacker)
        return;

    std::cout << "Area damage at (" << center.x << ", " << center.y
              << ") with range " << range << std::endl;

    // Gerçek implementasyonda tüm karakterleri tarayıp
    // menzilde olanları bulup hasar verilir
    // Şimdilik sadece log yazdırıyoruz
}

void CCombatSystem::SetCooldown(CCharacter* ch, DWORD skill_vnum, DWORD cooldown_ms)
{
    TSkillCooldown cd;
    cd.vnum = skill_vnum;
    cd.last_use_time = std::chrono::steady_clock::now();
    cd.cooldown_ms = cooldown_ms;

    m_mapCooldowns[ch][skill_vnum] = cd;
}

bool CCombatSystem::IsOnCooldown(CCharacter* ch, DWORD skill_vnum)
{
    auto it = m_mapCooldowns.find(ch);
    if (it == m_mapCooldowns.end())
        return false;

    auto cd_it = it->second.find(skill_vnum);
    if (cd_it == it->second.end())
        return false;

    return !cd_it->second.IsReady();
}

DWORD CCombatSystem::GetRemainingCooldown(CCharacter* ch, DWORD skill_vnum)
{
    auto it = m_mapCooldowns.find(ch);
    if (it == m_mapCooldowns.end())
        return 0;

    auto cd_it = it->second.find(skill_vnum);
    if (cd_it == it->second.end())
        return 0;

    return cd_it->second.GetRemainingCooldown();
}

bool CCombatSystem::LoadSkillProtos(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to load skill protos: " << filename << std::endl;
        return false;
    }

    // Basit CSV parsing (gerçek implementasyonda JSON veya binary format kullanılabilir)
    std::string line;
    while (std::getline(file, line))
    {
        // Parse skill data and register
        // Şimdilik atlanıyor
    }

    file.close();
    return true;
}
