# Publish BadlionMenuUI v1.1.5 from Termux

Target: `https://github.com/RealBatu20/BadlionClientUILibrary` (`main`).

Download these two files using your browser to **Android Downloads**:
- `BadlionClientUILibrary_GitHub_Ready_v1.1.5.zip`
- `publish_badlion_v115_termux.sh`

In Termux (F-Droid/GitHub Termux recommended), run:

```bash
pkg update -y
pkg install -y git unzip gh
termux-setup-storage
bash ~/storage/downloads/publish_badlion_v115_termux.sh \
  ~/storage/downloads/BadlionClientUILibrary_GitHub_Ready_v1.1.5.zip
```

Approve Android's file-access permission. If GitHub CLI asks you to log in, use the **RealBatu20** account in the browser; do not paste any personal access token into this chat. The script checks the login, clones the existing public repo, creates a remote `backup-before-badlion-v1.1.5-...` branch, unpacks all project files (including hidden `.github` files), commits them, and uses `git push --force-with-lease origin main`. Force-with-lease refuses to overwrite concurrent changes not seen by the local clone. No force push is necessary for a normal fast-forward, but this command honors the user's explicit replacement request while protecting the previous branch with a backup.

It attempts to enable or switch the GitHub Pages source to GitHub Actions. If this attempt fails, go to [Pages settings](https://github.com/RealBatu20/BadlionClientUILibrary/settings/pages), set **Build and deployment → Source → GitHub Actions**, and save. Check [GitHub Actions](https://github.com/RealBatu20/BadlionClientUILibrary/actions) for the **Deploy BadlionMenuUI documentation** workflow. If no run appears after the Pages setting is enabled, run it using **Actions → Deploy BadlionMenuUI documentation → Run workflow**.

When the deployment reports success, open:

https://realbatu20.github.io/BadlionClientUILibrary/

A script cannot bypass GitHub branch protection or GitHub authentication. If a push is rejected with a 403, ensure you logged into `RealBatu20` in `gh`, and that it has write access. If `main` is protected against force pushes, use a normal `git push origin main` instead, or review repository rules. Do not disable safety rules without reviewing what they protect.

The archive intentionally excludes Android NDK and proprietary Minecraft binary dependencies; it contains all project files and release binaries. The repository's `docs/` site is public after publishing.
