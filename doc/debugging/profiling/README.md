<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer-docs).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->

# Profiling

Here, you can learn how to analyze the performance of your 3D app which uses a Ramses asset.

## Optimize your 3D content first

You should always first start with optimizing your content/scene. Do you have unused objects? Are there big textures or geometry
which could be optimized? Do you have too many nodes? Are large parts of your scene invisible most of the time and could be stored
separately?

There are three tools which you can use to do that:
* The Ramses Composer itself - can use to browse the scene content in a user-friendly way
* The [Ramses Scene Viewer](https://bmwcarit.github.io/ramses/SceneViewer.html) - more technical, but shows low level details about RAM and VRAM usage
* The [Ramses Logic Viewer](https://ramses-logic.readthedocs.io/en/latest/viewer.html) - helpful to analyze CPU usage (shows where the CPU time goes for a scene)

You can download a prebuilt version of the ramses tools from their respective release packages:
* [Ramses Scene Viewer](https://github.com/bmwcarit/ramses/releases)
* [Ramses Logic Viewer](https://github.com/bmwcarit/ramses-logic/releases)

If these tools don't show any obvious issues and optimization potentials, read the sections below for more in-depth analysis.

## Ramses periodic logs

Ramses offers periodic logs which provide information about the current state of the Ramses client, renderer, and the
state in which your scenes are currently. These logs also contain useful performance-related info - how many FPS
are being currently rendered, were there any unexpectedly long frames, and what is the corelation between scene updates
and rendered frames.

Find a detailed documentation about the periodic logs and how to read them in the [Ramses docs](https://bmwcarit.github.io/ramses/Profiling.html).

## Android Profiler

The Android Profiler, a tool provided as part of the Android Studio SDK, is a great tool to profile
and quickly find bottlenecks in Android apps. The memory profiler gives a great overview of the
native memory consumption of your application. In addition, if the device's OpenGL drivers support that,
the Graphics memory section can provide a hint towards the video memory required by your application.

## Specialized tools

Most hardware vendors provide specialized tools for GPU Debugging and profiling. To list a few examples:

* [NVidia NSight](https://developer.nvidia.com/nsight-graphics)
* [Intel GPA](https://www.intel.com/content/www/us/en/developer/tools/graphics-performance-analyzers/download.html)
* [Snapdragon profiler](https://developer.qualcomm.com/software/snapdragon-profiler)

Those tools are very powerful and usually give device-specific hints about optimization potentials in your app.
