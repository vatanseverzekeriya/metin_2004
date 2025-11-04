# Metin2 PvP - Açık Kaynak Referanslar ve Karşılaştırma

## 🔍 Bulunan Açık Kaynak Metin2 Projeleri

### 1. **cCorax2/Source_code** (Primary Reference)
- **URL**: https://github.com/cCorax2/Source_code
- **Yıldız**: 70+ stars, 34 forks
- **Dil**: C++ (%90.6), C (%6.6), Assembly (%2.8)
- **Yapı**:
  - `/DB` - Veritabanı katmanı
  - `/common` - Ortak tipler ve fonksiyonlar
  - `/game` - Oyun mekaniği (pvp.cpp, char.cpp)
  - `/libthecore` - Temel kütüphaneler

**PvP Sistemi Özellikleri**:
- `CPVP` sınıfı: İki oyuncu arası PvP ilişkisi
- `CPVPManager`: Global PvP yönetimi
- PvP durumları: NONE, AGREE, FIGHT, REVENGE
- CRC hash ile oyuncu kombinasyonu takibi
- 10 dakika timeout sistemi

### 2. **ZeNu-Elijah/Metin2**
- **URL**: https://github.com/ZeNu-Elijah/Metin2
- **Yıldız**: 13 stars, 36 forks
- **Dil**: C++ (%49.7), Python (%48.2)
- **Özellikler**:
  - Advanced Duel Options
  - Mount System
  - Auto Refine
  - Instant Pickup
  - Target Element Management

### 3. **quantum-core-x**
- **URL**: https://github.com/MeikelLP/quantum-core-x
- **Dil**: Modern C#
- **Özellik**: Modern, maintainable server emulator

### 4. **open-mt2**
- **URL**: https://github.com/willianmarquess/open-mt2
- **Dil**: Node.js + TypeScript
- **Özellik**: JavaScript implementasyonu

---

## 📊 Mevcut Proje vs Açık Kaynak Referanslar

### ✅ Mevcut Projede MEVCUT Özellikler

#### 1. **Character Sistemi** (`src/game/Character.cpp`)
```cpp
✓ Temel sınıflar (Warrior, Assassin, Sura, Shaman)
✓ HP/SP sistemi
✓ Level sistemi ve exp kazanımı
✓ Saldırı ve savunma
✓ PvP kill/death istatistikleri
✓ Mesafe tabanlı saldırı kontrolü
✓ Attack speed sistemi
```

#### 2. **PvP Mekaniği**
```cpp
✓ PvP modları: NONE, NORMAL, GUILD, PARTY
✓ CanAttack() kontrolü
✓ OnKill/OnDeath callbacks
✓ Hasar hesaplama formülü
✓ Varyans sistemi (%80-120)
```

#### 3. **Mobil UI** (EKSTRA - Açık kaynakta yok!)
```cpp
✓ Touch input sistemi
✓ Virtual joystick (8 yön)
✓ UI Button sistemi
✓ Gesture detection
✓ Android/iOS entegrasyonu
```

### ❌ Açık Kaynaklarda Olup Bizde OLMAYAN

#### 1. **CPVPManager Sistemi** (cCorax2)
- Dedicated PvP manager sınıfı
- PvP request/agree mekanizması
- Revenge mode
- CRC hash tracking
- Timeout management (10 dakika)

#### 2. **Advanced Duel Options** (ZeNu-Elijah)
- Duel invitations
- Duel rewards
- Duel arenas
- Tournament system

#### 3. **PK Mode Detayları**
- Peace mode (saldırı yok)
- Free mode (herkese saldırı)
- Guild war mode
- Party mode (parti dışına saldırı)

#### 4. **Empire/Kingdom Sistemi**
- 3 krallık (Shinsoo, Chunjo, Jinno)
- Krallıklar arası savaş
- Village protection

---

## 🎯 Öncelikli Eklenecek Özellikler (Açık Kaynak Bazlı)

### Faz 1: Core PvP Sistemi (cCorax2 bazlı)

