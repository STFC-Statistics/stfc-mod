[CmdletBinding()]
param(
    [Parameter(Mandatory = $false)]
    [string]$ModRoot = "",

    [Parameter(Mandatory = $true)]
    [string]$M931Root,

    [Parameter(Mandatory = $true)]
    [string]$M93DevRoot,

    [Parameter(Mandatory = $false)]
    [string]$OutputPath = "",

    [Parameter(Mandatory = $false)]
    [string]$JsonOutputPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$workspaceRoot = (Resolve-Path (Join-Path $scriptRoot "..")).Path
if ([string]::IsNullOrWhiteSpace($ModRoot)) {
    $ModRoot = $workspaceRoot
}
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $workspaceRoot "docs\drift_report.md"
}

function Resolve-ExistingPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    $resolved = Resolve-Path -LiteralPath $Path -ErrorAction SilentlyContinue
    if ($null -eq $resolved) {
        throw "Path does not exist: $Path"
    }
    return $resolved.Path
}

function Get-SourceFiles {
    param([Parameter(Mandatory = $true)][string]$Root)

    $extensions = @("*.cc", "*.h", "*.cpp", "*.cxx", "*.hpp")
    $files = foreach ($extension in $extensions) {
        Get-ChildItem -LiteralPath $Root -Recurse -File -Filter $extension
    }
    return @($files | Sort-Object FullName -Unique)
}

function Get-LiteralLookups {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$RelativePath
    )

    $results = New-Object System.Collections.Generic.List[object]
    $lines = $Text -split "`r?`n"

    for ($index = 0; $index -lt $lines.Count; $index++) {
        $line = $lines[$index]
        $classPattern = 'il2cpp_get_class_helper\(\s*"(?<assembly>[^"]+)"\s*,\s*"(?<namespace>[^"]*)"\s*,\s*"(?<class>[^"]+)"\s*\)'
        $classMatches = ([regex]::new($classPattern)).Matches([string]$line)

        foreach ($match in $classMatches) {
            $results.Add([pscustomobject]@{
                Kind       = "class"
                Assembly   = $match.Groups["assembly"].Value
                Namespace  = $match.Groups["namespace"].Value
                Class      = $match.Groups["class"].Value
                Member     = ""
                File       = $RelativePath
                Line       = $index + 1
                Dynamic    = $false
            })
        }

        $memberPattern = '(?<kind>GetMethodInfo|GetMethod|GetField|GetProperty)\(\s*"(?<member>[^"]+)"'
        $memberMatches = ([regex]::new($memberPattern)).Matches([string]$line)
        foreach ($match in $memberMatches) {
            $results.Add([pscustomobject]@{
                Kind       = "member"
                Assembly   = ""
                Namespace  = ""
                Class      = ""
                Member     = $match.Groups["member"].Value
                File       = $RelativePath
                Line       = $index + 1
                Dynamic    = $false
            })
        }

        if ($line -match '(GetMethodInfo|GetMethod|GetField|GetProperty)\s*\(\s*[^"\s]') {
            $results.Add([pscustomobject]@{
                Kind       = "manual-review"
                Assembly   = ""
                Namespace  = ""
                Class      = ""
                Member     = "dynamic lookup"
                File       = $RelativePath
                Line       = $index + 1
                Dynamic    = $true
            })
        }
    }

    return $results.ToArray()
}

