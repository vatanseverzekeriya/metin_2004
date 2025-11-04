#include "../../include/game/Character.h"
#include "../../include/db/DBManager.h"
#include <cmath>
#include <iostream>

CCharacter::CCharacter()
    : m_dwPlayerID(0)
    , m_byJob(0)
    , m_eState(STATE_IDLE)
    , m_ePvPMode(PVP_MODE_NORMAL)
    , m_dwPvPKills(0)
    , m_dwPvPDeaths(0)
    , m_dwAttackSpeed(1000) // 1 saniye
    , m_pkVictim(nullptr)
{
    m_lastAttackTime = std::chrono::steady_clock::now();
}

CCharacter::~CCharacter()
{
    Destroy();
}

bool CCharacter::Initialize(DWORD player_id)
{
    m_dwPlayerID = player_id;
    return Load();
}

void CCharacter::Destroy()
{
    if (m_dwPlayerID > 0)
    {
        Save();
    }
    m_pkVictim = nullptr;
}

void CCharacter::SetPosition(const TPosition& pos)
{
    m_position = pos;
}

void CCharacter::SetPosition(LONG x, LONG y, LONG z)
{
    m_position.x = x;
    m_position.y = y;
    m_position.z = z;
}

bool CCharacter::MoveTo(LONG x, LONG y)
{
    if (IsDead())
        return false;

    SetState(STATE_MOVING);
    SetPosition(x, y, m_position.z);
    SetState(STATE_IDLE);
    return true;
}

void CCharacter::SetLevel(DWORD level)
{
    m_stats.level = level;
    CalculateMaxHP();
    CalculateMaxSP();
}

void CCharacter::SetHP(DWORD hp)
{
    if (hp > m_stats.max_hp)
        hp = m_stats.max_hp;

    m_stats.hp = hp;

    if (m_stats.hp == 0 && m_eState != STATE_DEAD)
    {
        m_eState = STATE_DEAD;
        std::cout << m_strName << " has died!" << std::endl;
    }
}

void CCharacter::IncreaseHP(DWORD amount)
{
    DWORD new_hp = m_stats.hp + amount;
    SetHP(new_hp);
}

void CCharacter::DecreaseHP(DWORD amount)
{
    if (amount >= m_stats.hp)
        SetHP(0);
    else
        SetHP(m_stats.hp - amount);
}

void CCharacter::GiveExp(DWORD exp)
{
    m_stats.exp += exp;
    std::cout << m_strName << " gained " << exp << " experience!" << std::endl;
    CheckLevelUp();
}

void CCharacter::SetGold(DWORD gold)
{
    m_stats.gold = gold;
}

bool CCharacter::ChangeGold(LONG amount)
{
    if (amount < 0 && (DWORD)(-amount) > m_stats.gold)
        return false;

    m_stats.gold += amount;
    return true;
}

void CCharacter::CalculateMaxHP()
{
    // Basit HP hesaplama: Seviye başına 100 HP
    m_stats.max_hp = 1000 + (m_stats.level - 1) * 100;
    if (m_stats.hp > m_stats.max_hp)
        m_stats.hp = m_stats.max_hp;
}

void CCharacter::CalculateMaxSP()
{
    // Basit SP hesaplama: Seviye başına 10 SP
    m_stats.max_sp = 100 + (m_stats.level - 1) * 10;
    if (m_stats.sp > m_stats.max_sp)
        m_stats.sp = m_stats.max_sp;
}

void CCharacter::CheckLevelUp()
{
    // Basit level up sistemi
    DWORD needed_exp = m_stats.level * 1000;

    while (m_stats.exp >= needed_exp && m_stats.level < 99)
    {
        m_stats.level++;
        m_stats.exp -= needed_exp;
        needed_exp = m_stats.level * 1000;

        // İstatistikleri artır
        m_stats.attack += 5;
        m_stats.defense += 3;
        m_stats.magic_attack += 3;
        m_stats.magic_defense += 3;

        CalculateMaxHP();
        CalculateMaxSP();

        // Full HP ve SP
        m_stats.hp = m_stats.max_hp;
        m_stats.sp = m_stats.max_sp;

        std::cout << m_strName << " leveled up to " << m_stats.level << "!" << std::endl;
    }
}

