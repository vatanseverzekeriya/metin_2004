/**
 * Metin2 PvP Manager Implementation
 *
 * Açık Kaynak Referans: cCorax2/Source_code (game/pvp.cpp)
 * GitHub: https://github.com/cCorax2/Source_code/blob/master/game/pvp.cpp
 *
 * Modern C++17 implementasyonu - Mobil optimizasyonlu
 */

#include "../../include/game/PVPManager.h"
#include "../../include/game/Character.h"
#include <algorithm>
#include <iostream>
#include <cstring>

// ============================================================================
// CPVP Implementasyonu
// ============================================================================

/**
 * CRC hesaplama (cCorax2 metodu)
 * İki oyuncu ID'sinden benzersiz hash üretir
 */
DWORD CPVP::CalcCRC(DWORD pid1, DWORD pid2)
{
    // Küçük ID her zaman önce gelsin (tutarlılık için)
    if (pid1 > pid2)
        std::swap(pid1, pid2);

    // Basit hash: (pid1 << 16) | pid2
    // Not: Gerçek Metin2'de daha karmaşık CRC32 kullanılır
    return (pid1 << 16) | (pid2 & 0xFFFF);
}

/**
 * Constructor (cCorax2 CPVP::CPVP bazlı)
 */
CPVP::CPVP(DWORD pid1, DWORD pid2)
    : m_bRevenge(false)
    , m_tStartTime(time(nullptr))
{
    // Oyuncu ID'lerini sırala (küçük önce)
    if (pid1 > pid2)
        std::swap(pid1, pid2);

    m_players[0] = TPlayer(pid1, 0);
    m_players[1] = TPlayer(pid2, 0);

    // CRC hesapla
    m_dwCRC = CalcCRC(pid1, pid2);

    // İlk oyuncu otomatik kabul etmiş sayılır (cCorax2 mantığı)
    m_players[0].bAgree = true;

    std::cout << "[PVP] New PvP: " << pid1 << " vs " << pid2
              << " (CRC: " << m_dwCRC << ")" << std::endl;
}

/**
 * Her iki oyuncu da kabul ettiyse true
 */
bool CPVP::IsFight() const
{
    return m_players[0].bAgree && m_players[1].bAgree;
}

/**
 * PvP durumunu döndür
 */
EPvPState CPVP::GetState() const
{
    if (m_bRevenge)
        return PVP_STATE_REVENGE;
    else if (IsFight())
        return PVP_STATE_FIGHT;
    else if (m_players[0].bAgree || m_players[1].bAgree)
        return PVP_STATE_WAIT;
    else
        return PVP_STATE_NONE;
}

/**
 * Oyuncu index'ini bul (0 veya 1)
 */
BYTE CPVP::FindIndex(DWORD pid) const
{
    if (m_players[0].dwPID == pid)
        return 0;
    else if (m_players[1].dwPID == pid)
        return 1;

    std::cerr << "[PVP] ERROR: PID " << pid << " not found in PvP!" << std::endl;
    return 0;
}

DWORD CPVP::GetPID(BYTE index) const
{
    return (index < 2) ? m_players[index].dwPID : 0;
}

DWORD CPVP::GetVID(BYTE index) const
{
    return (index < 2) ? m_players[index].dwVID : 0;
}

void CPVP::SetVID(DWORD pid, DWORD vid)
{
    BYTE index = FindIndex(pid);
    m_players[index].dwVID = vid;
}

/**
 * Oyuncu PvP'yi kabul etti (cCorax2 Agree() bazlı)
 */
void CPVP::Agree(DWORD pid)
{
    BYTE index = FindIndex(pid);
    m_players[index].bAgree = true;

    std::cout << "[PVP] Player " << pid << " agreed to PvP" << std::endl;

    if (IsFight())
    {
        std::cout << "[PVP] Fight started! " << m_players[0].dwPID
                  << " vs " << m_players[1].dwPID << std::endl;
    }
}

/**
 * En az bir oyuncu kabul etti mi?
 */
bool CPVP::Agree() const
{
    return m_players[0].bAgree || m_players[1].bAgree;
}

/**
 * Oyuncu kazandı (cCorax2 Win() bazlı)
 */
void CPVP::Win(DWORD pid)
{
    BYTE winner_index = FindIndex(pid);
    BYTE loser_index = (winner_index == 0) ? 1 : 0;

    std::cout << "[PVP] Winner: " << m_players[winner_index].dwPID
              << ", Loser: " << m_players[loser_index].dwPID << std::endl;

    // Kaybedene intikam hakkı ver
    m_players[loser_index].bCanRevenge = true;
}

/**
 * İntikam alabilir mi?
 */
bool CPVP::CanRevenge(DWORD pid) const
{
    BYTE index = FindIndex(pid);
    return m_players[index].bCanRevenge;
}

/**
 * İntikam modunu aktifleştir
 */
void CPVP::SetRevenge(DWORD pid)
{
    if (CanRevenge(pid))
    {
        m_bRevenge = true;
        UpdateTime();
        std::cout << "[PVP] Revenge mode activated for " << pid << std::endl;
    }
}

