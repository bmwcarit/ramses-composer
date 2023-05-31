<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer-docs).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->
# Common Issues

While using Ramses Composer, you may encounter a particularly annoying issue. Some of these issues are well-known and workarounds for them have been found.

This chapter is a compilation of common issues encountered by users and proposed solutions.


## I can't change position of top-level nodes

Currently, the position of top-level nodes is not changeable by design.

However, the position of node children can be switched around without any issues.

**Solution:** Create a ```root``` Node, and move Scene Graph objects under this ```root``` Node.


## I keep seeing weird flaky/clipping mesh artifacts

The ```Near Plane``` and ```Far Plane``` properties of the Cameras have to be in line with other numeric values, e.g. the precisions of the mesh shaders and the units used in the scene nodes. A mismatch can cause z-buffer rounding errors. A good rule of thumb is to scale things only on `MeshNode` level, and to choose a coordinate system which does not require scaling things higher in the node hierarchy.

Also, a low fragment shader precision may lead to clipping issues.

Aside from that: On computers with multiple GPUs, the operating system decides which program uses which GPU. By default, Ramses Composer forces the high-end GPU setting. However, the operating system can override this setting, thus launching with a weaker GPU.

**Solution 1:** Adjust the ```Near Plane``` and ```Far Plane``` properties of your Cameras in Ramses Composer.

**Solution 2:** Use high-precision shaders.

**Solution 3:** On multi-GPU systems, force your OS to use the most high-end GPU in its Graphics settings. 


## My top-level MeshNode transparencies are not in order

This issue is closely related to [nodes not being movable on the top level of a scene graph](#i-cant-change-position-of-top-level-nodes).

Currently, the render order is the same order of objects in the scenegraph (see the [Ramses Composer conventions](../../basics/conventions/README.md#rendering-order)).

Especially with top-level MeshNodes that contain MeshNode children, this can lead to unwanted rendering behavior with regards to transparencies and shadows.

This can be circumvented, just like the top-level node position issue, if you...

**Solution:** Create a ```root``` Node, and move Scene Graph objects under this ```root``` Node.


## Selecting LuaScripts with large amount of properties slows down Ramses Composer

When clicking on a scene graph object, UI widgets are dynamically generated for every property in the property browser. This can cause performance issues with LuaScripts that contain more than around 80-100 properties. We are aware of this issue and we will look at it.

**Suggestion:** Let us know when this seriously hinders progress, else try to split up larger LuaScripts into smaller partitions of multiple LuaScript objects.


## Upon launching, the application does not show the rendered scene, is unresponsive and then crashes

This happens if Ramses is not able to render the scene and might be caused by an incompatible GPU or drivers.

**Solution:** Try updating your drivers, especially the ones for your GPU.