bool CCharacter::CanAttack(CCharacter* victim)
{
    if (!victim || victim == this)
        return false;

    if (IsDead() || victim->IsDead())
        return false;

    // Mesafe kontrolü (150 birim = yakın saldırı menzili)
    if (GetDistance(victim) > 150)
        return false;

    // PvP modu kontrolü
    if (m_ePvPMode == PVP_MODE_NONE)
        return false;

    return true;
}

DWORD CCharacter::CalculateDamage(CCharacter* victim)
{
    if (!victim)
        return 0;

    // Basit hasar hesaplama
    LONG damage = m_stats.attack - victim->GetDefense();

    // Minimum hasar
    if (damage < 10)
        damage = 10;

    // Rastgelelik ekle (%80-120)
    int variance = (rand() % 41) - 20; // -20 ile +20 arası
    damage = damage * (100 + variance) / 100;

    return (DWORD)damage;
}

bool CCharacter::Attack(CCharacter* victim)
{
    if (!CanAttack(victim))
        return false;

    // Saldırı hızı kontrolü
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_lastAttackTime).count();

    if (elapsed < m_dwAttackSpeed)
        return false;

    m_lastAttackTime = now;
    SetState(STATE_ATTACKING);

    // Hasar hesapla
    DWORD damage = CalculateDamage(victim);

    // Hasarı uygula
    victim->OnDamage(this, damage);

    std::cout << m_strName << " attacks " << victim->GetName()
              << " for " << damage << " damage!" << std::endl;

    SetState(STATE_IDLE);
    return true;
}

void CCharacter::OnDamage(CCharacter* attacker, DWORD damage)
{
    DecreaseHP(damage);

    if (IsDead())
    {
        OnDeath(attacker);
        if (attacker)
            attacker->OnKill(this);
    }
}

void CCharacter::OnKill(CCharacter* victim)
{
    if (!victim)
        return;

    std::cout << m_strName << " has killed " << victim->GetName() << "!" << std::endl;

    // PvP kill sayacı
    IncreasePvPKills();

    // Exp kazanımı
    DWORD exp_gain = victim->GetLevel() * 50;
    GiveExp(exp_gain);
}

void CCharacter::OnDeath(CCharacter* killer)
{
    SetState(STATE_DEAD);
    IncreasePvPDeaths();

    std::cout << m_strName << " has been killed";
    if (killer)
        std::cout << " by " << killer->GetName();
    std::cout << "!" << std::endl;
}

void CCharacter::IncreasePvPKills()
{
    m_dwPvPKills++;
    CDBManager::Instance().UpdatePvPStats(m_dwPlayerID, m_dwPvPKills, m_dwPvPDeaths);
}

void CCharacter::IncreasePvPDeaths()
{
    m_dwPvPDeaths++;
    CDBManager::Instance().UpdatePvPStats(m_dwPlayerID, m_dwPvPKills, m_dwPvPDeaths);
}

DWORD CCharacter::GetDistance(const CCharacter* ch) const
{
    if (!ch)
        return 999999;

    LONG dx = m_position.x - ch->m_position.x;
    LONG dy = m_position.y - ch->m_position.y;

    return (DWORD)sqrt(dx * dx + dy * dy);
}

bool CCharacter::Save()
{
    if (m_dwPlayerID == 0)
        return false;

    return CDBManager::Instance().SavePlayer(m_dwPlayerID, m_stats, m_position);
}

bool CCharacter::Load()
{
    if (m_dwPlayerID == 0)
        return false;

    if (!CDBManager::Instance().LoadPlayer(m_dwPlayerID, m_stats, m_position))
        return false;

    // PvP istatistiklerini yükle
    CDBManager::Instance().GetPvPStats(m_dwPlayerID, m_dwPvPKills, m_dwPvPDeaths);

    return true;
}

void CCharacter::Update()
{
    // Düzenli güncelleme işlemleri
    // Örnek: HP/SP rejenerasyonu, buff kontrolü vs.
}
