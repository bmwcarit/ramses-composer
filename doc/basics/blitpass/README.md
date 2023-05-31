<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer-docs).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->

# BlitPass example
*You can find the example project {{ '[here]({}/doc/basics/blitpass)'.format(repo) }}.*

![](./docs/blit_result_scene.png)

This example shows how to use `BlitPass` render pass to perform copy between render buffers with the Ramses Composer.

In OpenGL, blitting is an efficient operation to copy rectangular regions between FBOs (represented in Ramses Composer by `RenderTargets`). It allows to re-use already rendered buffers containing color, depth or other data format in subsequent render passes.

Similarly to the [offscreen rendering example](../offscreen/README.md), this example first renders the front view of a duck to an offscreen render target. Then it blits color render buffer region to another render buffer four times to create a tiled image. Finally, it uses both render buffers contents to render textured quads, as well as original mesh (seen from default camera), to the default framebuffer.

## Scene graph and resources

First, the scene is set up. It contains:

* Two meshes: a duck and a quad
* MeshNodes: `DuckMeshNode`, `QuadMeshNodeLeft` and `QuadMeshNodeRight`
* Two cameras: a `DuckCamera` for offscreen rendering of the duck front view, and the default `PerspectiveCamera` for the final scene

All meshes are rendered using `TextureMaterial`. Note that each `MeshNode` has a Private Material defined to specify individual texture and toggle UV-flipping. Quad meshes exported from Blender and to flip the V coordinate. The image below cycles through all three Private Materials:

![](./docs/private_material.gif)

## Offscreen rendering

Second, offscreen rendering using `DuckRenderPass` to `DuckRenderBuffer` is set up:

* `DuckRenderBuffer` using `RGBA8` color format and `DuckDepthRenderBuffer` using `Depth24` format are created. They are set to `DuckRenderTarget`. Binding the depth buffer to the render target allows for depth-test.
* `DuckRenderLayer` is created with Renderable Tags set to `duck`, the tag assigned also to `DuckMeshNode` mesh.
* `DuckRenderPass` is created, specifying rendering of the `DuckRenderLayer` nodes using `DuckCamera` to `DuckRenderTarget`.
* RenderOrder property of `DuckRenderPass` is set to 0: it draws to `DuckRenderTarget` buffers before MainRenderPass reads `DuckRenderBuffer` as texture for quads. Clear Color property is set to some sort of green.

## BlitPass

Next, four `BlitPass` instances are created. Each one blits the same pixel region with duck's head from `DuckRenderBuffer` to `BlitRenderBuffer` four times in different locations to create a tiled pattern.

* `BlitRenderBuffer` is created for blit destination. Its dimensions must match source dimensions.
* `BlitPass` with `DuckRenderBuffer` as Source Render Buffer and `BlitRenderBuffer` as Target Render Buffer is created.
* Note that source and destination render buffers must have same dimensions and format. They must be different render buffers.
* Buffer Offset and Blitting Region dimensions specify the rectangular area to blit.
* `BlitPass` RenderOrder is set to 1: it must be executed after `DuckRenderPass`.

After creating and setting up the first `BlitPass`, Duplicate (Ctrl+D) command can be used to create three more copies. Blit regions are then specified individually.

![](./docs/blit_pass.png)

Note that all `BlitPasses` have same RenderOrder. Ramses Composer informs user that their execution order is undefined (as seen on screenshot). In this example, the execution order among `BlitPasses` does not matter: there is no mutual dependency between them.

## Displaying offscreen buffers contents

Finally, to visualize contents of both blit source, `DuckRenderBuffer` and destination, `BlitRenderBuffer`, they are set as texture uniforms `u_Tex` in the Private Material of left and right `QuadMeshNodes`. Note that `u_FlipUV` attribute is set to 1, because the `Quad` mesh has V coordinate flipped.

`MainRenderLayer` Renderable Tags are set to `render_main` (for rendering quads) and `duck` to display original duck mesh side view rendered with default `PerspectiveCamera`.

`MainRenderPass` Render Order is set to 3: it is must be executed last, after `DuckRenderPass` and all `BlitPasses`, because it depends on their output render buffers.
