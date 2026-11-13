package org.therealbatu.badlion;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.graphics.Color;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.JavascriptInterface;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.FrameLayout;
import org.json.JSONObject;
import java.io.File;
import java.io.FileInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

/** Install from the actual game's Activity on the UI thread during onCreate/onResume.
 * The launcher must arrange to load this class. A .levipack does not load DEX by itself. */
public final class BadlionMenuBridge {
    private static boolean libraryLinked;
    private final Activity activity;
    private final FrameLayout root;
    private final WebView web;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private boolean destroyed;
    private boolean lastVisible = true;
    private final Runnable sync = new Runnable() {
        @Override public void run() {
            if (destroyed) return;
            try {
                JSONObject value = new JSONObject(nativeSnapshot());
                boolean visible = value.optBoolean("visible", true);
                if (visible != lastVisible) {
                    lastVisible = visible;
                    web.setVisibility(visible ? View.VISIBLE : View.GONE);
                    if (visible) web.evaluateJavascript("window.BadlionUI && BadlionUI.refresh()", null);
                } else if (visible) {
                    web.evaluateJavascript("window.BadlionUI && BadlionUI.refresh()", null);
                }
            } catch (Exception ignored) {
                web.setVisibility(View.GONE);
            }
            handler.postDelayed(this, 350);
        }
    };

    private static native String nativeSnapshot();
    private static native boolean nativeSetEnabled(String id, boolean enabled);
    private static native void nativeSetVisible(boolean visible);
    private static native void nativeSetMode(String mode);
    private static native void nativeSetOpacity(int value);

    /** Requires an injected Java host or launcher modification that can obtain the Activity.
     * nativeLibraryPath is the absolute path of the installed .so, not an arbitrary URL. */
    public static BadlionMenuBridge install(Activity activity, File modDir,
                                              String nativeLibraryPath) throws IOException {
        if (Looper.myLooper() != Looper.getMainLooper())
            throw new IllegalStateException("install must run on Android main thread");
        if (activity == null || modDir == null || nativeLibraryPath == null)
            throw new IllegalArgumentException("activity, modDir, nativeLibraryPath required");
        synchronized (BadlionMenuBridge.class) {
            if (!libraryLinked) {
                System.load(nativeLibraryPath);
                libraryLinked = true;
            }
        }
        File html = new File(modDir, "resources/ui/index.html");
        String source = readUtf8(html);
        return new BadlionMenuBridge(activity, source);
    }

    @SuppressLint({"SetJavaScriptEnabled", "AddJavascriptInterface"})
    private BadlionMenuBridge(Activity a, String html) {
        activity = a;
        root = new FrameLayout(a);
        root.setBackgroundColor(Color.TRANSPARENT);
        web = new WebView(a);
        web.setBackgroundColor(Color.TRANSPARENT);
        WebSettings settings = web.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setAllowFileAccess(false);
        settings.setAllowContentAccess(false);
        settings.setJavaScriptCanOpenWindowsAutomatically(false);
        settings.setSupportMultipleWindows(false);
        web.setWebViewClient(new WebViewClient() {
            @Override public boolean shouldOverrideUrlLoading(WebView view, String url) {
                return !url.startsWith("https://badlion.local/");
            }
        });
        web.addJavascriptInterface(new HostApi(), "BadlionHost");
        root.addView(web, new FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        ViewGroup decor = (ViewGroup) activity.getWindow().getDecorView();
        decor.addView(root, new ViewGroup.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        web.loadDataWithBaseURL("https://badlion.local/", html, "text/html", "utf-8", null);
        handler.post(sync);
    }

    public void dispose() {
        if (Looper.myLooper() != Looper.getMainLooper())
            throw new IllegalStateException("dispose must run on UI thread");
        destroyed = true;
        handler.removeCallbacks(sync);
        ((ViewGroup) root.getParent()).removeView(root);
        web.removeJavascriptInterface("BadlionHost");
        web.destroy();
    }

    public void refreshLayout() {
        if (destroyed) return;
        web.evaluateJavascript("window.dispatchEvent(new Event('resize'))", null);
    }

    private static String readUtf8(File f) throws IOException {
        try (FileInputStream stream = new FileInputStream(f);
             ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            byte[] bytes = new byte[8192]; int n;
            while ((n = stream.read(bytes)) != -1) {
                output.write(bytes, 0, n);
                if (output.size() > 1024 * 1024)
                    throw new IOException("HTML asset too large");
            }
            return new String(output.toByteArray(), StandardCharsets.UTF_8);
        }
    }

    public final class HostApi {
        @JavascriptInterface public String snapshot() { return nativeSnapshot(); }
        @JavascriptInterface public boolean setEnabled(String id, boolean enabled) {
            return nativeSetEnabled(id, enabled);
        }
        @JavascriptInterface public void setVisible(boolean visible) {
            nativeSetVisible(visible);
        }
        @JavascriptInterface public void setMode(String mode) { nativeSetMode(mode); }
        @JavascriptInterface public void setOpacity(int value) { nativeSetOpacity(value); }
    }
}
