/**
 * Metin2 PvP Server - Kimlik Doğrulama ve Veritabanı Sistemi Test Programı
 *
 * Bu örnek, veritabanı bağlantısı, hesap kaydı, giriş ve oturum yönetiminin
 * nasıl kullanılacağını gösterir.
 */

#include "../../include/db/DBManager.h"
#include "../../include/auth/AuthManager.h"
#include "../../include/auth/SessionManager.h"
#include "../../include/common/types.h"
#include <iostream>
#include <string>
#include <vector>

// ANSI renk kodları (terminal çıktısı için)
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

void PrintSeparator()
{
    std::cout << COLOR_CYAN << "========================================" << COLOR_RESET << std::endl;
}

void PrintSuccess(const std::string& msg)
{
    std::cout << COLOR_GREEN << "[SUCCESS] " << msg << COLOR_RESET << std::endl;
}

void PrintError(const std::string& msg)
{
    std::cout << COLOR_RED << "[ERROR] " << msg << COLOR_RESET << std::endl;
}

void PrintInfo(const std::string& msg)
{
    std::cout << COLOR_BLUE << "[INFO] " << msg << COLOR_RESET << std::endl;
}

// =============================================================================
// Test 1: Veritabanı Bağlantısı
// =============================================================================
bool TestDatabaseConnection()
{
    PrintSeparator();
    PrintInfo("Test 1: Veritabanı Bağlantısı");
    PrintSeparator();

    CDBManager& db = CDBManager::Instance();

    // Veritabanı bilgileri (kendi ayarlarınıza göre değiştirin)
    std::string host = "localhost";
    std::string user = "root";
    std::string password = ""; // Kendi şifrenizi girin
    std::string database = "metin2";
    int port = 3306;

    if (db.Initialize(host, user, password, database, port))
    {
        PrintSuccess("Veritabanı bağlantısı başarılı!");
        return true;
    }
    else
    {
        PrintError("Veritabanı bağlantısı başarısız!");
        return false;
    }
}

// =============================================================================
// Test 2: Hesap Kaydı (Register)
// =============================================================================
bool TestAccountRegistration()
{
    PrintSeparator();
    PrintInfo("Test 2: Hesap Kaydı");
    PrintSeparator();

    CAuthManager& auth = CAuthManager::Instance();

    // Test hesabı bilgileri
    std::string login = "testplayer";
    std::string password = "test123";
    std::string email = "testplayer@example.com";

    std::cout << "Hesap kaydediliyor: " << login << std::endl;

    ERegisterResult result = auth.RegisterAccount(login, password, email);

    switch (result)
    {
        case REGISTER_SUCCESS:
            PrintSuccess("Hesap başarıyla kaydedildi!");
            return true;

        case REGISTER_FAILED_ALREADY_EXISTS:
            PrintInfo("Hesap zaten mevcut (bu normal, test hesabı önceden oluşturulmuş olabilir)");
            return true; // Test için başarı sayıyoruz

        case REGISTER_FAILED_INVALID_LOGIN:
            PrintError("Geçersiz kullanıcı adı!");
            return false;

        case REGISTER_FAILED_INVALID_PASSWORD:
            PrintError("Geçersiz şifre!");
            return false;

        case REGISTER_FAILED_INVALID_EMAIL:
            PrintError("Geçersiz email!");
            return false;

        case REGISTER_FAILED_DB_ERROR:
            PrintError("Veritabanı hatası!");
            return false;

        default:
            PrintError("Bilinmeyen hata!");
            return false;
    }
}

// =============================================================================
// Test 3: Hesap Girişi (Login)
// =============================================================================
bool TestAccountLogin(std::string& out_session_key)
{
    PrintSeparator();
    PrintInfo("Test 3: Hesap Girişi");
    PrintSeparator();

    CAuthManager& auth = CAuthManager::Instance();

    std::string login = "testplayer";
    std::string password = "test123";
    std::string ip_address = "127.0.0.1";

    std::cout << "Giriş yapılıyor: " << login << std::endl;

    EAuthResult result = auth.Login(login, password, ip_address, out_session_key);

    switch (result)
    {
        case AUTH_SUCCESS:
            PrintSuccess("Giriş başarılı!");
            std::cout << "Session Key: " << COLOR_YELLOW << out_session_key << COLOR_RESET << std::endl;
            return true;

        case AUTH_FAILED_INVALID_CREDENTIALS:
            PrintError("Geçersiz kullanıcı adı veya şifre!");
            return false;

        case AUTH_FAILED_ALREADY_LOGGED_IN:
            PrintError("Hesap zaten giriş yapmış!");
            return false;

        case AUTH_FAILED_DB_ERROR:
            PrintError("Veritabanı hatası!");
            return false;

        default:
            PrintError("Bilinmeyen hata!");
            return false;
    }
}

