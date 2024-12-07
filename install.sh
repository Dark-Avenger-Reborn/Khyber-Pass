#!/bin/bash

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Detect package manager
_OSTYPE_detect() {
  _found_arch PACMAN "Arch Linux" && return
  _found_arch DPKG   "Debian GNU/Linux" && return
  _found_arch DPKG   "Ubuntu" && return
  _found_arch YUM    "CentOS" && return
  _found_arch YUM    "Red Hat" && return
  _found_arch YUM    "Fedora" && return
  _found_arch ZYPPER "SUSE" && return

  [[ -z "$_OSTYPE" ]] || return

  if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "Can't detect OS type from /etc/issue. Running fallback method."
  fi
  [[ -x "/usr/bin/pacman" ]]           && _OSTYPE="PACMAN" && return
  [[ -x "/usr/bin/apt-get" ]]          && _OSTYPE="DPKG" && return
  [[ -x "/usr/bin/yum" ]]              && _OSTYPE="YUM" && return
  [[ -x "/opt/local/bin/port" ]]       && _OSTYPE="MACPORTS" && return
  command -v brew >/dev/null           && _OSTYPE="HOMEBREW" && return
  [[ -x "/usr/bin/emerge" ]]           && _OSTYPE="PORTAGE" && return
  [[ -x "/usr/bin/zypper" ]]           && _OSTYPE="ZYPPER" && return
  if [[ -z "$_OSTYPE" ]]; then
    echo "No supported package manager installed on system"
    echo "(supported: apt, homebrew, pacman, portage, yum, zypper)"
    exit 1
  fi
}

# Install necessary packages based on package manager
install_package() {
  local package=$1
  case $_OSTYPE in
    PACMAN)
      sudo pacman -S --noconfirm $package
      ;;
    DPKG)
      sudo apt-get update
      sudo apt-get install -y $package
      ;;
    YUM)
      sudo yum install -y $package
      ;;
    ZYPPER)
      sudo zypper install -y $package
      ;;
    MACPORTS)
      sudo port install $package
      ;;
    HOMEBREW)
      brew install $package
      ;;
    PORTAGE)
      sudo emerge $package
      ;;
    *)
      echo "Unsupported package manager"
      exit 1
      ;;
  esac
}

# Ensure required dependencies are installed
install_dependencies() {
  echo "Installing dependencies..."

  # Install libcurl (for interacting with webhooks)
  install_package "libcurl"

  # Install a C compiler (GCC or Clang)
  if command_exists gcc; then
    echo "GCC already installed."
  else
    echo "Installing GCC..."
    install_package "gcc"
  fi

  # Install curl (if not installed)
  if command_exists curl; then
    echo "Curl already installed."
  else
    echo "Installing curl..."
    install_package "curl"
  fi

  echo "Dependencies installed successfully."
}

# Run installation process
_OSTYPE_detect
install_dependencies
