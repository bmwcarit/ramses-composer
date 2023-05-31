<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer-docs).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->
# Exporting to Ramses

The Ramses Composer is holding its data in textual form optimized for version control, flexibility, and used as a base for collaborative work. In order to achieve maximum loading and runtime performance though, the Composer exports its scene to the native binary formats of `Ramses` and the `Ramses Logic Engine` - with typical extensions of .ramses and .rlogic respectively.

This chapter shows how to export your Ramses Composer project.

## Exporting in the GUI

On the menu bar of the GUI, click on "File", then in the context menu, click on "Export...".

The Export Dialog will open:

![](./docs/export_dialog_nice.png)

This dialog shows you all export settings as well as an overview of all created Ramses and Ramses Logic objects that will be exported. You also have the possibility to see warnings and errors found by the Ramses scene validator. The validation check output can be viewed in the "Warnings"/"Errors" tab of the Export Dialog "Summary" and will be performed every time you open the Export Dialog.

Please note that if this validation detected an error, the Export Dialog summary will be switched to a new "Errors" tab, warnings will not be displayed and exporting will be prohibited:

![](./docs/export_dialog_error.png)

The Export Configuration specifies the exported file paths and whether the Ramses scene should be compressed before exporting.

The "export path" will be set to the origin folder of the project file, or - if no project is loaded - the origin folder of the Ramses Composer executable. This path must not be relative. The "ramses file" and "logic file" are relative paths for the exported Ramses and Ramses Logic files, starting from the export path. The "compress" option specifies whether the Ramses scene resources should be compressed before exporting.
Please refer to the Ramses documentation for details in which cases compression makes sense - the general rule of thumb is that if you are not using resources in specialized compression formats (e.g. ASTC, ETC2, Draco...) you should use the Ramses LZ4 compression to keep the binary files as small as possible.
After everything has been set up, click on the "Export" button on the lower right of the Export Dialog and presto - the export should be done and two new files have been created!

Please note that files will be overwritten without asking when exporting to already existing files.


## Exporting in the headless version

With the headless version, you can export a project [using the same settings described in the GUI sub-chapter](#exporting-in-the-gui) with just one command line.
Currently there is no validation step when exporting from the command line, in case of scene errors it will fail silently.

This line should do the trick:

```[path to RaCoHeadless binary] -p [.rca project path] -e [binary ramses path] -c```

```-p``` will load the specified project file.

```-e``` will export the loaded project to two separate files ```[binary ramses path].ramses``` and ```[binary ramses path].rlogic```. The file path can be absolute or relative. Please note that files will be overwritten without asking when exporting to already existing files.

```-c``` will compress the Ramses scene resources. Omit this parameter to not compress them.

For an overview over more command line options, you can launch the RaCoHeadless binary with the ```--help``` parameter.