// =============================================================================
// Test 4: Session Doğrulama
// =============================================================================
bool TestSessionValidation(const std::string& session_key)
{
    PrintSeparator();
    PrintInfo("Test 4: Session Doğrulama");
    PrintSeparator();

    CAuthManager& auth = CAuthManager::Instance();

    if (auth.ValidateSession(session_key))
    {
        PrintSuccess("Session geçerli!");

        // Account ID'yi al
        DWORD account_id = auth.GetAccountIDBySession(session_key);
        std::cout << "Account ID: " << COLOR_YELLOW << account_id << COLOR_RESET << std::endl;

        return true;
    }
    else
    {
        PrintError("Session geçersiz!");
        return false;
    }
}

// =============================================================================
// Test 5: Karakter Listesi
// =============================================================================
bool TestCharacterList(const std::string& session_key)
{
    PrintSeparator();
    PrintInfo("Test 5: Karakter Listesi");
    PrintSeparator();

    CAuthManager& auth = CAuthManager::Instance();
    std::vector<std::pair<DWORD, std::string>> characters;

    if (auth.GetCharacterList(session_key, characters))
    {
        PrintSuccess("Karakter listesi alındı!");

        if (characters.empty())
        {
            std::cout << "Henüz karakter yok." << std::endl;
        }
        else
        {
            std::cout << "Karakterler:" << std::endl;
            for (size_t i = 0; i < characters.size(); i++)
            {
                std::cout << "  " << (i + 1) << ". "
                          << COLOR_YELLOW << characters[i].second << COLOR_RESET
                          << " (ID: " << characters[i].first << ")" << std::endl;
            }
        }

        return true;
    }
    else
    {
        PrintError("Karakter listesi alınamadı!");
        return false;
    }
}

// =============================================================================
// Test 6: Karakter Oluşturma
// =============================================================================
bool TestCharacterCreation(const std::string& session_key, DWORD& out_player_id)
{
    PrintSeparator();
    PrintInfo("Test 6: Karakter Oluşturma");
    PrintSeparator();

    CAuthManager& auth = CAuthManager::Instance();

    std::string char_name = "TestWarrior";
    BYTE job = CLASS_WARRIOR; // 0 = Warrior

    std::cout << "Karakter oluşturuluyor: " << char_name << " (Sınıf: Warrior)" << std::endl;

    ECreateCharacterResult result = auth.CreateCharacter(session_key, char_name, job, out_player_id);

    switch (result)
    {
        case CREATE_CHARACTER_SUCCESS:
            PrintSuccess("Karakter başarıyla oluşturuldu!");
            std::cout << "Player ID: " << COLOR_YELLOW << out_player_id << COLOR_RESET << std::endl;
            return true;

        case CREATE_CHARACTER_FAILED_NAME_EXISTS:
            PrintInfo("Karakter ismi zaten kullanılıyor (test için normal)");
            // Mevcut karakteri bul
            {
                std::vector<std::pair<DWORD, std::string>> characters;
                auth.GetCharacterList(session_key, characters);
                for (const auto& ch : characters)
                {
                    if (ch.second == char_name)
                    {
                        out_player_id = ch.first;
                        std::cout << "Mevcut karakter kullanılacak - Player ID: "
                                  << COLOR_YELLOW << out_player_id << COLOR_RESET << std::endl;
                        return true;
                    }
                }
            }
            return false;

        case CREATE_CHARACTER_FAILED_INVALID_NAME:
            PrintError("Geçersiz karakter ismi!");
            return false;

        case CREATE_CHARACTER_FAILED_INVALID_JOB:
            PrintError("Geçersiz sınıf!");
            return false;

        case CREATE_CHARACTER_FAILED_MAX_CHARACTERS:
            PrintError("Maksimum karakter sayısına ulaşıldı!");
            return false;

        default:
            PrintError("Karakter oluşturulamadı!");
            return false;
    }
}

