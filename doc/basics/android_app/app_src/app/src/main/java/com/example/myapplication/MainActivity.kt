//  -------------------------------------------------------------------------
//  Copyright (C) 2022 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

package com.example.myapplication

import android.os.Bundle
import android.util.Log
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.appcompat.app.AppCompatActivity
import com.bmwgroup.ramses.ClearColor

class MainActivity : AppCompatActivity(), SurfaceHolder.Callback {
    // The thread which renders into the SurfaceView of the activity
    private val monkeyThread: MonkeyThread by lazy {
        MonkeyThread("MonkeyThread", applicationContext)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Loads the scene from its binary asset files
        monkeyThread.initRamsesThreadAndLoadScene(assets, "monkey.ramses", "monkey.rlogic")

        // Ramses renders into the SurfaceView
        val surfaceView = findViewById<SurfaceView>(R.id.surfaceView)
        surfaceView.holder.addCallback(this)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        // We are working with a thread - we have to handle interrupts
        try {
            // Creates a display and shows the scene we loaded in onCreate()
            monkeyThread.createDisplayAndShowScene(holder.surface, ClearColor(0F, 0F, 0F, 1F))

            // Queue a command to start rendering
            monkeyThread.addRunnableToThreadQueue {
                // This is an async thread command - it's possible the thread died or was
                // stopped until this command reaches it. We have to check!
                if (monkeyThread.isDisplayCreated && !monkeyThread.isRendering) {
                    monkeyThread.startRendering()
                }
            }
        } catch (e: InterruptedException) {
            Log.e("MainActivity", "surfaceCreated failed: ", e)
        }
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        // Tell the monkey thread to adjust its size/camera based on the new surface size
        monkeyThread.resizeDisplay(width, height)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        try {
            monkeyThread.destroyDisplay()
        } catch (e: InterruptedException) {
            Log.e("MainActivity", "surfaceDestroyed failed: ", e)
        }
    }
}
