#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Копирует SSH ключ на удалённый сервер (аналог ssh-copy-id для Windows)

.DESCRIPTION
    Копирует публичный SSH ключ на удалённый сервер и добавляет его в authorized_keys
    для аутентификации без пароля.

.EXAMPLE
    ssh-copy-id.ps1 user@host
.EXAMPLE
    ssh-copy-id.ps1 -user root -host 192.168.1.100 -port 2222
.EXAMPLE
    ssh-copy-id.ps1 user@host -i ~/.ssh/id_ed25519.pub -f
#>

[CmdletBinding(DefaultParameterSetName = 'Target')]
param(
    [Parameter(ParameterSetName = 'Target', Position = 0, ValueFromRemainingArguments = $true)]
    [string]$Target,

    [Parameter(ParameterSetName = 'Advanced')]
    [string]$user,

    [Parameter(ParameterSetName = 'Advanced')]
    [string]$host,

    [Alias('p')]
    [Parameter(ParameterSetName = 'Target')]
    [Parameter(ParameterSetName = 'Advanced')]
    [int]$port = 0,

    [Alias('i')]
    [Parameter(ParameterSetName = 'Target')]
    [Parameter(ParameterSetName = 'Advanced')]
    [string]$identity_file,

    [Parameter(ParameterSetName = 'Target')]
    [Parameter(ParameterSetName = 'Advanced')]
    [string]$key_file,

    [Alias('f')]
    [switch]$force,

    [Alias('n')]
    [switch]$dry_run,

    [Alias('q')]
    [switch]$quiet,

    [Alias('h')]
    [switch]$help,

    [Alias('o')]
    [string[]]$ssh_options,

    [Alias('F')]
    [string]$ssh_config
)

# Функция для вывода сообщений
function Write-Log {
    param([string]$Message, [string]$Level = 'INFO')
    if ($quiet -and $Level -ne 'ERROR') { return }
    
    $timestamp = Get-Date -Format "HH:mm:ss"
    $color = switch ($Level) {
        'ERROR' { 'Red' }
        'WARN'  { 'Yellow' }
        'INFO'  { 'Green' }
        default { 'White' }
    }
    Write-Host "[$timestamp] [$Level] $Message" -ForegroundColor $color
}

# Функция для отображения справки
function Show-Help {
    $helpText = @"
Использование: ssh-copy-id.ps1 [опции] [пользователь@]хост

Опции:
  -i, --identity_file <файл>    Путь к публичному ключу (по умолчанию: ~/.ssh/id_rsa.pub)
  -p, --port <порт>             Порт SSH (по умолчанию: 22)
  -f, --force                   Не спрашивать подтверждение
  -n, --dry_run                 Показать, что будет сделано, но не выполнять
  -q, --quiet                   Тихий режим
  -o, --ssh_options "<опции>"   Дополнительные SSH опции
  -F, --ssh_config <файл>       Файл конфигурации SSH
  -h, --help                    Показать эту справку

Примеры:
  ssh-copy-id.ps1 user@example.com
  ssh-copy-id.ps1 -i ~/.ssh/id_ed25519.pub user@192.168.1.100
  ssh-copy-id.ps1 -p 2222 root@server.local -f
"@
    Write-Host $helpText
}

# Проверка наличия SSH
function Test-SSH {
    $sshCmd = Get-Command ssh -ErrorAction SilentlyContinue
    if (-not $sshCmd) {
        Write-Log "SSH клиент не найден. Установите OpenSSH." -Level ERROR
        return $false
    }
    return $true
}

# Парсинг целевой строки user@host
function Parse-Target {
    param([string]$Target)
    
    if ($Target -match '^([^@]+)@(.+)$') {
        return @{
            User = $matches[1]
            Host = $matches[2]
        }
    } else {
        $currentUser = $env:USERNAME
        return @{
            User = $currentUser
            Host = $Target
        }
    }
}

# Получение пути к публичному ключу
function Get-PublicKeyPath {
    param(
        [string]$IdentityFile,
        [string]$KeyFile
    )
    
    if ($KeyFile) {
        $keyPath = $KeyFile
    } elseif ($IdentityFile) {
        if ($IdentityFile -match '\.pub$') {
            $keyPath = $IdentityFile
        } else {
            $keyPath = $IdentityFile + '.pub'
        }
    } else {
        $keyPath = Join-Path $env:USERPROFILE '.ssh\id_rsa.pub'
    }
    
    $keyPath = $keyPath.Replace('~', $env:USERPROFILE)
    $keyPath = [Environment]::ExpandEnvironmentVariables($keyPath)
    
    return $keyPath
}

# Проверка справки
if ($help) {
    Show-Help
    exit 0
}

# Проверка SSH
if (-not (Test-SSH)) {
    exit 1
}

# Парсинг целевого хоста
if ($Target -and -not $host) {
    $parsed = Parse-Target -Target $Target
    if (-not $user) { $user = $parsed.User }
    if (-not $host) { $host = $parsed.Host }
}

