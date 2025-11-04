#!/bin/bash

# Metin2 PvP Server Build Script
# ===============================

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Functions
print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

# Banner
echo "=================================="
echo "  Metin2 PvP Server Build Script  "
echo "=================================="
echo ""

# Check if running as root
if [ "$EUID" -eq 0 ]; then
    print_warning "Please don't run this script as root"
fi

# Check for required tools
print_info "Checking dependencies..."

MISSING_DEPS=()

if ! command -v g++ &> /dev/null; then
    MISSING_DEPS+=("g++")
fi

if ! command -v make &> /dev/null; then
    MISSING_DEPS+=("make")
fi

if ! pkg-config --exists mysqlclient 2>/dev/null && ! pkg-config --exists mariadb 2>/dev/null; then
    MISSING_DEPS+=("libmysqlclient-dev")
fi

if ! pkg-config --exists lua5.3 2>/dev/null && ! pkg-config --exists lua 2>/dev/null; then
    MISSING_DEPS+=("liblua5.3-dev")
fi

if [ ${#MISSING_DEPS[@]} -ne 0 ]; then
    print_error "Missing dependencies: ${MISSING_DEPS[*]}"
    echo ""
    print_info "Install with:"
    echo "  sudo apt-get install build-essential libmysqlclient-dev liblua5.3-dev"
    echo "Or run:"
    echo "  make install"
    exit 1
else
    print_success "All dependencies found"
fi

# Build method selection
echo ""
print_info "Select build method:"
echo "  1) Make (default)"
echo "  2) CMake"
read -p "Choice [1]: " BUILD_METHOD
BUILD_METHOD=${BUILD_METHOD:-1}

# Clean old build
if [ -d "build" ] || [ -d "bin" ]; then
    print_info "Cleaning old build files..."
    make clean &> /dev/null
    rm -rf build bin
    print_success "Clean complete"
fi

# Build
echo ""
print_info "Building server..."

if [ "$BUILD_METHOD" -eq 2 ]; then
    # CMake build
    print_info "Using CMake..."
    mkdir -p build
    cd build || exit 1

    if cmake .. && make; then
        cd ..
        print_success "Build successful with CMake!"
    else
        cd ..
        print_error "Build failed!"
        exit 1
    fi
else
    # Make build
    print_info "Using Make..."
    if make; then
        print_success "Build successful with Make!"
    else
        print_error "Build failed!"
        exit 1
    fi
fi

# Check executable
if [ -f "bin/game_server" ]; then
    print_success "Server executable created: bin/game_server"

    # Ask to run
    echo ""
    read -p "Do you want to run the server now? [y/N]: " RUN_SERVER
    if [[ $RUN_SERVER =~ ^[Yy]$ ]]; then
        echo ""
        print_info "Starting server..."
        echo ""
        ./bin/game_server
    fi
else
    print_error "Server executable not found!"
    exit 1
fi
