<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer-docs).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->
# Vertex Skinning
*You can find the example project {{ '[here]({}/doc/advanced/skinning)'.format(repo) }}.*

Here we discuss how to import and setup skinned meshes from glTF files.


## Introduction to vertex skinning 

Vertex skinning allows to deform a mesh based on the pose of a skeleton represented by a node hierarchy. The skeleton influences the transformations of a separate meshnode which is the actually rendered object. The calculation of the position of the deformed vertices is performed in the vertex shader which needs additional information on the pose of the skeleton nodes and on how the skeleton pose should influence each vertex. The pose information is fed to the shader as an array of the transformation matrices of the individual skeleton nodes. The mesh in the glTF file itself contains additional attribute arrays which specify the set of skeleton nodes influencing each vertex and their weights. The shader can then assemble a dynamic model transformation matrix and use this instead of the normally use model matrix uniform to calculate the world position of a vertex. Dynamic changes of the pose of the skeleton will now lead to a deformation in the controlled mesh. 

More details about vertex skinning support in glTF can be found in the [glTF skinning tutorial](https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_020_Skins.md).

Skinned meshes are supported by the glTF format and can be imported into RamsesComposer using the glTF import dialog. To tie together the skeleton node hierarchy and the controlled meshnode a new Skin object type is available which will be automatically set up by the glTF import.

The following sections will describe the individual steps needed to setup a skinned mesh in RamsesComposer.

Note that skinning is supported only at feature levels 4 or greater. At lower feature levels the glTF import dialog will not show any skin in the file and Skin objects can't be created.

## Importing from glTF

The first step is the import of the actual glTF file which is performed using the glTF import dialog.

The dialog shows Skin objects in addition to the familiar nodes, meshnodes, and meshes. The Skin objects are needed to control the skinning so they should be selected if you want to use skinning.

![](./docs/import-dialog.png)

The import places the additionally created Skin objects in the scene graph:

![](./docs/scenegraph-with-skin.png)

The Mesh object imported shows additional `a_Joints0` and `a_Weights0` attributes. These describe the influencing skeleton nodes and their weights and will be used in the shader later.

![](./docs/mesh-property-browser.png)

The Skin object itself contains a reference to the controlled meshnode and a list of the nodes in the skeleton node hierarchy that control the deformation. In addition the skin inside the glTF file itself needs to be referenced by a uri and a skin index. The data needed from the glTF file is an additional array of inverse transformation matrices which are needed in addition to the dynamic transformation matrices of the skeleton nodes to calculate the correct world space transformation matrices fed to the shader.

When importing meshes using multiple materials several MeshNodes will be created. The Skin object will then contain a list of all these MeshNodes as targets.

To control the meshnode deformation via the shader the target meshnode set in the Skin object needs to have a private material with the correct uniform for the transformation matrices as described in detail below. The Skin object will therefore still show an error directly after the import:

![](./docs/skin-property-browser.png)


## Setting up the shader

As a starting point for setting up a shader for a skinned mesh a template shader is included in the RamsesComposer distribution in the `resources/shaders/` subfolder. The fragment shader can look like this for example:

```
#version 300 es

precision mediump float;

in vec3 a_Position;

uniform mat4 u_VMatrix;
uniform mat4 u_PMatrix;

in vec4 a_Joints0;
in vec4 a_Weights0;

// Adjust array size to actual number of joints
uniform mat4 u_jointMat[2];

void main() {
	mat4 skinMat = 
		a_Weights0.x * u_jointMat[int(a_Joints0.x)] +
		a_Weights0.y * u_jointMat[int(a_Joints0.y)] +
		a_Weights0.z * u_jointMat[int(a_Joints0.z)] +
		a_Weights0.w * u_jointMat[int(a_Joints0.w)];

    gl_Position = u_PMatrix * u_VMatrix * skinMat * vec4(a_Position, 1.0);
}
```

This shader needs the joints and weights attributes from the mesh and the transformation matrices for the skeleton nodes. The uniform names for the joints and weights attributes are chosen by convention as `a_Joints0` and `a_Weights0`. By convention the uniform name for the transformation matrices is `u_jointMat`. This must be and array of type mat4 whose length matches the number of joint node references in the Skin object.

Both joints and weights are vec4 types which allows for at most 4 skeleton nodes to control each individual vertex. Of course different vertices can be controlled by different sets of skeleton nodes. The joints are indices into the transformation matrix array while the weights specify the respective weight of each transformation matrix. The weighted transformation matrices are then combined in the shader to obtain the world transformation matrix which is used instead of the normal uniform model matrix `u_MMatrix`.

Note that Skin objects with a different number of skeleton nodes need separate shaders since the number of the skeleton nodes determines the length of the `u_jointMat` uniform array.

Since the fragment shader does not need special modifications to support skinning this completes the setup of the shader itself.

The shader has now to be used in the meshnode and the private material flag enabled. We will further set a nice color to actually see the mesh:

![](./docs/meshnode-property-browser.png)

At this point the Skin will not show an error message anymore and the basic setup is complete. The preview will now show the non-deformed mesh:

![](./docs/preview-1.png)


It is now possible to deform the meshnode via changes in the position or orientation of the skeleton node hierarchy.


## Control via animations

The example glTF file used already contains an animation which controls the rotation of the `node_2` in the skeleton node hierarchy via a link:

![](./docs/node-property-browser.png)

To control the deformation we change the `Progress` input property of the animation instead:

![](./docs/animation-property-browser.png)

And now the preview shows a deformed mesh:

![](./docs/preview-2.png)


## Multiple joints/weight sets

Since the joints and weights uniforms are of type `vec4` in the shader at most 4 skeleton nodes can influence each vertex. The mesh may contain multiple joint/weight attribute sets however which allows to exceed this limit. The shader will then need to use the additional joint/weight attributes to calculate the vertex model transformation matrix.
