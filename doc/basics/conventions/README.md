<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer-docs).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->

# Conventions

The Ramses Composer is designed to be generic and explicit in all of its features. It does not assume things like metric units, polygon winding/facing, coordinate systems, and does not enforce a material system. It gives complete freedom to the user to choose such conventions - and to be explicit about the state of the 3D project. All exceptions to this design philosophy are listed in this page.

## Coordinate systems

The Ramses Composer uses the same conventions as Ramses (and therefore as OpenGL). Those are as follows:

- Right-handed coordinate system (of the [semantic uniforms](#semantic-uniforms))
- Y is the up axis (of the default camera)
- -Z is the look direction (of the default camera)
- Depth range is [0, 1] (standard in OpenGL)
- Clip space X, Y: [-1, +1] and Z: [0, -1]
- Images imported as per GLTF 2.0 standard
    - bottom-left pixel corresponds to UV [0, 0]
    - top-left pixel corresponds to UV [0, 1]
- All rotations are Euler X-Y-Z rotations (to avoid Gimbal lock, use multiple nodes with single-axis rotation)

Texture origin can be selected (top-left or bottom-left). The default setting is to load textures "bottom-to-top", i.e. bottom texture row corresponds to UV(0, 0).

If you want to use different conventions - you can do so, by using different math in the GLSL shaders. Keep in mind that you can't use the automatic matrix generation of Ramses in this case.

## Blender's glTF exporter and the +Y axis

Blender's glTF exporter has a toggle named `+Y up` which may mislead that when activated, the exported file will have the standard OpenGL convention of `Y is up`:

![](./plus_Y_up.png)

However, when selecting this option, the exporter switches all transformation axes
so that Z becomes Y. Don't use this toggle, unless you know what you are doing!

## Rendering order

The rendering order corresponds to the order in which MeshNodes appear in the Scene Graph view if uncollapsed: top-to-bottom, depth-first. In other words, parents are rendered before *own* children, and siblings are rendered in the order in which they appear under their parent. Note also that render order only affects MeshNodes - they are the only entity that gets rendered.

## Attributes

The Ramses Composer's primary import format is GLTF 2.0. It is quite flexible and allows transmission of mesh attributes in a portable way ([see specification](https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#reference-mesh)) which can be used in custom GLSL shaders. In order to use them, you have to create attributes with a specific name. The following table shows the mapping between mesh attributes provided by GLTF and the names and types the Composer expects for them:


|GLTF Name   |Accessor Type(s)    |Description|Attribute name in custom GLSL shaders|
|----------  |--------------------|-----------|-------------------------------------|
|`POSITION`  |`"VEC3"`            |XYZ vertex positions|a_Position|
|`NORMAL`    |`"VEC3"`            |Normalized XYZ vertex normals|a_Normal|
|`TANGENT`   |`"VEC4"`            |XYZW vertex tangents where the *w* component is a sign value (-1 or +1) indicating handedness of the tangent basis|a_Tangent|
|`TEXCOORD_0`|`"VEC2"`            |UV texture coordinates for the first set|a_TextureCoordinate|
|`COLOR_0`   |`"VEC3"`<br>`"VEC4"`|RGB or RGBA vertex color|a_Color|
|`JOINTS_0`  | `"VEC4"`           |Joint node indices for skinning | a_Joints0 |
|`WEIGHTS_0` | `"VEC4"`           | Joint node weights for skinning | a_Weights0 |

Multiple color or UV attributes appear according to the scheme a_Color, a_Color1, a_Color2, etc.

Morph target attribues for the position and normal attributed will be imported with `_Morph_0`, `_Morph_1`, etc appended.

Multiple joint and weight attributes are numbered starting with 0: a_Joints0, a_Joints1, a_Joints2, etc.

<!--
TODO
Where do the a_Bitangent come from?
Also: check tangent types
-->

## Struct and Array Uniforms

Ramses Composer supports struct, scalar array and struct array uniforms.

Scalar array uniform:

```
uniform float weights[2];
```

![](./float_array_uniform.png)

Scalar array uniform can be linked with a single link for the entire entity or with links for individual items.

Struct uniform:

```
uniform struct Light
{
	vec3 eyePosOrDir;
	bool isDirectional;
	vec3 intensity;
	float attenuation;
} uLight;
```

![](./struct_uniform.png)

Struct array uniform:

```
struct Point2D {
	float x;
	float y;
};
uniform Point2D uPoints[2];
```

![](./struct_array_uniform.png)

Struct and struct array uniforms are flattened by Ramses into set of properties. Each element of uniform is represented as individual property in Ramses Composer:

* `struct.member` for struct element
* `struct[index].member` for struct array element

This means struct and struct array uniforms can't be linked with a single link for the entire entity. They should be linked as individual properties.

## Semantic Uniforms

There are few special semantic uniforms (i.e. uniforms which can't be explicitly set, but receive their values from Ramses). These are:


| Ramses Enumeration                 | Type    | Uniform name in custom GLSL shaders |
| -----------------------------------|---------|:-------------:|
| ```MODEL_MATRIX```                 | `MAT44` | u_MMatrix / uWorldMatrix |
| ```MODEL_VIEW_MATRIX```            | `MAT44` | u_MVMatrix / uWorldViewMatrix |
| ```MODEL_VIEW_PROJECTION_MATRIX``` | `MAT44` | u_MVPMatrix / uWorldViewProjectionMatrix |
| ```PROJECTION_MATRIX```            | `MAT44` | u_PMatrix / uProjectionMatrix |
| ```VIEW_MATRIX```                  | `MAT44` | u_VMatrix / uViewMatrix |
| ```NORMAL_MATRIX```                | `MAT44` | u_NMatrix / uNormalMatrix |
| ```CAMERA_WORLD_POSITION```        | `VEC3`  | u_CameraWorldPosition / uCameraPosition |
| ```RESOLUTION```                   | `VEC2`  | u_resolution / uResolution |

If any of these uniforms are found in a shader, they will not show up in the Property view of MeshNodes and Materials, but they will receive their value from Ramses.

In addition the `u_jointMat` uniform of private materials of MeshNodes will be set by Skin objects referencing the MeshNode. It has to be an array of type `VEC4` of the same length as the number of joints nodes in the Skin object.

## Shader Includes
Ramses Composer supports `#include` directives in shaders. The path must be relative to current source file. Shaders with `#include` directive are preprocessed by Ramses Composer and then sent to Ramses. Shader errors show locations in preprocessed file. Shader tooltips show preprocessor errors.

Example:

```
// Shader source file
#include "offset.inc"
#include "../some_relative_path/file.txt"
```

## Texture Loading and Conversion
Ramses Composer currently supports PNG images with 8 or 16 bit color depth.
The number of color channels can vary from 1 for greyscale to 4 for RGBA images.

The user can change the color/alpha channels seen by the shader using the `format` property of Texture objects. 
The loaded texture data is internally converted to a format of the Ramses data buffer containing only the channels needed by the specified format. The remaining channels are set up to replicate channels and/or fill them with constant 0/1 values as needed by the specified format.

The conversions performed can be seen in the property browser of Texture objects in an info box
showing the color channels present in the file, the channels in the Ramses data buffer, and the values seen by the shader.

For convenience the table below lists the conversions as shown by the info box for
all possible combinations of supported 8bit png file formats and valid format values.
The ramses and shader columns show the color channels in the Ramses data buffer and the channels as seen by the shader
with constant values shown as 0 or 1.

| File		| Format	| Ramses 	        | Shader		| 
| --------- | ---------	| ----------------- | ------------- | 
| RGB		| RGBA		| RGB				| RGB1			| 		  
| 			| RGB		| RGB				| RGB1			| 		  
| 			| RG		| RG				| RG01			| 		  
| 			| R			| R					| R001			| 		  
| RGBA		| RGBA		| RGBA				| RGBA			| 		  
| 			| RGB		| RGB				| RGB1			| 		  
| 			| RG		| RG				| RG01			| 		  
| 			| R			| R					| R001			| 
| R			| RGBA		| R					| RRR1			|
|			| RGB		| R					| RRR1			| 
| 			| RG		| R					| RR01			| 
| 			| R			| R					| R001			|
| RG		| RGBA		| RG				| RRRG			| 
|			| RGB		| R					| RRR1			| 
| 			| RG		| RG				| RG01			|
| 			| R			| R					| R001			|
