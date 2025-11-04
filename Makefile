# Metin2 PvP Server Makefile
# ===========================

# Compiler
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -I./include

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Libraries
MYSQL_CFLAGS = $(shell mysql_config --cflags 2>/dev/null || echo "-I/usr/include/mysql")
MYSQL_LIBS = $(shell mysql_config --libs 2>/dev/null || echo "-lmysqlclient")
LUA_CFLAGS = $(shell pkg-config --cflags lua5.3 2>/dev/null || pkg-config --cflags lua 2>/dev/null || echo "-I/usr/include/lua5.3")
LUA_LIBS = $(shell pkg-config --libs lua5.3 2>/dev/null || pkg-config --libs lua 2>/dev/null || echo "-llua5.3")

# Combined flags
CXXFLAGS += $(MYSQL_CFLAGS) $(LUA_CFLAGS)
LIBS = $(MYSQL_LIBS) $(LUA_LIBS) -lpthread -lm

# Source files
DB_SOURCES = $(SRC_DIR)/db/DBManager.cpp
GAME_SOURCES = $(SRC_DIR)/game/Character.cpp \
               $(SRC_DIR)/game/GameServer.cpp \
               $(SRC_DIR)/game/QuestManager.cpp \
               $(SRC_DIR)/game/PVPManager.cpp \
               $(SRC_DIR)/game/main.cpp

ALL_SOURCES = $(DB_SOURCES) $(GAME_SOURCES)

# Object files
DB_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(DB_SOURCES))
GAME_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(GAME_SOURCES))
ALL_OBJECTS = $(DB_OBJECTS) $(GAME_OBJECTS)

# Target executable
TARGET = $(BIN_DIR)/game_server

# Colors for output
COLOR_RESET = \033[0m
COLOR_GREEN = \033[32m
COLOR_YELLOW = \033[33m
COLOR_BLUE = \033[34m

# Default target
.PHONY: all
all: $(TARGET)
	@echo "$(COLOR_GREEN)Build complete! Executable: $(TARGET)$(COLOR_RESET)"

# Create directories
$(BUILD_DIR) $(BIN_DIR):
	@mkdir -p $@

$(BUILD_DIR)/db $(BUILD_DIR)/game: | $(BUILD_DIR)
	@mkdir -p $@

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)/db $(BUILD_DIR)/game
	@echo "$(COLOR_BLUE)Compiling $<...$(COLOR_RESET)"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Link executable
$(TARGET): $(ALL_OBJECTS) | $(BIN_DIR)
	@echo "$(COLOR_YELLOW)Linking $(TARGET)...$(COLOR_RESET)"
	@$(CXX) $(ALL_OBJECTS) $(LIBS) -o $@

# Clean build files
.PHONY: clean
clean:
	@echo "$(COLOR_YELLOW)Cleaning build files...$(COLOR_RESET)"
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "$(COLOR_GREEN)Clean complete!$(COLOR_RESET)"

# Run the server
.PHONY: run
run: $(TARGET)
	@echo "$(COLOR_GREEN)Starting server...$(COLOR_RESET)"
	@./$(TARGET)

# Show help
.PHONY: help
help:
	@echo "Metin2 PvP Server Build System"
	@echo "=============================="
	@echo ""
	@echo "Available targets:"
	@echo "  make          - Build the server"
	@echo "  make clean    - Remove build files"
	@echo "  make run      - Build and run the server"
	@echo "  make install  - Install required dependencies"
	@echo "  make help     - Show this help message"
	@echo ""

# Install dependencies (Ubuntu/Debian)
.PHONY: install
install:
	@echo "$(COLOR_YELLOW)Installing dependencies...$(COLOR_RESET)"
	@echo "This requires sudo privileges."
	@sudo apt-get update
	@sudo apt-get install -y \
		build-essential \
		cmake \
		libmysqlclient-dev \
		liblua5.3-dev \
		pkg-config
	@echo "$(COLOR_GREEN)Dependencies installed!$(COLOR_RESET)"

# Database setup
.PHONY: db-setup
db-setup:
	@echo "$(COLOR_YELLOW)Setting up database...$(COLOR_RESET)"
	@echo "Please make sure MySQL/MariaDB is running."
	@mysql -u root -p < sql/schema.sql
	@echo "$(COLOR_GREEN)Database setup complete!$(COLOR_RESET)"

# Show build configuration
.PHONY: config
config:
	@echo "Build Configuration:"
	@echo "==================="
	@echo "Compiler: $(CXX)"
	@echo "C++ Flags: $(CXXFLAGS)"
	@echo "Libraries: $(LIBS)"
	@echo "Target: $(TARGET)"
	@echo ""
