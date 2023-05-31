<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/COVESA/ramses-composer-docs).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->

# TracePlayer

The Ramses Composer contains a built-in trace player, which allows to play back RaCo trace (.rctrace) files.
Good applications of the TracePlayer include:
* re-playing captured real-world traces to debug exotic issues
* looping predefined sequence of scene states, for demos and scene validation purposes

## Basic usage

The TracePlayer has the usual playback features (Load, Play, Pause, Stop, toggle Loop mode, Step-forward/backward, and Jump-to).
![](docs/traceplayer_notrace.png "TracePlayer - No Trace Loaded")

First, browse to a valid `.rctrace` file and load it using the 3-dots icon (*this article includes an [example](#example-bmw-x5) based on the BMW X5 example model*).
![](docs/traceplayer_init.png "TracePlayer - Initial State").

The TracePlayer parses the `.rctrace` file, then updates properties of relevant Lua nodes sequentially in the scene based on their names. Therefore, you need to make sure to add the equivalent interface Lua scripts to the scene and match their names.
Furthermore, **IN** interfaces of the Lua script have to match the same structure on the properties in the `.rctrace` file.

## The .rctrace File Format

The trace file is a JSON file and must contain an array of frames, where each frame is a JSON object containing two nested objects: "SceneData" and "TracePlayerData".
**SceneData** can contain a list of features; those features are parsed and mapped by name by TracePlayer to the corresponding Lua nodes in the scene. The supported property types are boolean, integer, double, and strings.
**TracePlayerData** must contain the timestamp of a frame in milliseconds.

This is how a valid trace can look like:

```json
[
    {   // frame#1
        "SceneData" :
        {
            "Feature_1" :    // TracePlayer will look the scene up for a Lua node with the name "Feature_1". It shall have IN interface structs named "Function_1" and "Function_2"
            {
                "Function_1" :  //  "Function_1" struct shall contain "active", "ranking", "speed", and "color" IN properties with equivalent types
                {
                    "active" : true,    // BOOL
                    "ranking" : 7,      // INT
                    "speed" : 123.50,   // FLOAT
                    "color" : "blue"    // STRING
                },
                "Function_2" :
                {
                    "length" : 12.0,
                    "width" : 5.50,
                }
            },
            "Feature_2" :
            {
                "Function_1" :
                {
                    "class" : 3
                },
                "Function_2" :
                {
                    "positionX" : 1.789,
                    "positionY" : 45.887,
                }
            }
        },
        "TracePlayerData" :
        {
            "timestamp(ms)" : 1
        }
    },
    {   // frame#2
        "SceneData" :
        {
            /* list of features */
        },
        "TracePlayerData" :
        {
            "timestamp(ms)" : 2
        }
    },
    {   // frame#3
        "SceneData" :
        {
            /* list of features */
        },
        "TracePlayerData" :
        {
            "timestamp(ms)" : 3
        }
    }
]
```

## TracePlayerData

The TracePlayer offers the following property in the **TracePlayerData** object:
- **timestamp_milli**: an integer of the playback actual time in milliseconds, which can be used as the reference timer for animation.

## Example (BMW X5)

This article includes a sample trace file based on the BMW X5 demo model. You can find it in the
{{ '[traces folder]({}/doc/advanced/traceplayer/traces)'.format(repo) }}.

Download the X5 project from its [repository](https://github.com/bmwcarit/digital-car-3d) and load the **G05_main.rca** file as documented in project README.
Load and play the RaCo trace 
{{ '[g05_demo]({}/doc/advanced/traceplayer/traces/g05_demo.rctrace)'.format(repo) }}.
Try changing the frame timestamps and properties to see how the playback changes in the Ramses Composer viewport.
