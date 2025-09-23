@echo off
REM Batch script to sync multiple xv6 branches to your own remote
REM Usage: sync_xv6_branches.bat
REM Make sure you run this inside your local xv6 repository

REM List of branches to sync
set BRANCHES=util pgtbl traps lazy cow

set UPSTREAM=upstream
set UPSTREAM_URL=git://g.csail.mit.edu/xv6-labs-2023

REM Check if current directory is a git repository
if not exist ".git" (
    echo Error: current directory is not a Git repository
    exit /b 1
)

REM Add temporary upstream remote
git remote add %UPSTREAM% %UPSTREAM_URL%

for %%B in (%BRANCHES%) do (
    echo =====================================================
    echo Syncing branch %%B...
    echo Fetching %%B from %UPSTREAM%...
    git fetch %UPSTREAM% %%B

    REM Create a clean local branch from upstream
    git show-ref --quiet refs/heads/%%B
    if %errorlevel%==0 (
        echo Local branch %%B exists, resetting to upstream...
        git checkout %%B
        git reset --hard %UPSTREAM%/%%B
    ) else (
        echo Creating local branch %%B from upstream...
        git checkout -b %%B %UPSTREAM%/%%B
    )

    REM Push to your own origin
    echo Pushing branch %%B to origin...
    git push origin %%B --force
)

REM Remove temporary upstream
echo Removing temporary remote %UPSTREAM%...
git remote remove %UPSTREAM%

echo =====================================================
echo All branches synced successfully.
