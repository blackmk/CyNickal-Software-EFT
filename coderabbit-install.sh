#!/usr/bin/env sh

# CodeRabbit CLI Installation Script
#
# This script downloads and installs the CodeRabbit CLI to ~/.local/bin
# It automatically detects your platform (OS/architecture) and downloads the appropriate binary.
#
# USAGE:
#   # Install latest version
#   curl -fsSL https://cli.coderabbit.ai/install.sh | sh
#
#   # Install specific version
#   CODERABBIT_VERSION=v1.2.3 curl -fsSL https://cli.coderabbit.ai/install.sh | sh
#
#   # Use custom download URL (for development/testing)
#   CODERABBIT_DOWNLOAD_URL=https://localhost:8080/releases curl -fsSL install.sh | sh
#
#   # Install to custom directory
#   CODERABBIT_INSTALL_DIR=/usr/local/bin curl -fsSL install.sh | sh
#
# ENVIRONMENT VARIABLES:
#   CODERABBIT_VERSION      - Override version to install (e.g., "v1.2.3")
#   CODERABBIT_DOWNLOAD_URL - Override base download URL (default: https://cli.coderabbit.ai/releases)
#   CODERABBIT_INSTALL_DIR  - Override install directory (default: ~/.local/bin)
#
# SUPPORTED PLATFORMS:
#   - Linux x64, ARM64
#   - macOS x64, ARM64 (Apple Silicon)
#
# INSTALLATION LOCATION:
#   - Binary: ~/.local/bin/coderabbit
#   - Alias: ~/.local/bin/cr
#   - Adds ~/.local/bin to PATH if needed
#
# REQUIREMENTS:
#   - curl or wget (for downloading)
#   - unzip (for extracting)
#   - Standard POSIX shell

# Show CodeRabbit logo
show_logo() {
    # Define colors
    if [ -z "$NO_COLOR" ]; then
        orange=$(printf '\033[38;5;208m')
        reset=$(printf '\033[0m')
    else
        orange=''
        reset=''
    fi

    echo "" >&2
    echo "${orange}" >&2
    echo "            ____          _      ____       _     _     _ _   " >&2
    echo "  (\_/)    / ___|___   __| | ___|  _ \ __ _| |__ | |__ (_) |_ " >&2
    echo "  ( •_•)  | |   / _ \ / _\` |/ _ \ |_) / _\` | '_ \| '_ \| | __|" >&2
    echo "   />[_]  | |__| (_) | (_| |  __/  _ < (_| | |_) | |_) | | |_ " >&2
    echo "           \____\___/ \__,_|\___|_| \_\__,_|_.__/|_.__/|_|\__|" >&2
    echo "${reset}" >&2
    echo "" >&2
    echo "" >&2
}

# Print colored output
print_status() {
    echo "[INFO] $1" >&2
}

print_success() {
    echo "[SUCCESS] $1" >&2
}

print_warning() {
    echo "[WARNING] $1" >&2
}

print_error() {
    if [ -z "$NO_COLOR" ]; then
        red=$(printf '\033[0;31m')
        reset=$(printf '\033[0m')
        echo "${red}[ERROR] $1${reset}" >&2
    else
        echo "[ERROR] $1" >&2
    fi
}

# Detect OS and architecture
detect_platform() {
    os=$(uname -s | tr '[:upper:]' '[:lower:]')
    arch=$(uname -m)

    case "$os" in
        darwin)
            OS="darwin"
            ;;
        linux)
            OS="linux"
            ;;
        *)
            print_error "Unsupported operating system: $os"
            exit 1
            ;;
    esac

    case "$arch" in
        x86_64|amd64)
            ARCH="x64"
            ;;
        arm64|aarch64)
            ARCH="arm64"
            ;;
        *)
            print_error "Unsupported architecture: $arch"
            exit 1
            ;;
    esac

    print_status "Detected platform: $OS-$ARCH"
}

