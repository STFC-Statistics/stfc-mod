# Deployment script for STFC mod
# Handles file locking by retrying multiple times

$Source = "c:\Users\Danny\Desktop\dev\stfc-mod\build\windows\x64\release\stfc-community-mod.dll"
$Target = "C:\Games\Star Trek Fleet Command\Star Trek Fleet Command\default\game\version.dll"
$MaxRetries = 10
$RetryDelay = 2

Write-Host "Attempting to deploy mod DLL..."
Write-Host "Source: $Source"
Write-Host "Target: $Target"

for ($i = 1; $i -le $MaxRetries; $i++) {
    try {
        Copy-Item -Force $Source $Target -ErrorAction Stop
        Write-Host "✅ Deployment successful!"
        
        # Verify deployment
        $SourceHash = Get-FileHash $Source -Algorithm SHA256
        $TargetHash = Get-FileHash $Target -Algorithm SHA256
        
        if ($SourceHash.Hash -eq $TargetHash.Hash) {
            Write-Host "✅ File verification successful!"
            Write-Host "Hash: $($SourceHash.Hash)"
            exit 0
        } else {
            Write-Host "❌ File verification failed!"
            exit 1
        }
    }
    catch {
        Write-Host "⚠️ Attempt $i/$MaxRetries failed: $($_.Exception.Message)"
        if ($i -lt $MaxRetries) {
            Write-Host "   Retrying in $RetryDelay seconds..."
            Start-Sleep $RetryDelay
        }
    }
}

Write-Host "❌ Deployment failed after $MaxRetries attempts"
Write-Host "   Please close the game and try again"
exit 1
