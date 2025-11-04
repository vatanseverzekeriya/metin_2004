-- Metin2 PvP Server Database Schema
-- MySQL 5.7+ uyumludur

-- Veritabanı oluştur
CREATE DATABASE IF NOT EXISTS metin2 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE metin2;

-- ============================================================================
-- Hesap Tablosu (account)
-- ============================================================================
DROP TABLE IF EXISTS account;
CREATE TABLE account (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    login VARCHAR(16) NOT NULL UNIQUE,
    password CHAR(64) NOT NULL COMMENT 'SHA256 hash',
    email VARCHAR(100) NOT NULL,
    status TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=active, 1=blocked',
    create_time DATETIME NOT NULL,
    last_login DATETIME NOT NULL,
    PRIMARY KEY (id),
    INDEX idx_login (login),
    INDEX idx_email (email),
    INDEX idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Oyuncu hesapları';

-- ============================================================================
-- Oyuncu Tablosu (player)
-- ============================================================================
DROP TABLE IF EXISTS player;
CREATE TABLE player (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    account_id INT UNSIGNED NOT NULL,
    name VARCHAR(24) NOT NULL UNIQUE,
    job TINYINT UNSIGNED NOT NULL COMMENT '0=Warrior, 1=Assassin, 2=Sura, 3=Shaman',

    -- Seviye ve deneyim
    level INT UNSIGNED NOT NULL DEFAULT 1,
    exp BIGINT UNSIGNED NOT NULL DEFAULT 0,
    gold BIGINT UNSIGNED NOT NULL DEFAULT 0,

    -- Sağlık ve enerji
    hp INT UNSIGNED NOT NULL DEFAULT 1000,
    max_hp INT UNSIGNED NOT NULL DEFAULT 1000,
    sp INT UNSIGNED NOT NULL DEFAULT 100,
    max_sp INT UNSIGNED NOT NULL DEFAULT 100,

    -- Savaş özellikleri
    attack INT UNSIGNED NOT NULL DEFAULT 50,
    defense INT UNSIGNED NOT NULL DEFAULT 30,
    magic_attack INT UNSIGNED NOT NULL DEFAULT 20,
    magic_defense INT UNSIGNED NOT NULL DEFAULT 20,

    -- Pozisyon
    pos_x BIGINT NOT NULL DEFAULT 957200,
    pos_y BIGINT NOT NULL DEFAULT 244900,
    pos_z BIGINT NOT NULL DEFAULT 0,

    -- PvP istatistikleri
    pvp_kills INT UNSIGNED NOT NULL DEFAULT 0,
    pvp_deaths INT UNSIGNED NOT NULL DEFAULT 0,

    -- Zaman bilgileri
    create_time DATETIME NOT NULL,
    last_play DATETIME DEFAULT NULL,
    play_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Toplam oyun süresi (saniye)',

    PRIMARY KEY (id),
    INDEX idx_account_id (account_id),
    INDEX idx_name (name),
    INDEX idx_level (level),
    INDEX idx_pvp_kills (pvp_kills DESC),
    FOREIGN KEY (account_id) REFERENCES account(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Oyuncu karakterleri';

-- ============================================================================
-- Envanter Tablosu (inventory)
-- ============================================================================
DROP TABLE IF EXISTS inventory;
CREATE TABLE inventory (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    player_id INT UNSIGNED NOT NULL,
    item_vnum INT UNSIGNED NOT NULL COMMENT 'Item ID',
    count INT UNSIGNED NOT NULL DEFAULT 1,
    slot_pos TINYINT UNSIGNED NOT NULL COMMENT 'Envanter slotu',
    socket0 INT UNSIGNED NOT NULL DEFAULT 0,
    socket1 INT UNSIGNED NOT NULL DEFAULT 0,
    socket2 INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (id),
    INDEX idx_player_id (player_id),
    INDEX idx_item_vnum (item_vnum),
    FOREIGN KEY (player_id) REFERENCES player(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Oyuncu envanteri';

-- ============================================================================
-- PvP Log Tablosu (pvp_log)
-- ============================================================================
DROP TABLE IF EXISTS pvp_log;
CREATE TABLE pvp_log (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    killer_id INT UNSIGNED NOT NULL,
    victim_id INT UNSIGNED NOT NULL,
    killer_level INT UNSIGNED NOT NULL,
    victim_level INT UNSIGNED NOT NULL,
    pos_x BIGINT NOT NULL,
    pos_y BIGINT NOT NULL,
    kill_time DATETIME NOT NULL,
    PRIMARY KEY (id),
    INDEX idx_killer_id (killer_id),
    INDEX idx_victim_id (victim_id),
    INDEX idx_kill_time (kill_time DESC),
    FOREIGN KEY (killer_id) REFERENCES player(id) ON DELETE CASCADE,
    FOREIGN KEY (victim_id) REFERENCES player(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='PvP öldürme kayıtları';

-- ============================================================================
-- Sıralama Tablosu (rankings)
-- ============================================================================
DROP TABLE IF EXISTS rankings;
CREATE TABLE rankings (
    player_id INT UNSIGNED NOT NULL,
    rank_type ENUM('level', 'pvp_kills', 'gold') NOT NULL,
    rank_position INT UNSIGNED NOT NULL,
    rank_value BIGINT UNSIGNED NOT NULL,
    update_time DATETIME NOT NULL,
    PRIMARY KEY (player_id, rank_type),
    INDEX idx_rank_type (rank_type, rank_position),
    INDEX idx_update_time (update_time),
    FOREIGN KEY (player_id) REFERENCES player(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Oyuncu sıralamaları';

-- ============================================================================
-- Lonca Tablosu (guild)
-- ============================================================================
DROP TABLE IF EXISTS guild;
CREATE TABLE guild (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT,
    name VARCHAR(24) NOT NULL UNIQUE,
    leader_id INT UNSIGNED NOT NULL,
    level TINYINT UNSIGNED NOT NULL DEFAULT 1,
    exp INT UNSIGNED NOT NULL DEFAULT 0,
    create_time DATETIME NOT NULL,
    PRIMARY KEY (id),
    INDEX idx_name (name),
    INDEX idx_leader_id (leader_id),
    FOREIGN KEY (leader_id) REFERENCES player(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Loncalar';

-- ============================================================================
-- Lonca Üyeleri Tablosu (guild_member)
-- ============================================================================
DROP TABLE IF EXISTS guild_member;
CREATE TABLE guild_member (
    guild_id INT UNSIGNED NOT NULL,
    player_id INT UNSIGNED NOT NULL,
    grade TINYINT UNSIGNED NOT NULL DEFAULT 15 COMMENT '1=Leader, 15=Member',
    join_time DATETIME NOT NULL,
    PRIMARY KEY (guild_id, player_id),
    INDEX idx_player_id (player_id),
    FOREIGN KEY (guild_id) REFERENCES guild(id) ON DELETE CASCADE,
    FOREIGN KEY (player_id) REFERENCES player(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Lonca üyeleri';

-- ============================================================================
-- Örnek Veriler (Test için)
-- ============================================================================

-- Test hesabı (login: testuser, password: test123)
-- Password: SHA256("test123") = ecd71870d1963316a97e3ac3408c9835ad8cf0f3c1bc703527c30265534f75ae
INSERT INTO account (login, password, email, create_time, last_login) VALUES
('testuser', 'ecd71870d1963316a97e3ac3408c9835ad8cf0f3c1bc703527c30265534f75ae', 'test@example.com', NOW(), NOW());

-- Test karakteri
INSERT INTO player (account_id, name, job, level, exp, gold, hp, max_hp, sp, max_sp,
                    attack, defense, magic_attack, magic_defense,
                    pos_x, pos_y, pos_z, pvp_kills, pvp_deaths, create_time) VALUES
(1, 'TestWarrior', 0, 10, 5000, 10000, 2000, 2000, 200, 200,
 100, 60, 40, 40, 957200, 244900, 0, 5, 2, NOW());

-- ============================================================================
-- Stored Procedures ve Functions
-- ============================================================================

-- PvP Kill kaydı için stored procedure
DELIMITER //
DROP PROCEDURE IF EXISTS sp_record_pvp_kill//
CREATE PROCEDURE sp_record_pvp_kill(
    IN p_killer_id INT UNSIGNED,
    IN p_victim_id INT UNSIGNED,
    IN p_pos_x BIGINT,
    IN p_pos_y BIGINT
)
BEGIN
    DECLARE v_killer_level INT UNSIGNED;
    DECLARE v_victim_level INT UNSIGNED;

    -- Seviyeleri al
    SELECT level INTO v_killer_level FROM player WHERE id = p_killer_id;
    SELECT level INTO v_victim_level FROM player WHERE id = p_victim_id;

    -- Log kaydı ekle
    INSERT INTO pvp_log (killer_id, victim_id, killer_level, victim_level, pos_x, pos_y, kill_time)
    VALUES (p_killer_id, p_victim_id, v_killer_level, v_victim_level, p_pos_x, p_pos_y, NOW());

    -- Killer istatistiklerini güncelle
    UPDATE player SET pvp_kills = pvp_kills + 1 WHERE id = p_killer_id;

    -- Victim istatistiklerini güncelle
    UPDATE player SET pvp_deaths = pvp_deaths + 1 WHERE id = p_victim_id;
END//

-- Seviye sıralaması güncelleme
DROP PROCEDURE IF EXISTS sp_update_level_rankings//
CREATE PROCEDURE sp_update_level_rankings()
BEGIN
    DELETE FROM rankings WHERE rank_type = 'level';

    INSERT INTO rankings (player_id, rank_type, rank_position, rank_value, update_time)
    SELECT id, 'level',
           ROW_NUMBER() OVER (ORDER BY level DESC, exp DESC),
           level,
           NOW()
    FROM player
    ORDER BY level DESC, exp DESC
    LIMIT 100;
END//

-- PvP sıralaması güncelleme
DROP PROCEDURE IF EXISTS sp_update_pvp_rankings//
CREATE PROCEDURE sp_update_pvp_rankings()
BEGIN
    DELETE FROM rankings WHERE rank_type = 'pvp_kills';

    INSERT INTO rankings (player_id, rank_type, rank_position, rank_value, update_time)
    SELECT id, 'pvp_kills',
           ROW_NUMBER() OVER (ORDER BY pvp_kills DESC),
           pvp_kills,
           NOW()
    FROM player
    ORDER BY pvp_kills DESC
    LIMIT 100;
END//

DELIMITER ;

-- İlk sıralamaları oluştur
CALL sp_update_level_rankings();
CALL sp_update_pvp_rankings();

-- ============================================================================
-- Views (Görünümler)
-- ============================================================================

-- Top 100 Level Sıralaması
DROP VIEW IF EXISTS v_top_level;
CREATE VIEW v_top_level AS
SELECT
    r.rank_position,
    p.id,
    p.name,
    p.job,
    p.level,
    p.exp
FROM rankings r
JOIN player p ON r.player_id = p.id
WHERE r.rank_type = 'level'
ORDER BY r.rank_position ASC;

-- Top 100 PvP Sıralaması
DROP VIEW IF EXISTS v_top_pvp;
CREATE VIEW v_top_pvp AS
SELECT
    r.rank_position,
    p.id,
    p.name,
    p.job,
    p.level,
    p.pvp_kills,
    p.pvp_deaths,
    ROUND(p.pvp_kills / GREATEST(p.pvp_deaths, 1), 2) as kd_ratio
FROM rankings r
JOIN player p ON r.player_id = p.id
WHERE r.rank_type = 'pvp_kills'
ORDER BY r.rank_position ASC;

-- Aktif oyuncular (son 7 gün)
DROP VIEW IF EXISTS v_active_players;
CREATE VIEW v_active_players AS
SELECT
    a.login,
    p.id,
    p.name,
    p.level,
    p.last_play,
    TIMESTAMPDIFF(HOUR, p.last_play, NOW()) as hours_since_last_play
FROM player p
JOIN account a ON p.account_id = a.id
WHERE p.last_play >= DATE_SUB(NOW(), INTERVAL 7 DAY)
ORDER BY p.last_play DESC;

-- ============================================================================
-- GRANT İzinleri (Gerekirse)
-- ============================================================================
-- CREATE USER IF NOT EXISTS 'metin2user'@'localhost' IDENTIFIED BY 'metin2pass';
-- GRANT ALL PRIVILEGES ON metin2.* TO 'metin2user'@'localhost';
-- FLUSH PRIVILEGES;