// ============================================================================
// CPVPManager Implementasyonu
// ============================================================================

/**
 * Singleton instance (thread-safe C++11+)
 */
CPVPManager& CPVPManager::Instance()
{
    static CPVPManager instance;
    return instance;
}

/**
 * CRC hesaplama helper
 */
DWORD CPVPManager::GetCRC(DWORD pid1, DWORD pid2) const
{
    return CPVP::CalcCRC(pid1, pid2);
}

/**
 * PvP bul
 */
CPVP* CPVPManager::Find(DWORD pid1, DWORD pid2)
{
    DWORD crc = GetCRC(pid1, pid2);

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_map_pvp.find(crc);
    return (it != m_map_pvp.end()) ? &it->second : nullptr;
}

/**
 * Oyuncunun herhangi bir PvP'sini bul
 */
CPVP* CPVPManager::FindByPID(DWORD pid)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_map_pk_pvp.find(pid);
    if (it != m_map_pk_pvp.end())
    {
        DWORD crc = it->second;
        auto pvp_it = m_map_pvp.find(crc);
        if (pvp_it != m_map_pvp.end())
            return &pvp_it->second;
    }

    return nullptr;
}

/**
 * Yeni PvP request oluştur (cCorax2 Insert() bazlı)
 */
void CPVPManager::Insert(DWORD pid1, DWORD pid2)
{
    if (pid1 == pid2)
    {
        std::cerr << "[PVP] Cannot PvP with self!" << std::endl;
        return;
    }

    DWORD crc = GetCRC(pid1, pid2);

    std::lock_guard<std::mutex> lock(m_mutex);

    // Zaten var mı kontrol et
    auto it = m_map_pvp.find(crc);
    if (it != m_map_pvp.end())
    {
        // Varsa Agree() çağır (cCorax2 mantığı)
        std::cout << "[PVP] PvP already exists, calling Agree()" << std::endl;
        it->second.Agree(pid1);

        // Fight başladıysa callback
        if (it->second.IsFight() && m_onPvPStart)
            m_onPvPStart(pid1, pid2);

        return;
    }

    // Yeni PvP oluştur
    CPVP pvp(pid1, pid2);
    m_map_pvp[crc] = pvp;

    // Quick lookup için
    m_map_pk_pvp[pid1] = crc;
    m_map_pk_pvp[pid2] = crc;

    std::cout << "[PVP] PvP request: " << pid1 << " -> " << pid2 << std::endl;

    // Mobile callback: Request notification
    if (m_onPvPRequest)
        m_onPvPRequest(pid1, pid2);
}

/**
 * PvP request'i kabul et
 */
void CPVPManager::Agree(DWORD pid)
{
    CPVP* pvp = FindByPID(pid);
    if (!pvp)
    {
        std::cerr << "[PVP] No pending PvP for player " << pid << std::endl;
        return;
    }

    bool was_fight = pvp->IsFight();
    pvp->Agree(pid);

    // Yeni fight başladıysa
    if (!was_fight && pvp->IsFight())
    {
        DWORD pid1 = pvp->GetPID(0);
        DWORD pid2 = pvp->GetPID(1);

        std::cout << "[PVP] Fight started: " << pid1 << " vs " << pid2 << std::endl;

        if (m_onPvPStart)
            m_onPvPStart(pid1, pid2);
    }
}

/**
 * PvP'yi sona erdir
 */
void CPVPManager::Remove(DWORD pid1, DWORD pid2)
{
    DWORD crc = GetCRC(pid1, pid2);

    std::lock_guard<std::mutex> lock(m_mutex);

    m_map_pvp.erase(crc);
    m_map_pk_pvp.erase(pid1);
    m_map_pk_pvp.erase(pid2);

    std::cout << "[PVP] PvP removed: " << pid1 << " vs " << pid2 << std::endl;
}

/**
 * Oyuncunun tüm PvP'lerini sil
 */
void CPVPManager::RemoveAll(DWORD pid)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_map_pk_pvp.find(pid);
    if (it != m_map_pk_pvp.end())
    {
        DWORD crc = it->second;
        m_map_pvp.erase(crc);
        m_map_pk_pvp.erase(pid);

        std::cout << "[PVP] All PvPs removed for player " << pid << std::endl;
    }
}

/**
 * Saldırı kontrolü (cCorax2 CanAttack() bazlı)
 *
 * Basitleştirilmiş versiyon - gerçek Metin2'de çok daha karmaşık:
 * - Empire kontrolü
 * - Guild war kontrolü
 * - Safe zone kontrolü
 * - Party kontrolü
 * - etc.
 */
