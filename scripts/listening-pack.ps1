# scripts/listening-pack.ps1: checkpoint M2's listening pack on Windows, the twin of listening-pack.sh (read that for
# what it does). Renders every line of scripts/listening-pack.tsv with tee-render, loudness-matches each group with
# tee-fuzz --match, and names the files so that a group's sort side by side. Nothing it writes is committed.
#
#   powershell -ExecutionPolicy Bypass -File scripts\listening-pack.ps1 [-Quick] [-Bin DIR] [-Out DIR] [-Level DBFS]
param(
    [switch]$Quick,
    [string]$Bin = "",
    [string]$Out = "",
    [double]$Level = -20
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

function Find-Exe([string]$dir, [string]$name) {
    foreach ($n in @("$name.exe", $name)) {
        $p = Join-Path $dir $n
        if (Test-Path $p -PathType Leaf) { return (Resolve-Path $p).Path }
    }
    return $null
}
if ($Bin -eq "") {
    foreach ($d in @("build-release\bin", "build-dev\bin", "build-release\bin\Release", "build-dev\bin\RelWithDebInfo")) {
        if ((Find-Exe $d "tee-render") -and (Find-Exe $d "tee-fuzz")) { $Bin = $d; break }
    }
}
if ($Bin -eq "") { $Bin = "." }
$render = Find-Exe $Bin "tee-render"
$fuzz = Find-Exe $Bin "tee-fuzz"
if (-not $render) {
    Write-Error "listening-pack: no tee-render (give -Bin: a build's bin folder or an unpacked CI artefact)"
    exit 2
}
if ($Out -eq "") { $Out = if ($Quick) { "out\listening-pack-quick" } else { "out\listening-pack" } }
New-Item -ItemType Directory -Force -Path $Out | Out-Null
$readme = Join-Path $Out "README.txt"
$commit = (& git rev-parse --short HEAD 2>$null)
@(
    "TheEngineEngine listening pack (checkpoint M2): $((Get-Date).ToUniversalTime().ToString('yyyy-MM-dd HH:mm')) UTC",
    "commit: $commit",
    "tee-render: $render",
    "What to listen for in each group, and the yes-or-no lines: docs/checkpoints/M2.md",
    "Each group is matched to $Level dBFS gated RMS (tee-fuzz --match); compare within a group, not across.",
    ""
) | Set-Content -Path $readme -Encoding UTF8

$rendered = 0; $skipped = 0; $failed = 0
$script:group = ""; $script:files = @()
function Flush-Group {
    if ($script:files.Count -gt 0) {
        if ($script:group.EndsWith("-raw")) {
            Add-Content -Path $readme "  $($script:group): not loudness-matched (its question is the loudness)"
        } elseif ($fuzz) {
            & $fuzz --match $Level @script:files 2>&1 | Add-Content -Path $readme
        } else {
            Add-Content -Path $readme "  (no tee-fuzz beside tee-render: this group is not loudness-matched)"
        }
    }
    $script:files = @()
}

foreach ($line in Get-Content "scripts\listening-pack.tsv") {
    if ($line -eq "" -or $line.StartsWith("#")) { continue }
    $f = $line.Split("`t")
    $g, $label, $q, $build, $scenario = $f[0], $f[1], $f[2], $f[3], $f[4]
    if ($Quick -and $q -ne "1") { continue }
    if ($g -ne $script:group) { Flush-Group; $script:group = $g }
    $wav = Join-Path $Out "$g-$label.wav"
    $buildPath = Join-Path "content\builds" $build
    if (-not (Test-Path $buildPath)) {
        $msg = "skipped $g-$label`: content/builds/$build is not there yet (its package has not landed)"
        Write-Host $msg; Add-Content -Path $readme $msg
        $skipped++
        continue
    }
    Write-Host "rendering $g-$label ($build, $(Split-Path -Leaf $scenario))"
    $log = Join-Path $Out ".$g-$label.log"
    & $render --build $buildPath --scenario $scenario --out $wav --csv - --sim (Join-Path $root "sim") `
        --content (Join-Path $root "content") *> $log
    if ($LASTEXITCODE -eq 0) {
        $script:files += $wav
        $rendered++
        $hit = Select-String -Path $log -Pattern "unknown|kept unread|not in the program" | Select-Object -First 1
        if ($hit) { Add-Content -Path $readme "note $g-$label`: the build names fields this program lacks: $($hit.Line)" }
    } else {
        $msg = "FAILED $g-$label`: tee-render exited $LASTEXITCODE (its words: $log)"
        Write-Host $msg; Add-Content -Path $readme $msg
        $failed++
    }
}
Flush-Group
Add-Content -Path $readme ""
$summary = "$rendered rendered, $skipped skipped, $failed failed"
Write-Host $summary; Add-Content -Path $readme $summary
Write-Host "the pack: $Out"
if ($failed -ne 0) { exit 1 }