1. **CPVPManager Sınıfı Ekle**
```cpp
class CPVPManager {
    // PvP request/accept sistemi
    void Insert(DWORD pid1, DWORD pid2);
    void Accept(DWORD pid);
    void Decline(DWORD pid);

    // PvP kontrolü
    bool CanAttack(CHARACTER* attacker, CHARACTER* victim);

    // Revenge mode
    void Dead(CHARACTER* dead, CHARACTER* killer);

    // Cleanup
    void Process(); // 10 dakika timeout
};
```

2. **PvP Durumları Genişlet**
```cpp
enum EPvPState {
    PVP_STATE_NONE,
    PVP_STATE_REQUEST,    // Tek taraf istedi
    PVP_STATE_AGREE,      // İki taraf kabul etti
    PVP_STATE_FIGHT,      // Aktif savaş
    PVP_STATE_REVENGE     // İntikam modu
};
```

3. **PK Mode Detayları**
```cpp
enum EPKMode {
    PK_MODE_PEACE = 0,    // Hiç saldırı yok
    PK_MODE_NORMAL = 1,   // PvP request ile
    PK_MODE_FREE = 2,     // Herkese
    PK_MODE_PROTECT = 3   // Guild dışına
};
```

### Faz 2: Advanced Features (ZeNu-Elijah bazlı)

1. **Duel System**
   - Duel invitations
   - Arena teleportation
   - Duel rewards (gold, exp)
   - Spectator mode

2. **Guild War**
   - Guild vs Guild battles
   - Guild rankings
   - War rewards

3. **Empire System**
   - 3 krallık sistemi
   - Inter-empire wars
   - Territory control

---

## 📱 Mobil Adaptasyon Stratejisi

### Bizim GÜÇLÜ Yanlarımız (Açık kaynaklarda yok!)

```
✓ Touch input sistemi (multi-touch)
✓ Virtual joystick
✓ Mobile UI components
✓ Gesture system
✓ Network optimization (UDP/WebSocket)
✓ Android/iOS integration ready
```

### Mobil İçin Geliştirme Planı

#### 1. **PvP Mobile Kontrolları**
```cpp
// Attack button ile otomatik hedef
CGameControls::SetOnAttackCallback([]() {
    auto* target = FindNearestEnemy();
    if (target && CanAttack(target)) {
        Attack(target);
    }
});

// Swipe ile duel request
CTouchInput::RegisterGestureCallback([](const GestureInfo& gesture) {
    if (gesture.type == GESTURE_SWIPE && gesture.target) {
        SendPvPRequest(gesture.target);
    }
});
```

#### 2. **Auto-targeting Sistemi** (Mobil için kritik!)
```cpp
CCharacter* FindNearestEnemy(float range = 500.0f) {
    // En yakın düşmanı bul
    // PvP mode, empire, guild kontrolü
    // Return nullptr if none
}
```

#### 3. **Simplified PvP UI**
```
[Target Info]
━━━━━━━━━━━━━━━━━━━━━━
Name: EnemyWarrior
HP: ████████░░ 80%
Level: 45

[PvP Request?]
[ Accept ] [ Decline ]
```

---

## 🔧 Teknik Karşılaştırma

### Kod Kalitesi

| Feature | cCorax2 | Bizim Proje | Sonuç |
|---------|---------|-------------|--------|
| C++ Standard | C++98 | C++17 | ✅ **Daha modern** |
| Thread Safety | Partial | std::mutex | ✅ **Daha güvenli** |
| Database | MySQL C API | MySQL C API | ✔️ Aynı |
| PvP Manager | ✅ Var | ❌ Yok | 🔴 **Ekle** |
| Mobile Support | ❌ Yok | ✅ Var | ✅ **Bizde var!** |
| Network | TCP only | TCP/UDP/WS | ✅ **Daha esnek** |

### Mimari Karşılaştırma

**cCorax2 Mimarisi:**
```
Client → GameServer → CharacterManager
                   → PVPManager ✓
                   → DBManager
```