if (-not $host) {
    Write-Log "Не указан хост. Используйте: ssh-copy-id.ps1 user@host" -Level ERROR
    Show-Help
    exit 1
}

# Получение пути к ключу
$publicKeyPath = Get-PublicKeyPath -IdentityFile $identity_file -KeyFile $key_file

if (-not (Test-Path $publicKeyPath)) {
    Write-Log "Публичный ключ не найден: $publicKeyPath" -Level ERROR
    
    $privateKeyPath = $publicKeyPath -replace '\.pub$', ''
    if (Test-Path $privateKeyPath) {
        Write-Log "Найден приватный ключ: $privateKeyPath" -Level WARN
        Write-Log "Создайте публичный ключ: ssh-keygen -y -f $privateKeyPath > $publicKeyPath" -Level WARN
    }
    exit 1
}

# Чтение публичного ключа
$publicKey = Get-Content $publicKeyPath -Raw
if ([string]::IsNullOrWhiteSpace($publicKey)) {
    Write-Log "Файл ключа пуст: $publicKeyPath" -Level ERROR
    exit 1
}

# Построение SSH команды
$sshArgs = @()

if ($port -gt 0) {
    $sshArgs += '-p', $port.ToString()
}

if ($ssh_config) {
    $sshArgs += '-F', $ssh_config
}

if ($ssh_options) {
    foreach ($opt in $ssh_options) {
        $sshArgs += '-o', $opt
    }
}

# Формирование команды для проверки подключения
$testSshArgs = $sshArgs + @($user + '@' + $host, 'exit 0')

Write-Log "Копирование ключа: $publicKeyPath"
$portStr = ""
if ($port -gt 0) { $portStr = ":$port" }
Write-Log "На сервер: $user@$host$portStr"

if ($dry_run) {
    Write-Log "[DRY RUN] Ключ будет добавлен в ~/.ssh/authorized_keys"
    Write-Log "[DRY RUN] SSH команда: ssh $($testSshArgs -join ' ')"
    exit 0
}

# Создание удалённой директории .ssh
Write-Log "Проверка подключения..."
$null = ssh $testSshArgs 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Log "Не удалось подключиться к серверу. Проверьте логин/пароль." -Level ERROR
    exit 1
}

# Создание директории .ssh
Write-Log "Создание директории ~/.ssh..."
$mkdirCmd = "mkdir -p ~/.ssh"
$chmodCmd = "chmod 700 ~/.ssh"
$mkdirArgs = $sshArgs + @($user + '@' + $host, $mkdirCmd + " `&`& " + $chmodCmd)
$null = ssh $mkdirArgs 2>&1

# Добавление ключа в authorized_keys
Write-Log "Добавление ключа в authorized_keys..."

# Экранирование ключа для shell
$escapedKey = $publicKey.Trim() -replace "'", "'\''"

if ($force) {
    # Принудительное добавление без проверки
    $echoCmd = "echo '$escapedKey' >> ~/.ssh/authorized_keys"
    $chmodCmd = "chmod 600 ~/.ssh/authorized_keys"
    $addCmd = $echoCmd + " `&`& " + $chmodCmd
    $addKeyArgs = $sshArgs + @($user + '@' + $host, $addCmd)
    $null = ssh $addKeyArgs 2>&1
} else {
    # Проверка наличия ключа
    $checkCmd = 'cat ~/.ssh/authorized_keys'
    $checkArgs = $sshArgs + @($user + '@' + $host, $checkCmd)
    $existingKeys = ssh $checkArgs 2>&1 | Out-String
    
    $keyTrimmed = $publicKey.Trim()
    $pattern = [regex]::Escape($keyTrimmed)
    if ($existingKeys -match $pattern) {
        Write-Log "Ключ уже присутствует на сервере" -Level WARN
        exit 0
    }
    
    # Добавление ключа
    $echoCmd = "echo '$escapedKey' >> ~/.ssh/authorized_keys"
    $chmodCmd = "chmod 600 ~/.ssh/authorized_keys"
    $addCmd = $echoCmd + " `&`& " + $chmodCmd
    $addKeyArgs = $sshArgs + @($user + '@' + $host, $addCmd)
    $null = ssh $addKeyArgs 2>&1
}

# Проверка результата
$copyResult = $LASTEXITCODE

if ($copyResult -eq 0) {
    Write-Log "Ключ успешно скопирован!" -Level INFO

    # Тестовое подключение
    if (-not $quiet) {
        Write-Log "Проверка подключения с ключом..."
        $privateKeyPath = $publicKeyPath -replace '\.pub$', ''
        $testArgs = $sshArgs + @('-i', $privateKeyPath, $user + '@' + $host, 'exit 0')
        ssh $testArgs > $null 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Log "Подключение с ключом работает!" -Level INFO
        }
        else {
            Write-Log "Подключение с ключом не удалось. Возможно, требуется перезапуск SSH агента." -Level WARN
        }
    }
}
else {
    Write-Log "Ошибка при копировании ключа" -Level ERROR
    exit 1
}