// =============================================================================
// Test 7: Karakter Seçimi
// =============================================================================
bool TestCharacterSelection(const std::string& session_key, DWORD player_id)
{
    PrintSeparator();
    PrintInfo("Test 7: Karakter Seçimi");
    PrintSeparator();

    CAuthManager& auth = CAuthManager::Instance();

    std::cout << "Karakter seçiliyor (Player ID: " << player_id << ")" << std::endl;

    if (auth.SelectCharacter(session_key, player_id))
    {
        PrintSuccess("Karakter başarıyla seçildi!");
        return true;
    }
    else
    {
        PrintError("Karakter seçilemedi!");
        return false;
    }
}

// =============================================================================
// Test 8: Oyuncu Verilerini Yükleme ve Kaydetme
// =============================================================================
bool TestPlayerDataOperations(DWORD player_id)
{
    PrintSeparator();
    PrintInfo("Test 8: Oyuncu Verilerini Yükleme ve Kaydetme");
    PrintSeparator();

    CDBManager& db = CDBManager::Instance();

    // Oyuncu verilerini yükle
    TPlayerStats stats;
    TPosition pos;

    std::cout << "Oyuncu verileri yükleniyor..." << std::endl;

    if (db.LoadPlayer(player_id, stats, pos))
    {
        PrintSuccess("Oyuncu verileri yüklendi!");

        std::cout << COLOR_MAGENTA << "Oyuncu İstatistikleri:" << COLOR_RESET << std::endl;
        std::cout << "  Seviye: " << stats.level << std::endl;
        std::cout << "  Deneyim: " << stats.exp << std::endl;
        std::cout << "  Altın: " << stats.gold << std::endl;
        std::cout << "  HP: " << stats.hp << "/" << stats.max_hp << std::endl;
        std::cout << "  SP: " << stats.sp << "/" << stats.max_sp << std::endl;
        std::cout << "  Saldırı: " << stats.attack << std::endl;
        std::cout << "  Savunma: " << stats.defense << std::endl;
        std::cout << "  Pozisyon: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;

        // İstatistikleri değiştir ve kaydet
        std::cout << "\nOyuncu verileri güncelleniyor..." << std::endl;
        stats.level += 1;
        stats.exp += 1000;
        stats.gold += 5000;
        pos.x += 100;
        pos.y += 100;

        if (db.SavePlayer(player_id, stats, pos))
        {
            PrintSuccess("Oyuncu verileri kaydedildi!");
            std::cout << "  Yeni Seviye: " << stats.level << std::endl;
            std::cout << "  Yeni Deneyim: " << stats.exp << std::endl;
            std::cout << "  Yeni Altın: " << stats.gold << std::endl;
            return true;
        }
        else
        {
            PrintError("Oyuncu verileri kaydedilemedi!");
            return false;
        }
    }
    else
    {
        PrintError("Oyuncu verileri yüklenemedi!");
        return false;
    }
}

// =============================================================================
// Test 9: PvP İstatistikleri
// =============================================================================
bool TestPvPStats(DWORD player_id)
{
    PrintSeparator();
    PrintInfo("Test 9: PvP İstatistikleri");
    PrintSeparator();

    CDBManager& db = CDBManager::Instance();

    // Mevcut PvP istatistiklerini al
    DWORD kills = 0, deaths = 0;

    std::cout << "PvP istatistikleri alınıyor..." << std::endl;

    if (db.GetPvPStats(player_id, kills, deaths))
    {
        PrintSuccess("PvP istatistikleri alındı!");
        std::cout << "  Öldürme: " << COLOR_GREEN << kills << COLOR_RESET << std::endl;
        std::cout << "  Ölüm: " << COLOR_RED << deaths << COLOR_RESET << std::endl;

        float kd_ratio = (deaths > 0) ? (float)kills / deaths : (float)kills;
        std::cout << "  K/D Oranı: " << COLOR_YELLOW << kd_ratio << COLOR_RESET << std::endl;

        // İstatistikleri güncelle
        std::cout << "\nPvP istatistikleri güncelleniyor..." << std::endl;
        kills += 1;

        if (db.UpdatePvPStats(player_id, kills, deaths))
        {
            PrintSuccess("PvP istatistikleri güncellendi!");
            std::cout << "  Yeni Öldürme Sayısı: " << COLOR_GREEN << kills << COLOR_RESET << std::endl;
            return true;
        }
        else
        {
            PrintError("PvP istatistikleri güncellenemedi!");
            return false;
        }
    }
    else
    {
        PrintError("PvP istatistikleri alınamadı!");
        return false;
    }
}

