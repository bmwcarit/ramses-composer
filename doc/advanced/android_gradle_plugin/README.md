<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer-docs).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->
# Automatic binary generation

This tutorial explains how to improve your workflow when designing Android apps with assets produced
in the Ramses Composer.

This tutorial extends the [basic Android tutorial](../../basics/android_app/README.md).

## Typical artist workflow

The Ramses toolchain is primarily aimed at a subgroup of graphics developers called `technical artists`. The typical workflow of a technical artist is similar to that of a full-stack developer. Changes and improvements are typically done iteratively, at small increments, and vertical in nature.

In order to provide the most optimal workflow for technical artists, the Ramses toolchain provides
a special Gradle plugin for Android. The plugin connects the development/application workflow with the
design/composition workflow. Whenever you change the original Ramses Composer scene, the plugin
will pick it up, rebuild your app and integrate the latest/current version of the asset there.

## Add the plugin

You can add enable the plugin in your top-level build file using standard Gradle syntax (Gradle 7.2):

```groovy
// <root>/build.gradle
plugins {
    id 'io.github.bmwcarit.RaCoPlugin' version '0.3.1' apply false
}

```

You can now enable it in all subprojects which have Ramses assets:


```groovy
// E.g. <root>/app/build.gradle
plugins {
    id 'io.github.bmwcarit.RaCoPlugin'
}
```

Finally, you can configure the plugin to track asset(s) and recreate them any time the original
\<scene\>.rca file has been saved or modified:

```groovy
raCoConfig {
    // A path on your local machine where you downloaded the Ramses Composer
    raCoHeadlessPath = '/home/user/Downloads/tools/RamsesComposer-1-0-3/bin/RelWithDebInfo/RaCoHeadless.sh'
    // A list of tuples where the first element is a RaCo project file, the second one is the output filename
    // Note that the output filename does not include the file extensions (.ramses/.rlogic)
    // In this example, the plugin will generate two files:
    // - app/src/main/assets/monkey.ramses
    // - app/src/main/assets/monkey.rlogic
    inputs = [['../../monkey/monkey.rca', 'app/src/main/assets/monkey']]
}

// Tells Gradle to conditionally re-export assets before building the project
preBuild.dependsOn RaCoExport
```

Note that the RaCoHeadless path has to be a global path on your development machine which contains the
shell script to execute RaCoHeadless (the command-line version of Ramses Composer). The inputs can be global or local, but it's preferred to always have the source asset and the binary asset as relative paths
in your repository.

## How it works

With the setup from above, you can now edit your scene in a separate Ramses Composer GUI window, while
re-building the app and e.g. testing it on a device/emulator. Any time the binary assets are deleted, or
the source project is modified, the plugin will automatically re-export the assets.
