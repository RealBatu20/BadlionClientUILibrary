# GitHub Pages deployment and documentation site

This repository includes a **static GitHub Pages site** generated from the Markdown guides in `docs/*.md`. Its starting page is `docs/index.html`, with individual documentation pages directly in `docs/`, and site assets in `docs/assets/`.

## Authorize the GitHub connector to publish (if using ChatGPT)

GitHub App installations and repository access must include the `RealBatu20` account and specifically `RealBatu20/BadlionClientUILibrary`, with **Contents: Read and write** permission. An integration installed only for another GitHub account will return `403 Resource not accessible by integration` even when the target repository is public. In ChatGPT open Settings → Plugins → GitHub to review/reconnect the account, then ensure the corresponding GitHub App installation has access to the repository. This is an account authorization step; the included workflow cannot grant itself permission to write source files.

## Enable GitHub Pages

1. Open `https://github.com/RealBatu20/BadlionClientUILibrary/settings/pages` (repository admin).
2. Choose **Build and deployment → Source → GitHub Actions**.
3. Confirm the `.github/workflows/pages.yml` workflow is enabled in the **Actions** tab. Pushing to `main` should deploy `docs/`.
4. When GitHub reports deployment success, open `https://realbatu20.github.io/BadlionClientUILibrary/`.

GitHub Pages settings cannot be changed through the available repository file APIs, so a workflow file alone does **not** prove that the site is live. If Pages is already enabled for Actions deployments, the workflow publishes automatically.

## Rebuild the site

```sh
python3 -m pip install -r tools/requirements-docs.txt
python3 tools/build_docs.py
```

The generator converts the Markdown guides into independent HTML pages with a shared sidebar, responsive layout, quick navigation, search filtering, and code highlighting. It does not require a Node build step or a CDN.

## Header text API

`BadlionUI_SetTitle("MY CLIENT")` changes the title of the in-game HTML menu. Settings → Menu Header Text provides a local override without changing the original condensed font. The Premium tab was completely removed in v1.1.5.

## Publishing source and binaries

Commit the source tree, full docs, generated `web/index.html`, `dex/classes.dex`, and compiled release archives. The docs preview screenshots are under `previews/`. For an ARM64 rebuild use `tools/build.sh` with the supplied NDK and matching Preloader .so. Always publish release archives and their hashes together; don't claim an untested Android build has been verified on a physical device.
