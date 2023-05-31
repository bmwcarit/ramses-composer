//  -------------------------------------------------------------------------
//  Copyright (C) 2022 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

package com.example.myapplication

import android.content.Context
import android.util.Log
import com.bmwgroup.ramses.Property
import com.bmwgroup.ramses.RamsesThread

class MonkeyThread(threadName: String?, context: Context?) : RamsesThread(threadName, context)
{
    override fun onSceneLoaded() {
        /// Find the root property of the camera binding in the scene
        val camera = getLogicNodeRootInput("PerspectiveCamera_CameraBinding")

        viewportWidth = camera?.getChild("viewport")?.getChild("width")
        viewportHeight = camera?.getChild("viewport")?.getChild("height")
        aspectRatio = camera?.getChild("frustum")?.getChild("aspectRatio")

        lightId = getLogicNodeRootInput("LightControl")?.getChild("light_id")
    }

    override fun onUpdate() {
        lightId?.set((frame / 60) % 3)
        frame += 1
    }

    /* Overrides the base class method which calls this based on thread scheduling
     * This method is executed from the correct thread (the one which talks to ramses)
     */
    override fun onSceneLoadFailed() {
        // Implement actions to react to failed scene load
        Log.e("RamsesSampleApp", "Loading Scene failed")
    }

    override fun onLogicUpdated() {
        // Here it's possible to read out (but not write!) scene state data, like LogicNode outputs (see RamsesThread::getLogicNodeRootOutput()).
    }

    /* Overrides the base class method which calls this based on thread scheduling
     * This method is executed from the correct thread (the one which talks to ramses)
     */
    override fun onDisplayResize(width: Int, height: Int) {
        viewportWidth?.set(width)
        viewportHeight?.set(height)
        aspectRatio?.set(width.toFloat() / height)
    }

    private var viewportWidth: Property? = null
    private var viewportHeight: Property? = null
    private var aspectRatio: Property? = null

    private var lightId: Property? = null
    private var frame = 0
}

