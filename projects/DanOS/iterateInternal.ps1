$ErrorActionPreference = 'Stop'
[Diagnostics.CodeAnalysis.SuppressMessageAttribute('PSUserDeclaredVarsMoreThanAssignments', 'This is a global PS state variable')]
$oldErrorState = $PSNativeCommandUseErrorActionPreference
try {
    $PSNativeCommandUseErrorActionPreference = $true
    nasm.exe .\bootloaderStage1.asm -f bin -o .\bootloaderStage1.bin
    nasm.exe .\bootloaderStage2.asm -f bin -o .\bootloaderStage2.bin
    Remove-Item .\DanOS.vhd
    New-VHD -Path .\DanOS.vhd -Fixed -SizeBytes 3MB

    $stage1Bytes = Get-Content .\bootloaderStage1.bin -Raw -AsByteStream
    if ($stage1Bytes.Length -ne 512 ) { Write-Error 'Bootloader should be exactly 512 bytes' }

    $stage2Bytes = Get-Content .\bootloaderStage2.bin -Raw -AsByteStream

    $osBytes = Get-Content .\DanOS.vhd -Raw -AsByteStream
    for ($x = 0; $x -lt $stage1Bytes.Length; $x++ ) { $osBytes[$x] = $stage1Bytes[$x] }
    for ($x = 0; $x -lt $stage2Bytes.Length; $x++ ) { $osBytes[$x+512] = $stage2Bytes[$x] }

    Set-Content -Path .\DanOS.vhd -Value $osBytes -AsByteStream
}
finally {
    $PSNativeCommandUseErrorActionPreference = $oldErrorState
}