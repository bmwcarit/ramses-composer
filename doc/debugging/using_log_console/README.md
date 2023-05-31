<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer-docs).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->
# Using the Log Output Console

The GUI version of Ramses Composer comes with a launch setting that activates a separate console. That console is the log window. It displays Ramses-internal, Ramses Logic-internal and Ramses Composer-internal log messages. This allows supplementary information, warnings and issues to be shown instantly.

Note that Ramses Composer-specific logging output is also written to a file ```[Ramses Composer root folder]/configfiles/RamsesComposer.log```, regardless of whether the log window has been activated.


## How to Activate the Log Output Console

Launch the GUI version of Ramses Composer with the command argument ```-c```.

Two new windows will be opened: The actual Ramses Composer window, and a console window with contents that look similar to this:

![](docs/new_log_console.png)

This is the log output window that currently shows that Ramses has initialized a scene - makes sense, since we created a new scene upon launching Ramses Composer!