// =============================================================================
// Test 10: Session Temizleme ve Çıkış
// =============================================================================
bool TestLogout(const std::string& session_key)
{
    PrintSeparator();
    PrintInfo("Test 10: Çıkış (Logout)");
    PrintSeparator();

    CAuthManager& auth = CAuthManager::Instance();

    std::cout << "Çıkış yapılıyor..." << std::endl;

    if (auth.Logout(session_key))
    {
        PrintSuccess("Çıkış başarılı!");

        // Session'ın artık geçersiz olduğunu kontrol et
        if (!auth.ValidateSession(session_key))
        {
            PrintSuccess("Session başarıyla temizlendi!");
            return true;
        }
        else
        {
            PrintError("Session hala geçerli!");
            return false;
        }
    }
    else
    {
        PrintError("Çıkış başarısız!");
        return false;
    }
}

// =============================================================================
// Ana Test Fonksiyonu
// =============================================================================
int main()
{
    std::cout << COLOR_CYAN << R"(
╔════════════════════════════════════════════════════════════════╗
║                                                                ║
║            Metin2 PvP Server - Auth System Test               ║
║                                                                ║
║  Bu test programı veritabanı, kayıt, giriş ve oturum          ║
║  yönetimi sistemlerinin tüm özelliklerini test eder.          ║
║                                                                ║
╚════════════════════════════════════════════════════════════════╝
)" << COLOR_RESET << std::endl;

    int tests_passed = 0;
    int tests_total = 10;

    std::string session_key;
    DWORD player_id = 0;

    // Test 1: Veritabanı Bağlantısı
    if (TestDatabaseConnection())
        tests_passed++;
    else
    {
        PrintError("Veritabanı bağlantısı başarısız! Diğer testler çalıştırılamaz.");
        return 1;
    }

    // Test 2: Hesap Kaydı
    if (TestAccountRegistration())
        tests_passed++;

    // Test 3: Hesap Girişi
    if (TestAccountLogin(session_key))
        tests_passed++;
    else
    {
        PrintError("Giriş başarısız! Diğer testler çalıştırılamaz.");
        return 1;
    }

    // Test 4: Session Doğrulama
    if (TestSessionValidation(session_key))
        tests_passed++;

    // Test 5: Karakter Listesi
    if (TestCharacterList(session_key))
        tests_passed++;

    // Test 6: Karakter Oluşturma
    if (TestCharacterCreation(session_key, player_id))
        tests_passed++;

    // Test 7: Karakter Seçimi
    if (TestCharacterSelection(session_key, player_id))
        tests_passed++;

    // Test 8: Oyuncu Verilerini Yükleme ve Kaydetme
    if (TestPlayerDataOperations(player_id))
        tests_passed++;

    // Test 9: PvP İstatistikleri
    if (TestPvPStats(player_id))
        tests_passed++;

    // Test 10: Çıkış
    if (TestLogout(session_key))
        tests_passed++;

    // Sonuçları göster
    PrintSeparator();
    std::cout << COLOR_CYAN << "\n╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║          TEST SONUÇLARI                ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << COLOR_RESET << std::endl;

    std::cout << "Geçen Testler: " << COLOR_GREEN << tests_passed << COLOR_RESET
              << " / " << tests_total << std::endl;

    if (tests_passed == tests_total)
    {
        PrintSuccess("Tüm testler başarıyla geçti!");
    }
    else
    {
        PrintError("Bazı testler başarısız oldu!");
    }

    PrintSeparator();

    return (tests_passed == tests_total) ? 0 : 1;
}
