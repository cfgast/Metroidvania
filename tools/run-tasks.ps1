<#
.SYNOPSIS
    Loops through ImplementationPlan.md and invokes the Copilot CLI agent
    for each uncompleted task, one at a time.

.DESCRIPTION
    Parses ImplementationPlan.md to find the next task marked "Implemented: false",
    then launches copilot in non-interactive mode to implement it.
    Repeats until no uncompleted tasks remain.

.EXAMPLE
    .\tools\run-tasks.ps1
    .\tools\run-tasks.ps1 -DryRun
    .\tools\run-tasks.ps1 -MaxTasks 3
#>
param(
    [string]$PlanFile = "ImplementationPlan.md",
    [switch]$DryRun,
    [int]$MaxTasks = 0
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-NextTask {
    param([string]$Path)
    $content = Get-Content $Path -Raw
    $blocks = $content -split '(?m)^={2,}\s*$'
    for ($i = 0; $i -lt $blocks.Count; $i++) {
        $block = $blocks[$i].Trim()
        if ($block -match '(?s)^Task:\s*(.+?)(?:\r?\n)Implemented:\s*false\s*$') {
            return $Matches[1].Trim()
        }
    }
    return $null
}

function Get-AllPendingTasks {
    param([string]$Path)
    $content = Get-Content $Path -Raw
    $blocks = $content -split '(?m)^={2,}\s*$'
    $tasks = @()
    for ($i = 0; $i -lt $blocks.Count; $i++) {
        $block = $blocks[$i].Trim()
        if ($block -match '(?s)^Task:\s*(.+?)(?:\r?\n)Implemented:\s*false\s*$') {
            $tasks += $Matches[1].Trim()
        }
    }
    return $tasks
}

$repoRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$planPath = Join-Path $repoRoot $PlanFile

if (-not (Test-Path $planPath)) {
    Write-Error "Plan file not found: $planPath"
    exit 1
}

$completed = 0

if ($DryRun) {
    $allTasks = Get-AllPendingTasks -Path $planPath
    if ($allTasks.Count -eq 0) {
        Write-Host "[DONE] All tasks are complete!" -ForegroundColor Green
        exit 0
    }
    $count = $allTasks.Count
    Write-Host "Pending tasks: $count" -ForegroundColor Cyan
    for ($i = 0; $i -lt $allTasks.Count; $i++) {
        $num = $i + 1
        Write-Host "---------------------------------------------" -ForegroundColor Cyan
        Write-Host "Task ${num}: $($allTasks[$i])" -ForegroundColor Yellow
        Write-Host "  [DRY RUN] Would invoke copilot for this task." -ForegroundColor DarkGray
    }
    Write-Host "$count tasks remaining." -ForegroundColor Cyan
    exit 0
}

while ($true) {
    $task = Get-NextTask -Path $planPath
    if (-not $task) {
        Write-Host "[DONE] All tasks are complete!" -ForegroundColor Green
        break
    }

    $completed++
    Write-Host "---------------------------------------------" -ForegroundColor Cyan
    Write-Host "Task ${completed}: $task" -ForegroundColor Yellow
    Write-Host "---------------------------------------------" -ForegroundColor Cyan

    $prompt = "Read the ReadMe.md and ImplementationPlan.md. Implement the next uncompleted task. The task is: $task -- After implementing, build the project to verify it compiles, mark the task as Implemented: true in ImplementationPlan.md, and commit and push the changes."
    Write-Host "  Launching copilot agent..." -ForegroundColor DarkGray
    copilot -p $prompt --yolo --autopilot
    $exitCode = $LASTEXITCODE

    if ($exitCode -ne 0) {
        Write-Host "  [WARN] Agent exited with code $exitCode. Stopping." -ForegroundColor Red
        exit $exitCode
    }

    $check = Get-NextTask -Path $planPath
    if ($check -eq $task) {
        Write-Host "  [WARN] Task was NOT marked as complete. Stopping." -ForegroundColor Red
        exit 1
    }

    Write-Host "  [OK] Task completed." -ForegroundColor Green

    if ($MaxTasks -gt 0 -and $completed -ge $MaxTasks) {
        Write-Host "Reached MaxTasks limit. Stopping." -ForegroundColor Yellow
        break
    }
}

Write-Host "Finished. $completed tasks processed." -ForegroundColor Cyan
