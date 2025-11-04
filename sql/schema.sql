-- Metin2 PvP Server Database Schema
-- MySQL/MariaDB Database

-- Veritabanı oluştur
CREATE DATABASE IF NOT EXISTS metin2 CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE metin2;

-- Hesap tablosu
CREATE TABLE IF NOT EXISTS account (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    login VARCHAR(32) NOT NULL UNIQUE,
    password VARCHAR(128) NOT NULL,
    email VARCHAR(128),
    status ENUM('OK', 'BLOCK') DEFAULT 'OK',
    availDt DATETIME,
    create_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    last_login DATETIME,
    INDEX idx_login (login)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Oyuncu tablosu
CREATE TABLE IF NOT EXISTS player (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    account_id INT UNSIGNED NOT NULL,
    name VARCHAR(24) NOT NULL UNIQUE,
    job TINYINT UNSIGNED NOT NULL DEFAULT 0,
    voice TINYINT UNSIGNED NOT NULL DEFAULT 0,
    dir TINYINT UNSIGNED NOT NULL DEFAULT 0,

    -- Pozisyon
    pos_x INT NOT NULL DEFAULT 957200,
    pos_y INT NOT NULL DEFAULT 244900,
    pos_z INT NOT NULL DEFAULT 0,
    map_index INT NOT NULL DEFAULT 1,

    -- İstatistikler
    level TINYINT UNSIGNED NOT NULL DEFAULT 1,
    exp INT UNSIGNED NOT NULL DEFAULT 0,
    gold BIGINT UNSIGNED NOT NULL DEFAULT 0,

    -- Can ve Mana
    hp INT UNSIGNED NOT NULL DEFAULT 1000,
    max_hp INT UNSIGNED NOT NULL DEFAULT 1000,
    sp INT UNSIGNED NOT NULL DEFAULT 100,
    max_sp INT UNSIGNED NOT NULL DEFAULT 100,

    -- Savaş özellikleri
    attack INT UNSIGNED NOT NULL DEFAULT 50,
    defense INT UNSIGNED NOT NULL DEFAULT 30,
    magic_attack INT UNSIGNED NOT NULL DEFAULT 20,
    magic_defense INT UNSIGNED NOT NULL DEFAULT 20,

    -- PvP istatistikleri
    pvp_kills INT UNSIGNED NOT NULL DEFAULT 0,
    pvp_deaths INT UNSIGNED NOT NULL DEFAULT 0,
    pvp_points INT UNSIGNED NOT NULL DEFAULT 0,

    -- Temel özellikler
    st TINYINT UNSIGNED NOT NULL DEFAULT 1,  -- Strength
    ht TINYINT UNSIGNED NOT NULL DEFAULT 1,  -- Health
    dx TINYINT UNSIGNED NOT NULL DEFAULT 1,  -- Dexterity
    iq TINYINT UNSIGNED NOT NULL DEFAULT 1,  -- Intelligence

    -- Stat puanları
    stat_point SMALLINT UNSIGNED NOT NULL DEFAULT 0,
    skill_point SMALLINT UNSIGNED NOT NULL DEFAULT 0,

    -- Zamanlar
    create_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    last_login DATETIME,
    play_time INT UNSIGNED NOT NULL DEFAULT 0,

    FOREIGN KEY (account_id) REFERENCES account(id) ON DELETE CASCADE,
    INDEX idx_account (account_id),
    INDEX idx_name (name),
    INDEX idx_level (level)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Envanter tablosu
CREATE TABLE IF NOT EXISTS item (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    owner_id INT UNSIGNED NOT NULL,
    window ENUM('INVENTORY', 'EQUIPMENT', 'DRAGON_SOUL', 'BELT') DEFAULT 'INVENTORY',
    pos TINYINT UNSIGNED NOT NULL,
    vnum INT UNSIGNED NOT NULL,
    count SMALLINT UNSIGNED NOT NULL DEFAULT 1,

    -- Item özellikleri
    socket0 INT UNSIGNED NOT NULL DEFAULT 0,
    socket1 INT UNSIGNED NOT NULL DEFAULT 0,
    socket2 INT UNSIGNED NOT NULL DEFAULT 0,

    -- Bonuslar
    attrtype0 TINYINT UNSIGNED NOT NULL DEFAULT 0,
    attrvalue0 SMALLINT NOT NULL DEFAULT 0,
    attrtype1 TINYINT UNSIGNED NOT NULL DEFAULT 0,
    attrvalue1 SMALLINT NOT NULL DEFAULT 0,
    attrtype2 TINYINT UNSIGNED NOT NULL DEFAULT 0,
    attrvalue2 SMALLINT NOT NULL DEFAULT 0,
    attrtype3 TINYINT UNSIGNED NOT NULL DEFAULT 0,
    attrvalue3 SMALLINT NOT NULL DEFAULT 0,
    attrtype4 TINYINT UNSIGNED NOT NULL DEFAULT 0,
    attrvalue4 SMALLINT NOT NULL DEFAULT 0,

    FOREIGN KEY (owner_id) REFERENCES player(id) ON DELETE CASCADE,
    INDEX idx_owner (owner_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Guild tablosu
CREATE TABLE IF NOT EXISTS guild (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(24) NOT NULL UNIQUE,
    sp INT UNSIGNED NOT NULL DEFAULT 1000,
    master INT UNSIGNED NOT NULL,
    level TINYINT UNSIGNED NOT NULL DEFAULT 1,
    exp INT UNSIGNED NOT NULL DEFAULT 0,
    skill_point TINYINT UNSIGNED NOT NULL DEFAULT 0,
    win INT UNSIGNED NOT NULL DEFAULT 0,
    draw INT UNSIGNED NOT NULL DEFAULT 0,
    loss INT UNSIGNED NOT NULL DEFAULT 0,
    ladder_point INT NOT NULL DEFAULT 0,

    FOREIGN KEY (master) REFERENCES player(id),
    INDEX idx_name (name),
    INDEX idx_ladder (ladder_point)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Guild üyelik tablosu
CREATE TABLE IF NOT EXISTS guild_member (
    guild_id INT UNSIGNED NOT NULL,
    player_id INT UNSIGNED NOT NULL,
    grade TINYINT UNSIGNED NOT NULL DEFAULT 15,

    PRIMARY KEY (guild_id, player_id),
    FOREIGN KEY (guild_id) REFERENCES guild(id) ON DELETE CASCADE,
    FOREIGN KEY (player_id) REFERENCES player(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Quest durumu tablosu
CREATE TABLE IF NOT EXISTS quest (
    player_id INT UNSIGNED NOT NULL,
    quest_name VARCHAR(64) NOT NULL,
    state VARCHAR(32) NOT NULL DEFAULT 'start',
    flag TEXT,

    PRIMARY KEY (player_id, quest_name),
    FOREIGN KEY (player_id) REFERENCES player(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- PvP loglama tablosu
CREATE TABLE IF NOT EXISTS pvp_log (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    killer_id INT UNSIGNED NOT NULL,
    victim_id INT UNSIGNED NOT NULL,
    kill_time DATETIME DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (killer_id) REFERENCES player(id),
    FOREIGN KEY (victim_id) REFERENCES player(id),
    INDEX idx_killer (killer_id),
    INDEX idx_victim (victim_id),
    INDEX idx_time (kill_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Test verileri ekle
INSERT INTO account (login, password, email, status) VALUES
    ('test', PASSWORD('test123'), 'test@metin2.com', 'OK'),
    ('admin', PASSWORD('admin123'), 'admin@metin2.com', 'OK');

-- Başarıyla oluşturuldu mesajı
SELECT 'Database schema created successfully!' as Status;