function Find-ClassFiles {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Assembly,
        [Parameter(Mandatory = $true)][AllowEmptyString()][string]$Namespace,
        [Parameter(Mandatory = $true)][string]$Class
    )

    $expectedName = "$Class.cs"
    $safeExpectedName = $expectedName -notmatch '[<>:"/\\|?*]'
    # The available decompiler output uses dotted namespace directory names
    # (for example Digit.Client.Core), so do not split namespace dots here.
    $namespacePath = if ([string]::IsNullOrWhiteSpace($Namespace)) {
        ""
    } else {
        $Namespace
    }

    $matches = @()
    if ($safeExpectedName) {
        $expected = Join-Path (Join-Path (Join-Path $Root $Assembly) $namespacePath) $expectedName
        if (Test-Path -LiteralPath $expected -PathType Leaf) {
            $matches += (Get-Item -LiteralPath $expected)
        }
    }

    if ($matches.Count -eq 0 -and $safeExpectedName) {
        $matches = @(Get-ChildItem -LiteralPath $Root -Recurse -File -Filter $expectedName |
            Where-Object {
                $relative = $_.FullName.Substring($Root.Length).TrimStart("\", "/")
                ($relative -like "$Assembly\$namespacePath\$expectedName") -or
                ($relative -like "*$namespacePath\$expectedName")
            })
    }

    return @($matches | Sort-Object FullName -Unique)
}

function Test-MemberInClassFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Member
    )

    $text = Get-Content -LiteralPath $Path -Raw
    $escaped = [regex]::Escape($Member)
    return [regex]::IsMatch($text, "(?m)\b$escaped\b")
}

$modRootResolved = Resolve-ExistingPath $ModRoot
$m931Resolved = Resolve-ExistingPath $M931Root
$m93DevResolved = Resolve-ExistingPath $M93DevRoot

$sourceRoot = Join-Path $modRootResolved "mods\src"
$modFiles = Get-SourceFiles $sourceRoot
$lookups = New-Object System.Collections.Generic.List[object]

