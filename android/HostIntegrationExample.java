// Example snippet for an Android host that already owns the Minecraft Activity.
// Do not paste into an unrelated app; this must run in the game's process.
// In your Activity's onCreate/onResume (MAIN THREAD):
// BadlionMenuBridge overlay = BadlionMenuBridge.install(
//    this,
//    new File("/absolute/path/to/installed/BadlionMenuUI"),
//    "/absolute/path/to/installed/BadlionMenuUI/libbadlionmenuui.so");
// In onDestroy: overlay.dispose();
// When orientation/insets change: overlay.refreshLayout();
// Note: LeviLaunchroid 1.6.1 has no documented .levipack DEX auto-loader.
