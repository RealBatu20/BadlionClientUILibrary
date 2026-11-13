#!/data/data/com.termux/files/usr/bin/bash
# BadlionMenuUI v1.1.6 art-background hotfix GitHub publisher for Termux.
# Only changes https://github.com/RealBatu20/BadlionClientUILibrary and
# creates a remote backup branch before updating main.
set -euo pipefail

REPO='RealBatu20/BadlionClientUILibrary'
ZIPFILE="${1:-$HOME/storage/downloads/BadlionClientUILibrary_GitHub_Ready_v1.1.6.zip}"

err() { printf '\nERROR: %s\n' "$*" >&2; exit 1; }
for command in git unzip gh mktemp; do
  command -v "$command" >/dev/null 2>&1 || err "Install $command first: pkg update && pkg install git unzip gh"
done

[[ -f "$ZIPFILE" ]] || err "Archive not found: $ZIPFILE. Download the GitHub-ready ZIP to your Android Downloads folder and run termux-setup-storage."
unzip -tq "$ZIPFILE" >/dev/null || err 'ZIP verification failed; download it again.'

if ! gh auth status >/dev/null 2>&1; then
  printf 'Log in using your RealBatu20 GitHub account.\n'
  gh auth login --hostname github.com --git-protocol https --web --scopes repo,workflow
fi
ACCOUNT="$(gh api user --jq .login)"
[[ "$ACCOUNT" == 'RealBatu20' ]] || err "Currently logged in as '$ACCOUNT', not 'RealBatu20'. Use 'gh auth switch --user RealBatu20' or 'gh auth login' and retry."
gh auth setup-git

WORKDIR="$(mktemp -d "$HOME/badlion-publish-XXXXXX")"
PROJECT="$WORKDIR/BadlionClientUILibrary"
printf '\nCloning %s into %s\n' "$REPO" "$PROJECT"
git clone --branch main "https://github.com/$REPO.git" "$PROJECT"
cd "$PROJECT"
git fetch origin main

# Preserve the original remote main before overwriting any repository files.
BACKUP="backup-before-badlion-v1.1.6-$(date '+%Y%m%d-%H%M%S')"
printf '\nCreating remote backup branch: %s\n' "$BACKUP"
git push origin "refs/remotes/origin/main:refs/heads/$BACKUP" \
  || err 'Could not create remote backup branch. No main-branch changes were made.'

# Archive has no outer folder, and -o overwrites existing files (including README).
unzip -oq "$ZIPFILE" -d "$PROJECT"
[[ -f 'docs/index.html' && -f '.github/workflows/pages.yml' && -f 'releases/libBadlionMenuUI_1.1.6.levipack' && -f 'docs/FIX_BUTTON_ARTWORK_BACKGROUND.md' && -f 'src/BadlionMenu.cpp' ]] \
  || err 'Required hotfix, docs, workflow, or release files missing. Use the updated v1.1.6 ZIP.'
grep -Fq 'button.normalBgColor=0x01000000u' src/BadlionMenu.cpp \
  || err 'The archive contains the OLD opaque-square button code. Download the NEW hotfix ZIP.'
grep -Fq 'button.activeBgColor=0x01000000u' src/BadlionMenu.cpp \
  || err 'Pressed-state transparent wrapper override missing.'
grep -Fq 'button.borderColor=0x01000000u' src/BadlionMenu.cpp \
  || err 'Wrapper-border transparency override missing.'

git config user.name 'RealBatu20'
git config user.email '100330737+RealBatu20@users.noreply.github.com'
git add --all
if ! git diff --cached --quiet; then
  git commit -m 'Hotfix v1.1.6 native Levi button artwork; complete GitHub Pages docs'
fi

printf '\nUploading complete v1.1.6 button-artwork hotfix to main with force-with-lease...\n'
git push --force-with-lease origin main \
  || err 'Push rejected. Existing main is untouched; backup branch is available. Check permissions, branch protection, and any concurrent changes.'

# Pages uses a workflow, not the legacy /docs Jekyll publishing mode.
# This step is best-effort; if an account policy blocks it, use Settings > Pages.
printf '\nConfiguring GitHub Pages for GitHub Actions (if permitted)...\n'
if gh api "repos/$REPO/pages" >/dev/null 2>&1; then
  gh api -X PUT "repos/$REPO/pages" -f build_type=workflow >/dev/null \
    || printf 'Pages configuration needs manual setup in GitHub Settings > Pages.\n'
else
  gh api -X POST "repos/$REPO/pages" -f build_type=workflow >/dev/null \
    || printf 'Pages creation needs manual setup in GitHub Settings > Pages.\n'
fi


printf '\nUPLOAD SUCCESS: https://github.com/%s\n' "$REPO"
printf 'BACKUP: https://github.com/%s/tree/%s\n' "$REPO" "$BACKUP"
printf 'ACTION RUNS: https://github.com/%s/actions\n' "$REPO"
printf 'PAGES SETTINGS: https://github.com/%s/settings/pages\n' "$REPO"
printf 'EXPECTED DOCS URL: https://realbatu20.github.io/BadlionClientUILibrary/\n'
printf 'Verify the GitHub Pages deployment in Actions; the website is live only after it succeeds.\n'
