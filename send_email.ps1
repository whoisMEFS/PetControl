param(
    [string]$email,
    [string]$nome,
    [string]$vencimento
)

# Carregar credenciais
$cfgPath = "credentials.cfg"
if (!(Test-Path $cfgPath)) {
    Write-Output "ERRO: Arquivo credentials.cfg não encontrado."
    exit
}

$cfg = Get-Content $cfgPath | ForEach-Object {
    $p = $_.Split("=")
    if ($p.Length -eq 2) {
        @{ Key = $p[0].Trim(); Value = $p[1].Trim() }
    }
}

$dict = @{}
foreach ($item in $cfg) { $dict[$item.Key] = $item.Value }

$gmailUser = $dict["gmail_user"]
$gmailPass = $dict["gmail_pass"]

if (!$gmailUser -or !$gmailPass) {
    Write-Output "ERRO: gmail_user ou gmail_pass não foram definidos."
    exit
}

# ------------------------------
# CARREGAR MAILKIT (EMBARCADO)
# ------------------------------

$mailkitPath = ".\MailKit.dll"
$mimekitPath = ".\MimeKit.dll"

if (!(Test-Path $mailkitPath) -or !(Test-Path $mimekitPath)) {
    Write-Output "Baixando MailKit..."
    Invoke-WebRequest "https://raw.githubusercontent.com/jstedfast/MailKit/master/artifacts/MailKit.dll" -OutFile $mailkitPath
    Invoke-WebRequest "https://raw.githubusercontent.com/jstedfast/MimeKit/master/artifacts/MimeKit.dll" -OutFile $mimekitPath
}

Add-Type -Path $mimekitPath
Add-Type -Path $mailkitPath

# ------------------------------
# MONTAR E-MAIL UTF-8 REAL
# ------------------------------

$message = New-Object MimeKit.MimeMessage
$message.From.Add( (New-Object MimeKit.MailboxAddress "AgendaPet", $gmailUser) )
$message.To.Add( (New-Object MimeKit.MailboxAddress $nome, $email) )
$message.Subject = "Aviso: Plano prestes a vencer"

$bodyHtml = @"
<p>Olá $nome,</p>

<p>O seu plano está próximo do vencimento.</p>

<p><b>Data de vencimento:</b> $vencimento</p>

<p>Por favor, entre em contato para renovação.</p>

<p>Atenciosamente,<br>Equipe AgendaPet</p>
"@

$body = New-Object MimeKit.TextPart "html"
$body.Text = $bodyHtml
$body.ContentType.Charset = "utf-8"

$message.Body = $body

# ------------------------------
# ENVIO SMTP
# ------------------------------

try {
    $client = New-Object MailKit.Net.Smtp.SmtpClient
    $client.Connect("smtp.gmail.com", 587, [MailKit.Security.SecureSocketOptions]::StartTls)
    $client.Authenticate($gmailUser, $gmailPass)

    $client.Send($message)
    $client.Disconnect($true)

    Write-Output "OK"
}
catch {
    Write-Output "ERRO: $($_.Exception.Message)"
}
