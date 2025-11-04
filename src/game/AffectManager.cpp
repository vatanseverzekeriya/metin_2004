#include "../../include/game/AffectManager.h"
#include "../../include/game/Character.h"
#include <algorithm>
#include <iostream>

CAffectManager::CAffectManager(CCharacter* owner)
    : m_owner(owner)
    , m_lAttackBonus(0)
    , m_lDefenseBonus(0)
    , m_fSpeedMultiplier(1.0f)
{
}

CAffectManager::~CAffectManager()
{
    RemoveAllAffects();
}

bool CAffectManager::AddAffect(EAffectType type, DWORD value, DWORD duration_ms, CCharacter* caster)
{
    if (!m_owner || type == AFFECT_NONE)
        return false;

    // Aynı türden affect varsa kaldır
    RemoveAffect(type);

    TAffect affect;
    affect.type = type;
    affect.value = value;
    affect.duration_ms = duration_ms;
    affect.start_time = std::chrono::steady_clock::now();
    affect.caster = caster;

    m_vecAffects.push_back(affect);

    ApplyAffectEffect(affect);
    RecalculateBonuses();

    std::cout << m_owner->GetName() << " gained affect: " << (int)type
              << " (value: " << value << ", duration: " << duration_ms / 1000.0f << "s)" << std::endl;

    return true;
}

bool CAffectManager::RemoveAffect(EAffectType type)
{
    auto it = std::find_if(m_vecAffects.begin(), m_vecAffects.end(),
        [type](const TAffect& affect) {
            return affect.type == type;
        });

    if (it == m_vecAffects.end())
        return false;

    RemoveAffectEffect(*it);
    m_vecAffects.erase(it);
    RecalculateBonuses();

    std::cout << m_owner->GetName() << " lost affect: " << (int)type << std::endl;

    return true;
}

void CAffectManager::RemoveAllAffects()
{
    for (const auto& affect : m_vecAffects)
    {
        RemoveAffectEffect(affect);
    }

    m_vecAffects.clear();
    RecalculateBonuses();
}

bool CAffectManager::HasAffect(EAffectType type) const
{
    return std::any_of(m_vecAffects.begin(), m_vecAffects.end(),
        [type](const TAffect& affect) {
            return affect.type == type;
        });
}

const TAffect* CAffectManager::GetAffect(EAffectType type) const
{
    auto it = std::find_if(m_vecAffects.begin(), m_vecAffects.end(),
        [type](const TAffect& affect) {
            return affect.type == type;
        });

    if (it == m_vecAffects.end())
        return nullptr;

    return &(*it);
}

void CAffectManager::Update(float delta_time)
{
    if (!m_owner)
        return;

    // Süresi dolan affectleri kaldır
    auto it = m_vecAffects.begin();
    while (it != m_vecAffects.end())
    {
        if (it->IsExpired())
        {
            std::cout << m_owner->GetName() << " - Affect expired: " << (int)it->type << std::endl;
            RemoveAffectEffect(*it);
            it = m_vecAffects.erase(it);
        }
        else
        {
            // DoT (Damage over Time) effectleri işle
            switch (it->type)
            {
                case AFFECT_POISON:
                    ProcessPoison(*it, delta_time);
                    break;

                default:
                    break;
            }

            ++it;
        }
    }

    // Rejenerasyonu güncelle
    UpdateRegeneration(delta_time);
}

void CAffectManager::UpdateRegeneration(float delta_time)
{
    if (!m_owner || m_owner->IsDead())
        return;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_regenInfo.last_regen_time).count();

    // 1 saniyede bir regen
    if (elapsed >= 1000)
    {
        m_regenInfo.last_regen_time = now;

        // HP rejeni
        DWORD hp_regen = m_regenInfo.hp_regen_per_sec;

        // Percent regen
        DWORD max_hp = m_owner->GetMaxHP();
        hp_regen += (DWORD)(max_hp * m_regenInfo.hp_regen_percent);

        // Regen buff varsa bonus
        if (HasAffect(AFFECT_HP_REGEN))
        {
            const TAffect* affect = GetAffect(AFFECT_HP_REGEN);
            if (affect)
                hp_regen += affect->value;
        }

        if (hp_regen > 0 && m_owner->GetHP() < max_hp)
        {
            m_owner->IncreaseHP(hp_regen);
        }

        // SP rejeni
        DWORD sp_regen = m_regenInfo.sp_regen_per_sec;

        DWORD max_sp = m_owner->GetMaxSP();
        sp_regen += (DWORD)(max_sp * m_regenInfo.sp_regen_percent);

        if (HasAffect(AFFECT_SP_REGEN))
        {
            const TAffect* affect = GetAffect(AFFECT_SP_REGEN);
            if (affect)
                sp_regen += affect->value;
        }

        if (sp_regen > 0 && m_owner->GetSP() < max_sp)
        {
            // SP artırma fonksiyonu Character'a eklenecek
            // m_owner->IncreaseSP(sp_regen);
        }
    }
}

void CAffectManager::SetRegenRate(DWORD hp_per_sec, DWORD sp_per_sec)
{
    m_regenInfo.hp_regen_per_sec = hp_per_sec;
    m_regenInfo.sp_regen_per_sec = sp_per_sec;
}

void CAffectManager::RecalculateBonuses()
{
    m_lAttackBonus = 0;
    m_lDefenseBonus = 0;
    m_fSpeedMultiplier = 1.0f;

    for (const auto& affect : m_vecAffects)
    {
        switch (affect.type)
        {
            case AFFECT_ATTACK_BOOST:
                m_lAttackBonus += affect.value;
                break;

            case AFFECT_DEFENSE_BOOST:
                m_lDefenseBonus += affect.value;
                break;

            case AFFECT_SPEED_BOOST:
                m_fSpeedMultiplier += affect.value / 100.0f;
                break;

            case AFFECT_SLOW:
                m_fSpeedMultiplier -= affect.value / 100.0f;
                if (m_fSpeedMultiplier < 0.1f)
                    m_fSpeedMultiplier = 0.1f;
                break;

            default:
                break;
        }
    }
}

void CAffectManager::ApplyAffectEffect(const TAffect& affect)
{
    if (!m_owner)
        return;

    switch (affect.type)
    {
        case AFFECT_SHIELD:
            // Kalkan effecti uygulandı
            std::cout << m_owner->GetName() << " gained a shield absorbing " << affect.value << " damage" << std::endl;
            break;

        case AFFECT_STUN:
            std::cout << m_owner->GetName() << " is stunned!" << std::endl;
            break;

        case AFFECT_POISON:
            std::cout << m_owner->GetName() << " is poisoned!" << std::endl;
            break;

        default:
            break;
    }
}

void CAffectManager::RemoveAffectEffect(const TAffect& affect)
{
    // Affect kaldırılırken gerekli temizlik işlemleri
}

void CAffectManager::ProcessPoison(const TAffect& affect, float delta_time)
{
    if (!m_owner || m_owner->IsDead())
        return;

    // Saniyede bir hasar (örnek: saniyede 10 hasar)
    static float poison_timer = 0.0f;
    poison_timer += delta_time;

    if (poison_timer >= 1.0f)
    {
        poison_timer = 0.0f;

        DWORD poison_damage = affect.value; // value = saniyede hasar
        m_owner->DecreaseHP(poison_damage);

        std::cout << m_owner->GetName() << " takes " << poison_damage << " poison damage" << std::endl;
    }
}

void CAffectManager::ProcessDoT(const TAffect& affect, float delta_time)
{
    // Genel DoT (Damage over Time) işleme
    ProcessPoison(affect, delta_time);
}
