# Upload the complete BadlionMenuUI repository to GitHub

**Target:** `https://github.com/RealBatu20/BadlionClientUILibrary` (branch `main`). The GitHub-ready ZIP contains every project source, documentation Markdown file, generated GitHub Pages HTML/CSS/JS, demo tests, supplied UI assets, compiled release `.levipack`, `.so`, and release source archive. It intentionally does **not** contain the 113 MB Android NDK or the proprietary Minecraft binary supplied for building/testing.

## Prefer a GitHub app connection

If publishing through ChatGPT, connect the `RealBatu20` GitHub account under ChatGPT **Settings → Plugins → GitHub** and grant the GitHub App **Contents: Read and write** access to `RealBatu20/BadlionClientUILibrary`. A GitHub connector authenticated as you is not sufficient when its GitHub App installation covers other accounts only; GitHub returns HTTP 403. After connecting, ask ChatGPT to upload the prepared v1.1.4 repository. Connecting a plugin requires your explicit action.

## Publish with Git on your own computer

First extract the `BadlionClientUILibrary_GitHub_Ready_v1.1.4.zip` contents into a temporary directory. Copy them to an authenticated clone of the target repository:

```bash
git clone https://github.com/RealBatu20/BadlionClientUILibrary.git
cd BadlionClientUILibrary
# Copy everything from the unzipped GitHub-ready directory HERE,
# including .github/ and all hidden files, overwriting README.md.
git add -A
git commit -m "Release BadlionMenuUI v1.1.4 and GitHub Pages docs"
git push origin main
```

The repository currently contains only a small README, so a normal push should be sufficient. **Do not use `git push --force` by default.** If you explicitly need to replace divergent remote history and have checked the changes, prefer `--force-with-lease` over `--force`.

## Enable documentation site

After the commit reaches GitHub, open **Settings → Pages → Build and deployment → Source → GitHub Actions**. The included `.github/workflows/pages.yml` deploys the `docs/` directory. Once the workflow successfully finishes, open:

`https://realbatu20.github.io/BadlionClientUILibrary/`

A Pages workflow does not prove that Pages has been enabled. Verify under the GitHub Actions and Pages screens.

## Release integrity

The `releases/1.1.4/` folder contains the exact compiled artifacts. File checksums are in `releases/1.1.4/BadlionMenuUI_build_report.json`. The underlying compiled binary has not been retested on a physical Android device in this release; browser and ARM64 build checks are independent verification steps.