# Download a file using curl or wget
download_file() {
    url="$1"
    output="$2"

    # Debug: show what we're trying to download
    # echo "DEBUG: URL='$url', Output='$output'" >&2

    if command -v curl >/dev/null 2>&1; then
        if [ -n "$output" ]; then
            curl -fsSL "$url" -o "$output"
        else
            curl -fsSL "$url"
        fi
    elif command -v wget >/dev/null 2>&1; then
        if [ -n "$output" ]; then
            wget -q "$url" -O "$output"
        else
            wget -q "$url" -O -
        fi
    else
        print_error "Neither curl nor wget is available. Please install one of them."
        return 1
    fi
}

# Create install directory if it doesn't exist
create_install_dir() {
    bin_dir="${CODERABBIT_INSTALL_DIR:-$HOME/.local/bin}"

    # Expand tilde if present
    case "$bin_dir" in
      "~")        bin_dir="$HOME" ;;
      "~/"*)      bin_dir="$HOME/${bin_dir#~/}" ;;
      *)          : ;;
    esac

    if [ ! -d "$bin_dir" ]; then
        print_status "Creating $bin_dir directory..."
        mkdir -p "$bin_dir"
    fi
    BIN_DIR="$bin_dir"
    print_status "Install directory: $BIN_DIR"
}

# Get version to install
get_version() {
    base_url="$1"

    # Check if version is overridden by environment variable
    if [ -n "$CODERABBIT_VERSION" ]; then
        print_status "Using version from environment: $CODERABBIT_VERSION"
        echo "$CODERABBIT_VERSION"
        return 0
    fi

    # Get the latest version from VERSION file
    version_url="${base_url}/latest/VERSION"
    version=""

    print_status "Fetching latest version from: $version_url"

    version=$(download_file "$version_url") || exit 1

    if [ -z "$version" ]; then
        print_error "Failed to fetch version information"
        exit 1
    fi

    # Trim whitespace
    version=$(echo "$version" | tr -d '[:space:]')

    print_status "Latest version: $version"
    echo "$version"
}

# Download and install the CLI
install_cli() {
    # Allow override with environment variable for local development
    base_url="https://cli.coderabbit.ai/releases"
    version=$(get_version "$base_url")
    download_url="${base_url}/${version}/coderabbit-${OS}-${ARCH}.zip"
    install_path="$BIN_DIR/coderabbit"

    # Create secure temporary directory with restrictive permissions
    temp_dir=$(mktemp -d -t coderabbit-install.XXXXXX)
    chmod 700 "$temp_dir"
    temp_file="$temp_dir/coderabbit-${OS}-${ARCH}.zip"

    # Set up cleanup trap for temporary directory
    trap 'rm -rf "$temp_dir"' EXIT INT TERM

    print_status "Downloading CodeRabbit CLI from $download_url..."

    if ! download_file "$download_url" "$temp_file"; then
        exit 1
    fi

    if [ ! -f "$temp_file" ]; then
        print_error "Failed to download CLI archive"
        exit 1
    fi

    print_status "Extracting CLI binary..."
    if command -v unzip >/dev/null 2>&1; then
        unzip -q "$temp_file" -d "$temp_dir"
    else
        print_error "unzip is required but not available. Please install it."
        exit 1
    fi

    # Find the binary in the extracted files
    binary_path="$temp_dir/coderabbit"
    if [ ! -f "$binary_path" ]; then
        print_error "Could not find coderabbit binary in downloaded archive"
        exit 1
    fi

    print_status "Installing CLI to $install_path..."
    mv "$binary_path" "$install_path"
    chmod +x "$install_path"

    # Cleanup handled by trap

    # Create symlink for 'cr' command
    ln -sf "$install_path" "$BIN_DIR/cr"

    print_success "CodeRabbit CLI installed successfully!"
}

# Check if install directory is in PATH
check_path() {
    case ":$PATH:" in
        *":$BIN_DIR:"*) return 0 ;;
        *)              return 1 ;;
    esac
}

