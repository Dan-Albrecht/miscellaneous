function prompt {
    $rightFlame = [char]::ConvertFromUtf32(0xe0c0)
    $triangleRight = [char]::ConvertFromUtf32(0xe0b0)
    $exitGlyph = [char]::ConvertFromUtf32(0xf705)
    $noGit = [char]::ConvertFromUtf32(0xf663)
    $highVoltage = [char]::ConvertFromUtf32(0x26a1)
    $origLastExitCode = $LASTEXITCODE
    $date = Get-Date -Format "[HH:mm]"
    $prompt = ""

    if (Test-Administrator) {
        $prompt += $highVoltage
    }

    $prompt += Write-Prompt -ForegroundColor Black -BackgroundColor Green -Object $triangleRight
    $prompt += Write-Prompt -ForegroundColor White -BackgroundColor Green -Object $date
    $prompt += Write-Prompt -ForegroundColor Green -BackgroundColor Blue -Object $triangleRight
    $prompt += Write-Prompt -ForegroundColor White -BackgroundColor Blue -Object "$($ExecutionContext.SessionState.Path.CurrentLocation)"
    $prompt += Write-Prompt -ForegroundColor Blue -BackgroundColor Gray -Object $triangleRight

    if ($status = Get-GitStatus -Force) {
        $temp = $GitPromptSettings.PathStatusSeparator.Text
        $GitPromptSettings.PathStatusSeparator.Text = ""
        $status = Write-GitStatus -Status $status
        $GitPromptSettings.PathStatusSeparator.Text = $temp
        $prompt += Write-Prompt -BackgroundColor Gray -Object $status
    }
    else {
        $prompt += Write-Prompt -ForegroundColor "White" -BackgroundColor "Gray" -Object ($noGit + " ")
    }

    $prompt += Write-Prompt -ForegroundColor Gray -BackgroundColor Yellow -Object $triangleRight
    $prompt += Write-Prompt -ForegroundColor Black -BackgroundColor Yellow -Object "$exitGlyph $origLastExitCode "
    $prompt += Write-Prompt -ForegroundColor Red -BackgroundColor Red -Object " "
    $prompt += Write-Prompt -ForegroundColor Red -BackgroundColor Black -Object $rightFlame
    $prompt += "`n"

    $LASTEXITCODE = $origLastExitCode
    $prompt
}

function CreateDynamicAlias() {
    param(
        [Parameter(Mandatory = $true)][string]$name,
        [Parameter(Mandatory = $true)][string]$action
    )

    $functionName = [guid]::NewGuid().ToString("N")
    $dynamicFunction = "function global:$functionName() {$action}"
    Invoke-Expression $dynamicFunction
    Set-Alias -Name $name -Value $functionName -Scope Global
    $PromptSettings.Aliases.Add("$name -> $action")
}

function Invoke-PullDefaultBranch {
    $currentSettings = [RepoSettings]::GetCurrentSettings($PromptSettings.SettingsFile)
    $pullBranch = $currentSettings.DefaultBranch

    # Progress seems to be going to stderr and screwing this up...
    # Invoke-WithErrorHandling "git" @('pull', 'origin', $pullBranch)
    git pull origin $pullBranch
}

function Invoke-WithErrorHandling {
    param (
        [Parameter(Mandatory = $true)][string]$execString,
        [Parameter(Mandatory = $false)][string[]]$args
    )

    $output = & $execString $args 2>&1
    $lastExit = $LASTEXITCODE

    if ($lastExit -ne 0) {
        Write-Error "Command $execString $args exited with code $lastExit and output of $output"
    }

    return $output
}

function Get-AliasEx {
    Get-Alias
    Write-Host
    Write-Host "Cool aliases"
    Write-Host "============"
    foreach ($alias in $PromptSettings.Aliases) {
        Write-Host $alias
    }
}

function Write-NonTerminatingError {
    param (
        [Parameter(Mandatory = $true)][string]$message
    )

    Write-Host -ForegroundColor Red -Object $message
}

function Write-TerminatingError {
    param (
        [Parameter(Mandatory = $true)][string]$message
    )

    # Don't want all the crap powershell would normally print out
    # $ErrorView = 'ConciseView' doesn't apply to scripts so
    # create a simple good enough one
    Write-Host -ForegroundColor Red -Object $message
    exit
}

function Import-ModuleEx {
    param (
        [Parameter(Mandatory = $true)][string]$name,
        [Parameter(Mandatory = $true)][string]$version
    )

    $modules = Get-Module -Name $name -ListAvailable   
    
    if ($null -eq $modules) {
        Write-TerminatingError "Module missing, run: Install-Module -Name $name -RequiredVersion $version"
    }

    if ($null -eq ($modules.Version | Where-Object { $_ -eq $version })) {
        $foundVersions = $modules.Version
        $foundVersions = [string]::Join(", ", $foundVersions)
        Write-NonTerminatingError "While loading $name expected to find version $version, but found $foundVersions."
        Write-TerminatingError "Update prompt settings or install via: Install-Module -Name $name -RequiredVersion $version"
    }

    Import-Module -Name $name -RequiredVersion $version
}

function Test-Font {
    param (
        [Parameter(Mandatory = $true)][string]$name,
        [Parameter(Mandatory = $true)][string]$remediationInfo
    )

    $font = Get-ItemProperty 'HKCU:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Fonts' -Name $name -ErrorAction SilentlyContinue
    if ($null -ne $font) {
        return;
    }

    $font = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Fonts' -Name $name -ErrorAction SilentlyContinue
    if ($null -ne $font) {
        return;
    }

    Write-Warning "Font '$name' was not found, you may have some rendering errors. To remediate: $remediationInfo"
}
