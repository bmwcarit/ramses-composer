<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/GENIVI/ramses-composer-docs).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->

# Render Pipeline

## Render passes

Rendering in Ramses scenes starts with render passes. All render passes in the scene are rendered in the ascending order given by their "Order" property. 
Each render pass renders all mesh nodes gathered by the render layers it references, using the camera set in the "Camera" property and renders into the render buffers
defined by the render target set in the "Target" property.

Render passes can be en-/disabled, a disabled render pass is not rendered.

## Render targets

Render targets in Ramses define a set of render buffers which are used during rendering by a render pass. A render target is essentially just a collection of
render buffers. There are a few constraints how render buffers can be associated with a render target:

* All render buffers must have the same size.
* Only one render buffer can be a depth/stencil buffer, the rest must be color buffers.
* A render target must have a valid first render buffer.
* If several render buffers are set, there cannot be empty slots or invalid render buffers between them.
* Each render buffer can only be added once to a render target.

All of these constraints are checked by Ramses Composer and a scene error is created if at least one of them is not fulfilled.

## Render buffers

Render buffers in Ramses are used to define the buffers a render pass can render to, and which can later be used in samplers in shaders. 
A render buffer can be set to a given resolution and format, and its sampling parameters can be defined.

## Render layers

Render layers are used to gather mesh nodes which must be rendered with a render pass. A render pass references render layers,
and the render layers determine mesh nodes and other render layer which are used to render them.

Which mesh nodes and other render layers are part of a given render layer is determined by a tag system:

* Each node can be tagged with any number of tags using its "Tags" property.
* If a mesh node tag matches one of the tags in the render layer "Renderable Tags" property, the mesh node is rendered with the render layer.
    * The mesh node must also match the material filter to be rendered, see below.
* If a node tag matches one of the tags in the render layer "Renderable Tags" property, 
  all mesh nodes in the scenegraph branch rooted by the tagged node are rendered with the render layer.
    * The mesh nodes must also match the material filter to be rendered, see below.
* The rendered mesh nodes in a render layer can further be filtered using the material filter:
    * If "Material Filter Behaviour" is set to "Exclude materials with any of the listed tags", the render layer will only render mesh nodes
      which **do not** reference a material tagged with one of the tags in "Material Filter Tags".
    * If "Material Filter Behaviour" is set to "Include materials with any of the listed tags", the render layer will only render mesh nodes
      which **do** reference a material tagged with one of the tags in "Material Filter Tags".
    * Given that often render passes expect to render mesh nodes with certain materials, the material filter can be used to easily
      render scene graphs with many mesh nodes by just tagging the root node and the materials.
* Each render layer can also be tagged with any number of tags using its "Tags" property. Similar to the nodes, render layer A is part
  of render layer B if one of the tags in the "Tags" property of A matches one of the tags in Bs "Renderable Tags" property.
    * It is not allowed to add a render layer to itself (directly or indirectly).
* Depending on the setting of the "Material Filter Mode" property the render order within a render layer is defined either by the order index given by the tag referencing the mesh node or render layer or by the scene graph order of the mesh nodes.
    * It is an error if the same mesh node is added twice to the same render layer with different order indices.
    * Ramses Composer will generate a warning if a render layer is rendered by another render layer which is using scene graph order,
      as the order between the referenced render layer and the referenced mesh nodes is unclear.