foreach ($file in $modFiles) {
    $relative = $file.FullName.Substring($modRootResolved.Length).TrimStart("\", "/")
    $text = Get-Content -LiteralPath $file.FullName -Raw
    foreach ($lookup in (Get-LiteralLookups -Text $text -RelativePath $relative)) {
        $lookups.Add($lookup)
    }
}

$classes = @($lookups | Where-Object { $_.Kind -eq "class" } | Sort-Object Assembly, Namespace, Class, File, Line -Unique)
$manual = @($lookups | Where-Object { $_.Kind -eq "manual-review" })
$results = New-Object System.Collections.Generic.List[object]

foreach ($lookup in $classes) {
    $m931Files = @(Find-ClassFiles -Root $m931Resolved -Assembly $lookup.Assembly -Namespace $lookup.Namespace -Class $lookup.Class)
    $m93DevFiles = @(Find-ClassFiles -Root $m93DevResolved -Assembly $lookup.Assembly -Namespace $lookup.Namespace -Class $lookup.Class)

    $status = if ($m931Files.Count -gt 0 -and $m93DevFiles.Count -gt 0) {
        "present-both"
    } elseif ($m931Files.Count -gt 0) {
        "m931-only"
    } elseif ($m93DevFiles.Count -gt 0) {
        "m93-dev-only"
    } else {
        "missing-both"
    }

    $results.Add([pscustomobject]@{
        Kind          = "class"
        Assembly      = $lookup.Assembly
        Namespace     = $lookup.Namespace
        Class         = $lookup.Class
        Member        = ""
        Source        = $lookup.File
        Line          = $lookup.Line
        Status        = $status
        M931Files     = @($m931Files | ForEach-Object { $_.FullName })
        M93DevFiles   = @($m93DevFiles | ForEach-Object { $_.FullName })
        Evidence      = if ($status -eq "present-both") { "static-confirmed" } else { "manual-review" }
    })
}

# Member lookup extraction is intentionally reported with source locations. Without a complete
# parser for C++ object scope, associating every member with a class helper can be ambiguous.
foreach ($lookup in @($lookups | Where-Object { $_.Kind -eq "member" })) {
    $results.Add([pscustomobject]@{
        Kind          = "member"
        Assembly      = ""
        Namespace     = ""
        Class         = "scope-ambiguous"
        Member        = $lookup.Member
        Source        = $lookup.File
        Line          = $lookup.Line
        Status        = "manual-review"
        M931Files     = @()
        M93DevFiles   = @()
        Evidence      = "manual-review"
    })
}

foreach ($item in $manual) {
    $results.Add([pscustomobject]@{
        Kind          = "manual-review"
        Assembly      = ""
        Namespace     = ""
        Class         = ""
        Member        = $item.Member
        Source        = $item.File
        Line          = $item.Line
        Status        = "manual-review"
        M931Files     = @()
        M93DevFiles   = @()
        Evidence      = "manual-review"
    })
}

$results = @($results | Sort-Object Kind, Assembly, Namespace, Class, Member, Source, Line)

$outputResolved = [IO.Path]::GetFullPath($OutputPath)
$outputDirectory = Split-Path -Parent $outputResolved
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null

$presentBoth = @($results | Where-Object { $_.Status -eq "present-both" }).Count
$missingBoth = @($results | Where-Object { $_.Status -eq "missing-both" }).Count
$manualCount = @($results | Where-Object { $_.Evidence -eq "manual-review" }).Count

$report = New-Object System.Collections.Generic.List[string]
$report.Add("# IL2CPP Drift Report")
$report.Add("")
$report.Add("Generated: $(Get-Date -Format o)")
$report.Add("")
$report.Add("This is a static lookup preflight. It does not prove native detour safety, signatures, offsets, or runtime behavior.")
$report.Add("")
$report.Add("| Metric | Count |")
$report.Add("|---|---:|")
$report.Add("| Class lookups present in both builds | $presentBoth |")
$report.Add("| Class lookups missing in both builds | $missingBoth |")
$report.Add("| Manual-review results | $manualCount |")
$report.Add("")
$report.Add("## Results")
$report.Add("")
$report.Add("| Kind | Assembly | Namespace | Class | Member | Source | Line | Status | Evidence |")
$report.Add("|---|---|---|---|---|---|---:|---|---|")

foreach ($result in $results) {
    $values = @(
        $result.Kind,
        $result.Assembly,
        $result.Namespace,
        $result.Class,
        $result.Member,
        $result.Source,
        $result.Line,
        $result.Status,
        $result.Evidence
    ) | ForEach-Object { ([string]$_).Replace("|", "\|") }
    $report.Add("| " + ($values -join " | ") + " |")
}

$report.Add("")
$report.Add("## Manual review requirements")
$report.Add("")
$report.Add("- Resolve scope-ambiguous member lookups against the owning class helper.")
$report.Add("- Resolve dynamic lookup names and overloads from dump/metadata and runtime logs.")
$report.Add("- Confirm game-visible signatures and original-call fallbacks before detouring.")
$report.Add("- Validate short ARM64 functions and hardcoded offsets with runtime evidence.")

Set-Content -LiteralPath $outputResolved -Value ($report -join "`r`n") -Encoding UTF8

if ([string]::IsNullOrWhiteSpace($JsonOutputPath)) {
    $JsonOutputPath = [IO.Path]::ChangeExtension($outputResolved, ".json")
}
$jsonResolved = [IO.Path]::GetFullPath($JsonOutputPath)
$jsonDirectory = Split-Path -Parent $jsonResolved
New-Item -ItemType Directory -Force -Path $jsonDirectory | Out-Null

$payload = [pscustomobject]@{
    GeneratedAt = (Get-Date).ToUniversalTime().ToString("o")
    ModRoot = $modRootResolved
    M931Root = $m931Resolved
    M93DevRoot = $m93DevResolved
    Results = $results
}
$payload | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $jsonResolved -Encoding UTF8

if ($missingBoth -gt 0) {
    Write-Warning "$missingBoth class lookup(s) were not found in either decompiled tree. Review $outputResolved."
}
if ($manualCount -gt 0) {
    Write-Warning "$manualCount result(s) require manual review. This is expected for member scope and dynamic lookups."
}

Write-Host "Wrote report: $outputResolved"
Write-Host "Wrote JSON:   $jsonResolved"
Write-Host "Class lookups: $($classes.Count); manual review: $manualCount"
