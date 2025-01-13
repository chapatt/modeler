package io.abalog.modeler

import com.google.androidgamesdk.GameActivity
import android.view.WindowInsets
import android.content.res.Configuration

class MainActivity : GameActivity() {
    companion object {
        init {
            System.loadLibrary("modeler")
        }
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
            hideSystemUi()
        } else if (newConfig.orientation == Configuration.ORIENTATION_PORTRAIT) {
            showSystemUi()
        }
    }

    private fun hideSystemUi() {
        window.insetsController?.hide(WindowInsets.Type.statusBars())
    }

    private fun showSystemUi() {
        window.insetsController?.show(WindowInsets.Type.statusBars())
    }
}