<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer-docs).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->

# Data and scope

The Ramses Composer is a tool which controls the state of a 3D project before the [export step](../export/README.md) to binary Ramses formats.
It is important to understand the lifecycle and origin of Composer objects and their data in order to use them efficiently, reduce unneeded
duplication of resources, and store the data in a way that it can be configured for different use-cases and export scenarios.
This section of the documentation explains when data is copied and when referenced, and how objects behave in the different categories
of content (scene, resources, prefabs, and external project references).

This manual page is intended for advanced usage of the Composer.
For introduction to the individual features, have a look at the dedicated examples:

* [Hello World](../hello_world/README.md) - fundamentals of references to external resources
* [Monkey heads](../monkey/README.md#lua-scripting) - links between properties inside the project
* [Prefabs](../prefabs/README.md) - packaging of data in reusable components
* [External references](../../advanced/external_references/README.md) - splitting projects into reusable modules which can reference each other

## Composer data and imported resources

The Composer has its own data model which it stores in a single file - the \<project\>.rca file. Data which is imported from external sources via a file URI
is not duplicated internally, but instead kept as a reference (to the originating file) as a relative path. Therefore, an `rca` file should be
always distributed together with the imported files at the correct location specified in the URIs. It is strongly advised to keep these files in subfolders of the directory where the
`rca` file lives in order to make the project portable, i.e. be able to move and archive the whole folder for easy distribution and versioning.

For options to use absolute paths and best practices for project structure, refer to
[the dedicated section](../../advanced/best_practices/README.md#relative-and-absolute-paths).

## Composer data and exported binary files

The Composer currently exports two binary files - a `ramses` file (contains the exported Ramses scene) and a
`rlogic` file (contains the exported Ramses Logic content and references the Ramses scene).
All references to non-native Ramses objects are resolved to native Ramses content on export.
This includes external resources (shaders, PNG, glTF, etc), prefabs and external project references.

During the export, the Ramses scene is exported in the same state as it is in the Composer at the time of export.
Refer to the [export manual](../export/README.md)
for details how to use the export functionality. See the [section below](#mapping-to-ramses-objects) for
details which data is exported and which objects to expect.

## Scene Graph objects

The Scene Graph tree in the Composer represents in broad terms the Ramses scene which will be
created on `export`. Each object there corresponds to one or more exported Ramses objects. See
the [section below](#mapping-to-ramses-objects) for details on the exact mapping.

All objects in the Composer Scene Graph have their own data. The exceptions to this rule are:

* Links - if two properties are linked to the same output - the link will make sure they receive the same value
* Materials - `MeshNode` objects contain a reference to a material, but the `MeshNode`s' local material settings are independent from the default settings of the material

<!-- TODO rework this once we fix materials behavior... The above sentence can not be generalized - if two meshes refer to the
same material, they get their own copy of the data, but if they refer to the same mesh - they share the mesh. This makes
sense if one knows the Ramses API, but it doesn't make much sense from a user/data point of view. The UI is also particularly
confusing - it appears as if the material is being referenced (i.e. the data is somewhere else) but it's not-->

## Copy and Deep Copy

Any object in the Composer can be copied and pasted. By default, a copy is "shallow", i.e. it does not recursively copy references to other objects.
If you want to do a "deep copy", you have to use the "Copy (Deep)/Cut (Deep)" function available via right click. See the sections below for details.

The copy function appends a suffix of the form `<orig. name> (N)` to the name of pasted objects when a name conflict occurs - both for shallow and deep copies.

### Shallow copy

When an object is shallow copied, all its values will be copied. This also includes references to other objects in the scene (e.g. MeshNode -> Mesh), so pasted objects share the same references as the copied objects. When the referenced object does not exist in the scene anymore, the reference will be set to `<empty>`.

### Deep copy

When deep-copied, other objects referenced in the copied objects will also get copied. When pasting, the references in the copied objects will be replaced with references to the newly created reference objects. Note that deep-copying an object with links will not create a copy of the linked LuaScript when pasting, but copies the links. Also note that deep-copy-pasting PrefabInstances will generate new copies of the referenced Prefab, but not vice versa.

<!-- TODO document how this affects ext refs -->

## Mapping to Ramses objects

Here is a table which describes which Ramses (and Logic) objects are generated for each of the
Composer objects:

| Composer type     |From View          |Exported Ramses object(s)                              |Exported Logic object(s)   |Notes |
|-------------------|-------------------|-------------------------------------------------------|---------------------------|-------------------------------------|
|Node               |Scene              |ramses::Node                       | rlogic::RamsesNodeBinding | Child nodes are assigned as children in Ramses. RamsesNodeBinding points to Ramses node. |
|MeshNode           |Scene              |ramses::MeshNode ramses::Appearance ramses::GeometryBinding| rlogic::RamsesNodeBinding rlogic::RamsesAppearanceBinding| Same as Node, but with Appearance and GeometryBinding which refers to array resources (see Mesh) |
|PerspectiveCamera  |Scene              |ramses::PerspectiveCamera                              | rlogic::RamsesCameraBinding | Are assigned by the user to render passes |
|OrthographicCamera |Scene              |ramses::OrthographicCamera                             | rlogic::RamsesCameraBinding | Are assigned by the user to render passes |
|Material           |Resources          |ramses::Effect                                         |                           | Holds the Effect, not the appearance and uniform values (see MeshNode) |
|Mesh               |Resources          |ramses::ArrayResource                                  |                           | Holds geometry data referenced by ramses::MeshNode's ramses::GeometryBinding |
|Texture            |Resources          |ramses::Texture2D  ramses::TextureSampler              |                           | Currently static |
|CubeMap            |Resources          |ramses::TextureCube  ramses::TextureSampler            |                           | Currently static |
|LuaScript          |Scene              |                                                       | rlogic::LuaScript         |                                                                     |
|Animation          |Scene              |                                                       | rlogic::AnimationNode     |                                                                     |
|AnimationChannel   |Resources          |                                                       | 2 or 4 rlogic::DataArrays | The number of rlogic::DataArrays depends on the interpolation type - e.g. cubic interpolation requires in/out tangents in addition to keyframes |
|PrefabInstance     |Scene              | Various                                               | Various                   | Exported content depends on referenced Prefab. Each PrefabInstance creates its own copy based on Prefab contents.  |
|Prefab             |Prefab             |                                                       |                           | Content created only if referenced by a PrefabInstance. Underlying nodes and scripts are exported as if they had their own Scene Graph. Ramses nodes are parented to the parent node of the corresponding PrefabInstance |
|RenderBuffer       |Resources          | ramses::RenderBuffer                                  |                           | A direct mapping to the Ramses render buffer. |
|RenderLayer        |Resources          | ramses::RenderGroup                                   |                           | Each render layer creates a ramses::RenderGroup. The renderables in a render group are determined by tags. The order of the renderables is either given by manually specifying the order index for each tag, or given by the scene graph order. |
|RenderPass         |Resources          | ramses::RenderPass                                    |                           | A direct mapping to the Ramses render pass. |
|RenderTarget       |Resources          | ramses::RenderTarget                                  |                           | A direct mapping to the Ramses render target. |
|BlitPass           |Resources          | ramses::BlitPass                                      |                           | A direct mapping to the Ramses blit pass. |


## Additional note on prefabs

Prefabs don't export any objects unless used in a PrefabInstance.

Furthermore, `Prefab(Instance)` scene content is instantiated (copied) on all places where a `PrefabInstance` is attached in the scene. The content copies receive a prefix in the name which denotes which `PrefabInstance` created them. This excludes Resources like Textures, Vertex arrays and Effects - they are exported once even if used by multiple `MeshNodes/PrefabInstances`.

## Default Resources

Ramses Composer contains default resources, serving as placeholder resources. A default resource used in a scene will also get exported.
Multiple objects share the same default resource, independent of whether this object is in the current project or imported from a
different project as an external reference or otherwise. This is done to prevent unnecessary default resource duplicates upon exporting
the scene.

### CubeMap

The default CubeMap shows fallback textures on all sides.

### Meshes

The default Mesh in RaCo is a simple cube.

### Material

There are two default Materials.

Using a Mesh that does not contain normals will result in the first default Material: a single matte color with the RGBA values [1.0, 0.0, 0.2, 1.0] that spans the entire Mesh.

Using a Mesh that contains normals but no Material will result in the second default Material: an orange color with the RGBA values [1.0, 0.5, 0.0, 1.0] and surface reflection.

### Texture

The default Texture in RaCo consists of a singular "test card" image that shows the UV coordinate roots and what conventions for these coordinates' origin are being used (currently OpenGL or DirectX).
