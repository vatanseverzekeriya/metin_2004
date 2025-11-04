#ifndef __INC_GAME_PVPMANAGER_H__
#define __INC_GAME_PVPMANAGER_H__

/**
 * Metin2 PvP Manager - Mobile Optimized
 *
 * Açık Kaynak Referans: cCorax2/Source_code (game/pvp.cpp, game/pvp.h)
 * Modern C++17 ile yeniden implement edilmiş, mobil platformlar için optimize edilmiş
 *
 * Özellikler:
 * - PvP request/accept sistemi
 * - Revenge mode (5 dakika)
 * - CRC hash ile hızlı arama
 * - Thread-safe tasarım
 * - Mobile notification callbacks
 */

#include "../common/types.h"
#include <map>
#include <memory>
#include <mutex>
#include <functional>
#include <ctime>

// Forward declarations
class CCharacter;

// PvP durumları (cCorax2 bazlı)
enum EPvPState
{
    PVP_STATE_NONE = 0,      // PvP yok
    PVP_STATE_WAIT = 1,      // Bir taraf kabul etti, diğeri bekliyor
    PVP_STATE_FIGHT = 2,     // Her iki taraf kabul etti - aktif savaş
    PVP_STATE_REVENGE = 3    // İntikam modu
};

/**
 * İki oyuncu arası PvP ilişkisini temsil eder
 * Referans: CPVP sınıfı (cCorax2)
 */
class CPVP
{
public:
    // Oyuncu bilgi yapısı (TPlayer - cCorax2 bazlı)
    struct TPlayer
    {
        DWORD dwPID;           // Player ID
        DWORD dwVID;           // Virtual ID (character instance)
        bool bAgree;           // Kabul durumu
        bool bCanRevenge;      // İntikam alabilir mi?

        TPlayer() : dwPID(0), dwVID(0), bAgree(false), bCanRevenge(false) {}
        TPlayer(DWORD pid, DWORD vid)
            : dwPID(pid), dwVID(vid), bAgree(false), bCanRevenge(false) {}
    };

    CPVP(DWORD pid1, DWORD pid2);
    ~CPVP() = default;

    // Temel bilgiler
    DWORD GetCRC() const { return m_dwCRC; }
    bool IsFight() const;  // Her iki taraf da kabul ettiyse true
    EPvPState GetState() const;

    // Oyuncu bilgileri
    DWORD GetPID(BYTE index) const;
    DWORD GetVID(BYTE index) const;
    void SetVID(DWORD pid, DWORD vid);

    // PvP işlemleri
    void Agree(DWORD pid);     // Oyuncu kabul etti
    bool Agree() const;        // En az bir oyuncu kabul etti mi?
    void Win(DWORD pid);       // Oyuncu kazandı

    // Revenge (cCorax2 Dead() fonksiyonu bazlı)
    bool CanRevenge(DWORD pid) const;
    void SetRevenge(DWORD pid);
    bool IsRevenge() const { return m_bRevenge; }

    // Zaman kontrolü
    time_t GetStartTime() const { return m_tStartTime; }
    void UpdateTime() { m_tStartTime = time(nullptr); }

private:
    TPlayer m_players[2];      // İki oyuncu
    DWORD m_dwCRC;             // Hash ID (hızlı arama için)
    bool m_bRevenge;           // İntikam modu aktif mi?
    time_t m_tStartTime;       // Başlama zamanı

    // CRC hesaplama (cCorax2 metodu)
    static DWORD CalcCRC(DWORD pid1, DWORD pid2);
    BYTE FindIndex(DWORD pid) const;  // 0 veya 1
};

/**
 * Global PvP yöneticisi (Singleton)
 * Referans: CPVPManager sınıfı (cCorax2)
 *
 * Modern C++17 özellikleri:
 * - std::mutex ile thread-safe
 * - std::function callbacks (mobile notifications)
 * - Smart pointers
 */
class CPVPManager
{
public:
    // Singleton instance (thread-safe C++11+)
    static CPVPManager& Instance();

    // PvP işlemleri (cCorax2 metodları)
    void Insert(DWORD pid1, DWORD pid2);           // Yeni PvP request
    void Agree(DWORD pid);                         // Request'i kabul et
    void Remove(DWORD pid1, DWORD pid2);           // PvP'yi sona erdir
    void RemoveAll(DWORD pid);                     // Oyuncunun tüm PvP'lerini sil

    // Kontroller (cCorax2 CanAttack bazlı)
    bool CanAttack(CCharacter* pkAttacker, CCharacter* pkVictim);
    bool IsFighting(DWORD pid1, DWORD pid2);
    EPvPState GetPvPState(DWORD pid1, DWORD pid2);

    // Events (cCorax2 Dead() bazlı)
    void OnDeath(CCharacter* pkDead, CCharacter* pkKiller);
    void OnConnect(DWORD pid);
    void OnDisconnect(DWORD pid);

    // Cleanup (cCorax2 Process() - 10 dakika timeout)
    void Process();

    // Mobile callbacks (EKLEME - mobil için)
    using PvPRequestCallback = std::function<void(DWORD requester, DWORD target)>;
    using PvPStartCallback = std::function<void(DWORD pid1, DWORD pid2)>;
    using PvPEndCallback = std::function<void(DWORD winner, DWORD loser)>;
    using RevengeCallback = std::function<void(DWORD victim, DWORD killer)>;

    void SetOnPvPRequest(PvPRequestCallback cb) { m_onPvPRequest = cb; }
    void SetOnPvPStart(PvPStartCallback cb) { m_onPvPStart = cb; }
    void SetOnPvPEnd(PvPEndCallback cb) { m_onPvPEnd = cb; }
    void SetOnRevenge(RevengeCallback cb) { m_onRevenge = cb; }

    // Debugging / Stats
    size_t GetPvPCount() const;
    void Clear();

private:
    // Singleton - private constructor
    CPVPManager() = default;
    ~CPVPManager() = default;
    CPVPManager(const CPVPManager&) = delete;
    CPVPManager& operator=(const CPVPManager&) = delete;

    // PvP bulma metodları
    CPVP* Find(DWORD pid1, DWORD pid2);
    CPVP* FindByPID(DWORD pid);

    // CRC hesaplama
    DWORD GetCRC(DWORD pid1, DWORD pid2) const;

    // Storage (cCorax2 benzeri)
    std::map<DWORD, CPVP> m_map_pvp;        // CRC -> CPVP
    std::map<DWORD, DWORD> m_map_pk_pvp;    // PID -> CRC (hızlı arama)

    // Thread safety (modern C++17)
    mutable std::mutex m_mutex;

    // Mobile callbacks
    PvPRequestCallback m_onPvPRequest;
    PvPStartCallback m_onPvPStart;
    PvPEndCallback m_onPvPEnd;
    RevengeCallback m_onRevenge;

    // Constants
    static constexpr time_t PVPLIST_TIMEOUT = 600;  // 10 dakika (cCorax2)
    static constexpr time_t REVENGE_TIMEOUT = 300;  // 5 dakika (mobil için)
};

#endif // __INC_GAME_PVPMANAGER_H__
