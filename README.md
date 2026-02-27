# ssh-copy-id for Windows

Native Windows port of the Linux `ssh-copy-id` utility written in C.

## Description

Copies your public SSH key to a remote server and adds it to `~/.ssh/authorized_keys` for passwordless authentication.

## Requirements

- Windows 10/11 or Windows Server 2016+
- OpenSSH Client (built-in on Windows 10 1809+)

## Installation

### Quick Start

1. Download `ssh-copy-id.exe` to any directory
2. Add the directory to your `PATH` or use the full path

### Check OpenSSH

```cmd
where ssh
```

If not found, install it:
```cmd
# Windows 10/11 (via Settings)
# Settings → Apps → Optional features → Add a feature → OpenSSH Client
```

### Compile from Source

If you have GCC (MinGW) installed:

```cmd
gcc -O2 -Wall -Wextra -o ssh-copy-id.exe ssh-copy-id.c -lws2_32
```

Or use the build script:
```cmd
build.bat
```

## Usage

### Basic Syntax

```cmd
ssh-copy-id.exe [options] [user@]host
```

### Options

| Option | Description |
|--------|-------------|
| `-i <file>` | Path to public key (default: `~/.ssh/id_rsa.pub`) |
| `-p <port>` | SSH port (default: 22) |
| `-f` | Don't check for existing keys, force add |
| `-n` | Dry-run: show what would be done |
| `-q` | Quiet mode |
| `-o "<options>"` | Additional SSH options |
| `-F <file>` | SSH configuration file |
| `-h` | Show help |

## Examples

### Copy default key

```cmd
ssh-copy-id.exe user@example.com
```

### With custom port

```cmd
ssh-copy-id.exe -p 2222 user@192.168.1.100
```

### With custom key file

```cmd
ssh-copy-id.exe -i C:\Users\%USERNAME%\.ssh\id_ed25519.pub user@server.local
```

### Force copy (no check)

```cmd
ssh-copy-id.exe -f root@10.0.0.1
```

### With SSH options

```cmd
ssh-copy-id.exe -o "StrictHostKeyChecking=no" user@host
```

### Dry-run

```cmd
ssh-copy-id.exe -n user@host
```

## Generate SSH Key

If you don't have an SSH key:

```cmd
ssh-keygen -t ed25519 -C "your_email@example.com"
```

Or RSA:

```cmd
ssh-keygen -t rsa -b 4096 -C "your_email@example.com"
```

## Verify

After copying, test the connection:

```cmd
ssh user@host
```

Should connect without password prompt.

## Project Structure

```
copy-id/
├── ssh-copy-id.exe      # Compiled executable (recommended)
├── ssh-copy-id.c        # C source code
├── ssh-copy-id.ps1      # PowerShell version (alternative)
├── ssh-copy-id.cmd      # PowerShell wrapper
├── build.bat            # Build script
├── Makefile             # Makefile for GCC/MSVC
└── README.md            # Documentation
```

## Features

- Native Windows executable (no PowerShell required)
- All major options from original Linux utility
- Windows path support (`C:\Users\...` or `~`)
- Minimal dependencies (only OpenSSH client)
- No encoding issues (English messages)

## Troubleshooting

### "SSH client not found"

Install OpenSSH Client via Windows Settings or PowerShell:

```powershell
Add-WindowsCapability -Online -Name OpenSSH.Client~~~~0.0.1.0
```

### "Public key not found"

1. Check if key exists: `dir %USERPROFILE%\.ssh\id_rsa.pub`
2. If only private key exists, generate public key:
   ```cmd
   ssh-keygen -y -f %USERPROFILE%\.ssh\id_rsa > %USERPROFILE%\.ssh\id_rsa.pub
   ```

### "Failed to connect to server"

- Check server availability: `ping host`
- Check port: `telnet host port`
- Verify login credentials

## License

MIT License
