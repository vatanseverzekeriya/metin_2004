#ifndef __INC_GAME_CHARACTER_H__
#define __INC_GAME_CHARACTER_H__

#include "../common/types.h"
#include "Movement.h"
#include <string>
#include <memory>
#include <set>
#include <chrono>

// Forward declarations
class CAffectManager;
class CMovement;

class CCharacter
{
public:
    CCharacter();
    virtual ~CCharacter();

    // Başlatma ve yükleme
    bool Initialize(DWORD player_id);
    void Destroy();

    // Temel bilgiler
    DWORD GetPlayerID() const { return m_dwPlayerID; }
    const std::string& GetName() const { return m_strName; }
    void SetName(const std::string& name) { m_strName = name; }

    BYTE GetJob() const { return m_byJob; }
    void SetJob(BYTE job) { m_byJob = job; }

    // Pozisyon
    const TPosition& GetPosition() const { return m_position; }
    void SetPosition(const TPosition& pos);
    void SetPosition(LONG x, LONG y, LONG z = 0);
    bool MoveTo(LONG x, LONG y);

    // İstatistikler
    const TPlayerStats& GetStats() const { return m_stats; }
    void SetStats(const TPlayerStats& stats) { m_stats = stats; }

    DWORD GetLevel() const { return m_stats.level; }
    void SetLevel(DWORD level);

    DWORD GetHP() const { return m_stats.hp; }
    void SetHP(DWORD hp);
    void IncreaseHP(DWORD amount);
    void DecreaseHP(DWORD amount);

    DWORD GetMaxHP() const { return m_stats.max_hp; }

    DWORD GetSP() const { return m_stats.sp; }
    void SetSP(DWORD sp);
    void IncreaseSP(DWORD amount);
    void DecreaseSP(DWORD amount);
    DWORD GetMaxSP() const { return m_stats.max_sp; }

    DWORD GetExp() const { return m_stats.exp; }
    void GiveExp(DWORD exp);

    DWORD GetGold() const { return m_stats.gold; }
    void SetGold(DWORD gold);
    bool ChangeGold(LONG amount);

    // Savaş
    DWORD GetAttack() const { return m_stats.attack; }
    DWORD GetDefense() const { return m_stats.defense; }
    DWORD GetMagicAttack() const { return m_stats.magic_attack; }
    DWORD GetMagicDefense() const { return m_stats.magic_defense; }

    // Durum
    ECharacterState GetState() const { return m_eState; }
    void SetState(ECharacterState state) { m_eState = state; }
    bool IsDead() const { return m_eState == STATE_DEAD; }

    // PvP
    EPvPMode GetPvPMode() const { return m_ePvPMode; }
    void SetPvPMode(EPvPMode mode) { m_ePvPMode = mode; }
    bool CanAttack(CCharacter* victim);

    DWORD GetPvPKills() const { return m_dwPvPKills; }
    DWORD GetPvPDeaths() const { return m_dwPvPDeaths; }
    void IncreasePvPKills();
    void IncreasePvPDeaths();

    // Saldırı sistemi
    bool Attack(CCharacter* victim);
    void OnDamage(CCharacter* attacker, DWORD damage);
    void OnKill(CCharacter* victim);
    void OnDeath(CCharacter* killer);

    DWORD CalculateDamage(CCharacter* victim);

    // Kaydetme ve yükleme
    bool Save();
    bool Load();

    // Güncelleme
    void Update(float delta_time);

    // Mesafe hesaplama
    DWORD GetDistance(const CCharacter* ch) const;

    // Affect Manager
    CAffectManager* GetAffectManager() const { return m_pkAffectManager; }

    // Movement
    CMovement* GetMovement() const { return m_pkMovement; }
    bool StartMove(const TPosition& target, EMovementType type = MOVE_TYPE_RUN);
    void StopMove();

private:
    DWORD m_dwPlayerID;
    std::string m_strName;
    BYTE m_byJob;

    TPosition m_position;
    TPlayerStats m_stats;

    ECharacterState m_eState;
    EPvPMode m_ePvPMode;

    DWORD m_dwPvPKills;
    DWORD m_dwPvPDeaths;

    // Saldırı zamanlaması
    std::chrono::steady_clock::time_point m_lastAttackTime;
    DWORD m_dwAttackSpeed; // Milisaniye cinsinden

    // Hedef tracking
    CCharacter* m_pkVictim;

    // Affect ve Movement yöneticileri
    CAffectManager* m_pkAffectManager;
    CMovement* m_pkMovement;

    void CalculateMaxHP();
    void CalculateMaxSP();
    void CheckLevelUp();
};

#endif // __INC_GAME_CHARACTER_H__
