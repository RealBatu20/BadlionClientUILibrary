# v1.1.6 — Levi Button Background Hotfix (rebuilt November 13, 2026)

This is an updated binary of **1.1.6**, not the earlier build with the same version number. Uninstall the earlier build before importing. The native registered button sets nonzero, nearly transparent ARGB 0x01000000 for normal/active background and border to suppress LeviLaunchroid 1.6.1's `ExternalButtonOverlay` GradientDrawable wrapper. Only the supplied before/after PNGs remain visible. The single Levi HUD button, startup Show Everywhere preference, position/size editor, and all HTML features are unchanged.

Read `docs/FIX_BUTTON_ARTWORK_BACKGROUND.md` or the [docs page](docs/button-artwork.html). Nine offline tests pass. Android device behavior remains unverified.

The Termux publisher (`scripts/publish_badlion_v116_termux.sh`) uploads this rebuilt archive, creating a remote backup branch before updating main.