bool CPVPManager::CanAttack(CCharacter* pkAttacker, CCharacter* pkVictim)
{
    if (!pkAttacker || !pkVictim)
        return false;

    if (pkAttacker == pkVictim)
        return false;

    DWORD pid1 = pkAttacker->GetPlayerID();
    DWORD pid2 = pkVictim->GetPlayerID();

    CPVP* pvp = Find(pid1, pid2);

    // PvP yok
    if (!pvp)
    {
        // PK mode kontrolü (Character'dan)
        EPvPMode attackerMode = pkAttacker->GetPvPMode();

        // PVP_MODE_NONE ise saldırı yok
        if (attackerMode == PVP_MODE_NONE)
            return false;

        // PVP_MODE_NORMAL ise PvP request gerekli
        if (attackerMode == PVP_MODE_NORMAL)
            return false;

        // Diğer modlar için (GUILD, PARTY) daha fazla kontrol gerekir
        // Şimdilik basit: normal mode değilse izin ver
        return true;
    }

    // Revenge mode kontrolü
    if (pvp->IsRevenge())
    {
        // İntikam modundaysa ve süre dolmadıysa
        time_t now = time(nullptr);
        time_t elapsed = now - pvp->GetStartTime();

        if (elapsed < REVENGE_TIMEOUT && pvp->CanRevenge(pid1))
        {
            std::cout << "[PVP] Revenge attack allowed! " << pid1 << " -> " << pid2 << std::endl;
            return true;
        }

        // Süre dolmuşsa revenge'i kapat
        if (elapsed >= REVENGE_TIMEOUT)
        {
            std::cout << "[PVP] Revenge timeout expired" << std::endl;
            Remove(pid1, pid2);
            return false;
        }
    }

    // Normal PvP - fight durumunda mı?
    return pvp->IsFight();
}

/**
 * PvP durumunu kontrol et
 */
bool CPVPManager::IsFighting(DWORD pid1, DWORD pid2)
{
    CPVP* pvp = Find(pid1, pid2);
    return pvp && pvp->IsFight();
}

/**
 * PvP durumunu al
 */
EPvPState CPVPManager::GetPvPState(DWORD pid1, DWORD pid2)
{
    CPVP* pvp = Find(pid1, pid2);
    return pvp ? pvp->GetState() : PVP_STATE_NONE;
}

/**
 * Ölüm eventi (cCorax2 Dead() bazlı)
 *
 * Önemli: Revenge mode burada aktifleşir!
 */
void CPVPManager::OnDeath(CCharacter* pkDead, CCharacter* pkKiller)
{
    if (!pkDead || !pkKiller)
        return;

    DWORD pidDead = pkDead->GetPlayerID();
    DWORD pidKiller = pkKiller->GetPlayerID();

    CPVP* pvp = Find(pidDead, pidKiller);
    if (!pvp)
    {
        std::cout << "[PVP] Death outside of PvP system" << std::endl;
        return;
    }

    // Killer kazandı
    pvp->Win(pidKiller);

    // Callback: PvP bitti
    if (m_onPvPEnd)
        m_onPvPEnd(pidKiller, pidDead);

    // Revenge mode aktifleştir
    pvp->SetRevenge(pidDead);

    // Callback: Revenge available
    if (m_onRevenge)
        m_onRevenge(pidDead, pidKiller);

    std::cout << "[PVP] Death in PvP: " << pkKiller->GetName()
              << " killed " << pkDead->GetName()
              << " - Revenge mode available for 5 min" << std::endl;
}

/**
 * Oyuncu bağlandı
 */
void CPVPManager::OnConnect(DWORD pid)
{
    std::cout << "[PVP] Player " << pid << " connected" << std::endl;
    // TODO: Veritabanından PvP durumlarını yükle
}

/**
 * Oyuncu bağlantısı kesildi
 */
void CPVPManager::OnDisconnect(DWORD pid)
{
    std::cout << "[PVP] Player " << pid << " disconnected" << std::endl;
    // TODO: PvP'leri kaydet veya temizle
    RemoveAll(pid);
}

/**
 * Periyodik cleanup (cCorax2 Process() - 10 dakika timeout)
 *
 * Her 1 saniyede çağrılmalı (game loop'tan)
 */
void CPVPManager::Process()
{
    time_t now = time(nullptr);

    std::lock_guard<std::mutex> lock(m_mutex);

    // Timeout olan PvP'leri bul ve sil
    auto it = m_map_pvp.begin();
    while (it != m_map_pvp.end())
    {
        CPVP& pvp = it->second;
        time_t elapsed = now - pvp.GetStartTime();

        // 10 dakika timeout (cCorax2 mantığı)
        if (elapsed > PVPLIST_TIMEOUT)
        {
            DWORD pid1 = pvp.GetPID(0);
            DWORD pid2 = pvp.GetPID(1);

            std::cout << "[PVP] Timeout: Removing PvP " << pid1 << " vs " << pid2
                      << " (elapsed: " << elapsed << "s)" << std::endl;

            // Quick lookup'tan da sil
            m_map_pk_pvp.erase(pid1);
            m_map_pk_pvp.erase(pid2);

            it = m_map_pvp.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

/**
 * PvP sayısı
 */
size_t CPVPManager::GetPvPCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_map_pvp.size();
}

/**
 * Tüm PvP'leri temizle
 */
void CPVPManager::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_map_pvp.clear();
    m_map_pk_pvp.clear();
    std::cout << "[PVP] All PvPs cleared" << std::endl;
}
