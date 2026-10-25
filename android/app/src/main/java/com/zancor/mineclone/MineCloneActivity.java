package com.zancor.mineclone;

import android.app.AlertDialog;
import android.util.Log;
import android.view.inputmethod.InputMethodManager;
import android.app.NativeActivity;
import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.text.InputFilter;
import android.text.InputType;
import android.view.View;
import android.view.WindowManager;
import android.widget.EditText;

public final class MineCloneActivity extends NativeActivity {
    // NativeActivity opens the .so through dlopen. Register it with the Java VM
    // as well so dialog callbacks can resolve nativeText instead of throwing
    // UnsatisfiedLinkError when the user confirms or cancels an edit.
    static { System.loadLibrary("mineclone"); }
    private WifiManager.MulticastLock multicastLock;
    private AlertDialog textDialog;
    private static native void nativeText(int token, String text);

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        immersive();
        WifiManager wifi = (WifiManager)getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        if (wifi != null) {
            multicastLock = wifi.createMulticastLock("MineClone LAN");
            multicastLock.setReferenceCounted(false);
        }
    }
    private void immersive() {
        getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
    }
    @Override public void onWindowFocusChanged(boolean focused) { super.onWindowFocusChanged(focused); if (focused && textDialog == null) immersive(); }
    @Override protected void onResume() { super.onResume(); if (multicastLock != null && !multicastLock.isHeld()) multicastLock.acquire(); }
    @Override protected void onPause() { if (multicastLock != null && multicastLock.isHeld()) multicastLock.release(); super.onPause(); }
    @Override protected void onDestroy() { if (textDialog != null) textDialog.dismiss(); super.onDestroy(); }

    // Called from the native render thread; all Android widgets stay on the UI thread.
    public void editGameText(final String current, final int limit, final int kind, final int token) {
        runOnUiThread(() -> {
            if (isFinishing() || isDestroyed()) return;
            try {
                if (textDialog != null) textDialog.dismiss();
                EditText input = new EditText(this);
                input.setSingleLine(true);
                input.setInputType(kind == 2 ? InputType.TYPE_CLASS_NUMBER : InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
                input.setFilters(new InputFilter[]{new InputFilter.LengthFilter(limit)});
                input.setText(current);
                input.selectAll();
                AlertDialog dialog = new AlertDialog.Builder(this, android.R.style.Theme_Material_Dialog_Alert)
                    .setTitle("MineClone").setView(input)
                    .setPositiveButton("OK", (d, which) -> nativeText(token, input.getText().toString()))
                    .setNegativeButton("Cancelar", (d, which) -> nativeText(token, null))
                    .setOnCancelListener(d -> nativeText(token, null)).create();
                textDialog = dialog;
                dialog.setOnDismissListener(d -> { if (textDialog == dialog) textDialog = null; immersive(); });
                dialog.setOnShowListener(d -> {
                    input.requestFocus();
                    dialog.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
                    input.post(() -> {
                        if (!dialog.isShowing() || isFinishing() || isDestroyed()) return;
                        InputMethodManager keyboard = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
                        if (keyboard != null) keyboard.showSoftInput(input, InputMethodManager.SHOW_IMPLICIT);
                    });
                });
                dialog.show();
            } catch (RuntimeException error) {
                Log.e("MineClone", "Unable to open text editor", error);
                nativeText(token, null);
            }
        });
    }
}
