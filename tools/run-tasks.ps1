<#
.SYNOPSIS
    Loops through ImplementationPlan.md and invokes the Copilot CLI agent
    for each uncompleted task, one at a time.

.DESCRIPTION
    Parses ImplementationPlan.md to find the next task marked "Implemented: false",
    then launches `copilot` in non-interactive mode to implement it.
    Repeats until no uncompleted tasks remain.

.EXAMPLE
    .\tools\run-tasks.ps1
    .\tools\run-tasks.ps1 -DryRun          # preview tasks without running
    .\tools\run-tasks.ps1 -MaxTasks 3      # stop after 3 tasks
#>
param(
    [string]$PlanFile = "ImplementationPlan.md",
    [switch]$DryRun,
    [int]$MaxTasks = 0  # 0 = unlimited
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

# --- Main loop ---
$repoRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$planPath = Join-Path $repoRoot $PlanFile

if (-not (Test-Path $planPath)) {
    Write-Error "Plan file not found: $planPath"
    exit 1
}

$completed = 0

# In dry-run mode, just list all pending tasks and exit
if ($DryRun) {
    $allTasks = Get-AllPendingTasks -Path $planPath
    if ($allTasks.Count -eq 0) {
        Write-Host "`n✅ All tasks in $PlanFile are complete!" -ForegroundColor Green
        exit 0
    }
    Write-Host "`nPending tasks ($($allTasks.Count)):" -ForegroundColor Cyan
    for ($i = 0; $i -lt $allTasks.Count; $i++) {
        Write-Host "`n─────────────────────────────────────────────" -ForegroundColor Cyan
        Write-Host "Task $($i + 1) : $($allTasks[$i])" -ForegroundColor Yellow
        Write-Host "  [DRY RUN] Would invoke copilot for this task." -ForegroundColor DarkGray
    }
    Write-Host "`n$($allTasks.Count) task(s) remaining." -ForegroundColor Cyan
    exit 0
}

while ($true) {
    $task = Get-NextTask -Path $planPath
    if (-not $task) {
        Write-Host "`n✅ All tasks in $PlanFile are complete!" -ForegroundColor Green
        break
    }

    $completed++
    Write-Host "`n─────────────────────────────────────────────" -ForegroundColor Cyan
    Write-Host "Task $completed : $task" -ForegroundColor Yellow
    Write-Host "─────────────────────────────────────────────" -ForegroundColor Cyan

    $prompt = @"
Read the ReadMe.md and ImplementationPlan.md. Implement the next uncompleted task (Implemented: false). The task is:

$task

After implementing, build the project to verify it compiles, mark the task as Implemented: true in ImplementationPlan.md, and commit + push the changes.
"@
    Write-Host "  Launching copilot agent..." -ForegroundColor DarkGray
    copilot -p $prompt --yolo --autopilot
    $exitCode = $LASTEXITCODE

    if ($exitCode -ne 0) {
        Write-Host "  ⚠️  Agent exited with code $exitCode. Stopping." -ForegroundColor Red
        exit $exitCode
    }

    # Re-read plan to confirm the task was marked done
    $check = Get-NextTask -Path $planPath
    if ($check -eq $task) {
        Write-Host "  ⚠️  Task was NOT marked as complete. Stopping to avoid an infinite loop." -ForegroundColor Red
        exit 1
    }

    Write-Host "  ✅ Task completed." -ForegroundColor Green

    if ($MaxTasks -gt 0 -and $completed -ge $MaxTasks) {
        Write-Host "`nReached --MaxTasks limit ($MaxTasks). Stopping." -ForegroundColor Yellow
        break
    }
}

Write-Host "`nFinished. $completed task(s) processed." -ForegroundColor Cyan