# Add install directory to PATH in shell profile
setup_path() {
    shell_profile=""
    shell_name=$(basename "$SHELL")
    path_export="export PATH=\"$BIN_DIR:\$PATH\""

    case "$shell_name" in
        bash)
            if [ -f "$HOME/.bash_profile" ]; then
                shell_profile="$HOME/.bash_profile"
            elif [ -f "$HOME/.bashrc" ]; then
                shell_profile="$HOME/.bashrc"
            else
                shell_profile="$HOME/.bash_profile"
            fi
            ;;
        zsh)
            shell_profile="$HOME/.zshrc"
            ;;
        fish)
            # Fish shell uses a different syntax
            if [ ! -d "$HOME/.config/fish" ]; then
                mkdir -p "$HOME/.config/fish"
            fi
            echo "set -gx PATH $BIN_DIR \$PATH" >> "$HOME/.config/fish/config.fish"
            print_success "Added $BIN_DIR to PATH in Fish shell configuration"
            return 0
            ;;
        *)
            shell_profile="$HOME/.profile"
            ;;
    esac

    if [ ! -f "$shell_profile" ]; then
        touch "$shell_profile"
    fi

    # Check if PATH export already exists for this directory
    if ! grep -qF "$path_export" "$shell_profile" 2>/dev/null; then
        echo '' >> "$shell_profile"
        echo '# Added by CodeRabbit CLI installer' >> "$shell_profile"
        echo "$path_export" >> "$shell_profile"
        print_success "Added $BIN_DIR to PATH in $shell_profile"
        print_warning "Please restart your shell or run: source $shell_profile"
    else
        print_status "PATH already configured in $shell_profile"
    fi
}

# Verify installation
verify_installation() {
    if check_path; then
        if command -v coderabbit >/dev/null 2>&1; then
            print_success "Installation verified! CodeRabbit CLI is ready to use."
            print_status "Try running: coderabbit --help"
            print_status "Or use the short alias: cr --help"
        else
            print_warning "CLI installed but not found in PATH. Please restart your shell."
        fi
    else
        print_warning "$BIN_DIR is not in your PATH. Please restart your shell or add it manually."
    fi
}

# Check for required external tools
check_required_tools() {
    missing_tools=""

    # Check for unzip (required for extracting CLI archive)
    if ! command -v unzip >/dev/null 2>&1; then
        missing_tools="${missing_tools} unzip"
    fi

    # Check for git (required for CodeRabbit CLI functionality)
    if ! command -v git >/dev/null 2>&1; then
        missing_tools="${missing_tools} git"
    fi

    # Trim leading whitespace
    missing_tools=$(echo "$missing_tools" | sed 's/^ *//')

    if [ -z "$missing_tools" ]; then
        print_success "All required tools are available"
        return 0
    fi

    error_msg="Missing required tools:"
    for tool in $missing_tools; do
        error_msg="$error_msg\n  - $tool"
    done
    error_msg="$error_msg\nPlease install the missing tools before proceeding"
    print_error "$error_msg"
    exit 1
}

# Main installation process
main() {
    show_logo
    print_status "Starting CodeRabbit CLI installation..."

    detect_platform
    check_required_tools
    create_install_dir
    install_cli

    if ! check_path; then
        print_status "Setting up PATH configuration..."
        setup_path
    else
        print_status "PATH is already configured correctly"
    fi

    verify_installation

    print_success "Installation complete!"
    echo
    print_status "Next steps:"
    echo "  1. Restart your shell or run: source ~/.$(basename $SHELL)rc"
    echo "  2. Run 'coderabbit auth login' to authenticate"
    echo
    echo "===== Try CodeRabbit CLI with AI Coding Agents ====="
    echo
    echo "How it works:"
    echo "1. Give your AI assistant a coding task that includes reviewing it using CodeRabbit"
    echo "2. The AI writes code, runs \`coderabbit review --plain\`, and applies suggestions"
    echo "3. This creates an automated code improvement loop"
    echo
    echo "Example prompt to try:"
    echo
    echo "Implement a JSON parser in Go without using the standard library. Run \`coderabbit review --plain\` to get comprehensive code analysis and improvement suggestions. Apply the feedback to write cleaner, more maintainable code."
    echo
    echo "Review Modes:"
    echo "   --plain            : Detailed feedback with fix suggestions"
    echo "   --prompt-only      : Minimal output for token efficiency"
    echo
    echo "Quick Commands:"
    echo "  coderabbit --plain         # Detailed analysis"
    echo "  coderabbit --prompt-only   # Token-efficient"
    echo "  cr --help                  # Show all options"
    echo
    echo "Note: Run commands from your git repository root directory"
}

# Run main function
main "$@"
