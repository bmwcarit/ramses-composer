<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer-docs).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->
# Best Practices

This chapter introduces best practices that will help you make your Ramses Composer workflow as smooth as possible.


## Folder Layout

As of now, saving a Ramses Composer project will not generate or move any supplementary folders, e.g. for external files. You will have to create and maintain a project folder structure yourself.

We recommend a singular tree structure with the Ramses Composer project file in the project root folder. External files for resources, such as images, scripts and shaders, are saved in separate folders under the project root folder; they can contain sub-folders that refine and categorize resource semantics.

A project structure can look like this for example:

```
MyCoolProject/
    ├ main.rca
    ├ Images/
        ├ Animal/
            ├ AligatorEyes.png
            └ CrocodileEyes.png
        ├ Swamp/
            ├ SwampReflection.png
            └ SwampDirt.png
    ├ Lua/
        ├ biteScript.lua
        ├ feetMovementScript.lua
        └ waterScript.lua
    ├ Mesh/
        ├ Aligator.gltf
        └ Crocodile.gltf
    └ Shader/
        ├ Leather/
            ├ leather.frag
            └ leather.vert
        └ Swamp/
            ├ swamp.frag
            └ swamp.vert
```

For multiple projects there are more possibilities, all according to your needs. One way is to treat each project folder as a "module" inside another folder, while each project maintains their singular tree structure. This also means that each project could contain duplicate resources.


## Relative and Absolute Paths

For every file path-based property in Ramses Composer (such as URIs for external files), Ramses Composer offers two approaches: absolute paths and relative paths. Every path-based property has an absolute/relative flag. By default, this flag is set to relative. However, this flag can be changed by either inserting an absolute/relative path or right-clicking on the property and clicking on the "Use Absolute Flag" flag in the context menu (This also changes the path itself):

![](docs/absolute_relative_option.png)

Paths that are on different partitions (e.g. different hard drive or network drive) will be forced to absolute and there will be no possibility of changing them to relative.

Saving a project to a different place on the file system in Ramses Composer does not move any external files referenced in the project. However, every relative path inside the project gets updated so that references to these external files still persist. If the project gets saved to a different partition, these relative paths will be automatically converted to absolute. They will not be converted back to relative after saving the project back to the original hard drive partition.

Moving the project file itself to a different place in your file manager will not update any paths. References to external files via relative paths might break. For successful moving, the resources themselves should be moved together with the project file while keeping the original hierarchy, most suitably [in a nice singular file tree layout](#folder-layout).

Absolute paths will not be affected by either action.

### Should I Use Absolute or Relative Paths?

Use relative paths whenever possible. As described earlier, relative paths make external dependencies more robust and portable.

Absolute paths should only be used as a last resort. They are more prone to breaking external dependencies, depending on the resource file path (for local files) or network settings (for network drive files).

For a nifty alternative to absolute paths, try using relative paths to
[external references](../external_references/README.md) in local project copies.


## File Version Handling

Every Ramses Composer project file contains a file version. If a new Ramses Composer release version contains a feature/bugfix that modifies the internal structure of the project file, this file version is increased. The reason for this is to implement smooth migration of old Ramses Composer projects to the newest version.

Loading a project with an older version in a newer Ramses Composer version is allowed. The file version migration is designed to automatically update your loaded data in Ramses Composer while keeping it intact. When you save the project afterwards, the data and file version in the project file itself will be overridden with the updated state.

Loading a project with a newer version in an older Ramses Composer version is not possible. By design, Ramses Composer will show a runtime error message that the file version is too high and the project will remain unloaded. Instead, a new project will be created.

We strongly advise against circumventing Ramses Composer's file version check, as it may cause data corruption and crashes. We also suggest storing a backup of the project file before migrating to a new version, ideally in a VCS tool like Git.