**Bizim Mimari:**
```
Mobile Client → NetworkManager (UDP/WS/TCP)
              → GameServer
              → Character (PvP embedded)
              → DBManager
              → TouchInput/GameControls ✓
```

**İdeal Hibrit Mimari:**
```
Mobile Client → NetworkManager
              → GameServer
              → CharacterManager
              → PVPManager (EKLE!) ✓
              → DBManager
              → MobileUI ✓
```

---

## 📝 Aksiyon Planı

### Kısa Vade (1-2 hafta)

1. ✅ **PVPManager sınıfı ekle** (cCorax2 bazlı)
   - Request/Accept sistemi
   - CRC tracking
   - Timeout management

2. ✅ **PK Mode genişlet**
   - Peace, Normal, Free, Protect modları
   - Mode değiştirme UI (mobile)

3. ✅ **Revenge sistemi**
   - OnKill/OnDeath hook'ları
   - Revenge timer (5 dakika)

### Orta Vade (2-4 hafta)

4. **Duel System** (ZeNu-Elijah bazlı)
   - Duel invitations
   - Arena system
   - Rewards

5. **Auto-targeting** (Mobile için)
   - Smart targeting algorithm
   - Touch-to-target
   - Target lock

6. **Guild War temel**
   - Guild structure
   - War declaration
   - Simple scoring

### Uzun Vade (1-2 ay)

7. **Empire System**
   - 3 krallık
   - Territory wars
   - Siege mechanics

8. **Tournament System**
   - Ranked PvP
   - Leaderboards
   - Season rewards

9. **Advanced Mobile Features**
   - Spectator mode
   - Replay system
   - Social features

---

## 📚 Referans Dosyaları

### Kritik Dosyalar (cCorax2)

1. **pvp.cpp** - Ana PvP mantığı
   - https://github.com/cCorax2/Source_code/blob/master/game/pvp.cpp

2. **char.cpp** - Karakter yönetimi
   - https://github.com/cCorax2/Source_code/blob/master/game/char.cpp

3. **char_battle.cpp** - Savaş mekaniği
   - Combat calculations
   - Damage formulas
   - Skill usage

### İncelenecek Özellikler (ZeNu-Elijah)

1. **Advanced Duel Options**
   - `/Advance Duel Options/Source/Server/game/pvp.cpp`

2. **Target Element**
   - Smart targeting system

---

## 🎯 Sonuç ve Öneriler

### Güçlü Yanlarımız ✅
1. **Modern C++17** kodumuz daha temiz
2. **Mobil desteği** açık kaynaklarda yok!
3. **Network esnekliği** (UDP/WebSocket)
4. **Thread-safe** tasarım

### Eksiklerimiz 🔴
1. Dedicated **PVPManager** yok
2. **Duel/Arena** sistemi yok
3. **Revenge mode** eksik
4. **Guild war** sistemi yok

### Strateji 🎯

**"En iyilerini al, mobil için optimize et!"**

1. cCorax2'den **PVPManager** mimarisini al
2. ZeNu-Elijah'tan **Advanced Duel** fikirlerini al
3. Kendi **Mobile UI/Controls** avantajımızı koru
4. Modern **C++17** kodlama standartlarını kullan
5. Mobile-first yaklaşımla **auto-targeting** ekle

### Hedef

**"Açık kaynak Metin2 PvP sisteminin mobil platformlar için modernize edilmiş versiyonu"**

- ✅ Desktop Metin2'nin tüm PvP özellikleri
- ✅ Mobile-optimized controls
- ✅ Modern C++ architecture
- ✅ Cloud-ready networking (UDP/WebSocket)

---

## 📞 Sonraki Adımlar

1. **PVPManager** implementasyonu başlat
2. **PK modes** genişletmesi
3. **Mobile PvP UI** tasarımı
4. **Test senaryoları** yaz
5. **Performans optimizasyonu** (mobil için kritik!)

---

**Son Güncelleme**: 2025-11-04
**Referanslar**: cCorax2, ZeNu-Elijah, quantum-core-x, open-mt2
**Durum**: Analiz tamamlandı, implementasyon planı hazır ✅
