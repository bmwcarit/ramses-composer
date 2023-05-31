<!--
SPDX-License-Identifier: MPL-2.0

This file is part of Ramses Composer
(see https://github.com/bmwcarit/ramses-composer-docs).

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->

# Lua Modules
*You can find the example project {{ '[here]({}/doc/advanced/modules)'.format(repo) }}.*

![](../../basics/monkey/docs/viewport_preview.png)

This tutorial demonstrates how to reuse Lua functions and configuration data using Lua modules.

## What are modules good for?

Any large project needs to reuse code or data at some point. This holds true for assets, in particular for Lua source code.
It's quite easy to reuse data/resources in the Ramses Composer - you just have to reference the same external file (png, glsl, glTF etc.).
Naturally, you can also have two script instances referencing the same Lua source file. The Ramses Composer will however still
create two *instances* of Lua scripts, which will still have their own state and data. The only thing they share is that both scripts
will be recreated if the source code changes. Moreover, if you want to slightly modify one of the instances, you have to create two mostly
identical Lua source files, and thus create code duplication.

Modules provide a solution to this problem which works very similar to how typical libraries or modules in other languages work.
You can put some of the functions and data you need in your project in a module, and reference that module from multiple scripts.
Let's build an example which does that!

## Copy paste

In this tutorial, we will use the [monkey example](../../basics/monkey/README.md#recreate-monkey-sample) and build on top of it.
In that example, we created three monkey heads which are lit by a Phong shader which is in turn controlled by a Lua script.
Let's imagine we wanted to create a second, different lighting model, but we wanted to keep the light configuration and the color
settings shared between the old and the new shading model.

We notice that that the basic monkey lighting script has default values for the light properties, which are set in the GLOBAL table
during initialization of the script. Also, the run() function checks if a non-zero value is set for the colors, and if not, uses
the default colors. We want to move both the colors and the function which decides whether to use a default color or not into a new module.

## Creating the light module

To create a new module, we need to create a new Lua source file and write its code there. We follow the
[official Logic Engine docs](https://ramses-logic.readthedocs.io/en/latest/lua_syntax.html#custom-modules) and
create the following [Lua module source file](./scripts/LightModule.lua). You may notice that the file looks quite
similar to the LightControl script from the basic monkey example, with a few structural differences:

* The module has no global functions such as interface()
* Instead, it declares a table (called light) in the beginning of the file and returns it to the caller at the end
* In the table, the module puts data (color and light properties) and functions to obtain those (getLightDirection(), resolveColor())
* It has more error checks than the original LightControl script

We will explain later why this is necessary. Let's move on and use the new module first!

## Using the module

Modules have to be explicitly created and imported in the scripts which need to access them. To create a module,
right click the resources area and create a new object of type 'LuaScriptModule'. Set its URI to the source code file of the Lua Module (LightModule) we created in the previous section.

To let a script make use of a module, you have to explicitly declare how you want to use it. Add the following line in the LightControl script:

```lua
modules('light')

function interface(IN,OUT)
...

```

Create a new LuaScript in the Scene Graph and add the LightControl script to it. The error "Required LuaScriptModule 'light' is unassigned" will show up, because you declared a module in the script file but didn't provide a module to the respective LuaScript object in Ramses Composer. To fix the error, go to the LuaScript's property page and you will now see a list property called Modules with one entry called 'light'. Click and select
the only module we have in the project - the LightModule.

Now the LightControl script will display another error. In order to fix it, give the light_id in the Property Browser of the LuaScript a value between 1 and 3

Now the LuaScript is working again, but it essentially does the same thing as before - hardcodes its light properties.
We included the new module as a dependency, but we are not using it yet. To use it, we can refer to all functions and data
members of the module using a table with the same name as declared in the ``modules()`` statement:

```lua

modules("light")

function interface(IN,OUT)
   ...
end

function run(IN,OUT)
    OUT.light_direction = light.getLightDirection(IN.light_id)
    OUT.light_color     = light.resolveColor("light", IN.light_color)
    OUT.diffuse_color   = light.resolveColor("diffuse", IN.diffuse_color)
end
```

Now, with this change, the LightControl Lua script delegates the configuration of the light properties
to the LightModule module. You can now add a second script, which uses the same basic configuration,
but modifies or extends it. For example, it could make the colors brigther, or it could substitute the
hardcoded diffuse color with a dynamic color which is animated. Feel free to experiment!

## Notes on the LightModule

The example module we created for this example has a slightly different syntax than other scripts in the Logic engine.
The first notable difference is that it declares no global functions or data, but instead declares a local table (the module),
fills it with content, and returns it. That's because this is how Lua recommends to define modules
(see e.g. [this guide](https://www.tutorialspoint.com/lua/lua_modules.htm)). This means that you can
use any existing Lua source code and expose it as a module in your assets - provided that it uses only the standard modules
[allowed by the Logic Engine](https://ramses-logic.readthedocs.io/en/latest/lua_syntax.html#standard-modules).

The second notable difference is that the light module issues errors when some of the arguments don't match the
expectations of the function. You can of course do some sort of "default behavior" instead of the error. However, consider
that if you use the module from multiple scripts, it can become difficult to track errors. It is therefore recommended
to use the Lua ``error()`` built-in function to raise an error if something is not as expected in the module functions.

## Recreate modules sample

This example is created in a way similar to the [monkey example](../../basics/monkey/README.md#recreate-monkey-sample). We only highlight the
steps specific to this example here:

* Create a copy of the monkey project
* Create a new Lua file which creates an empty table and returns it
* Create a new LuaScriptModule in the Resource view
    * Set its URI to the file created above
    * Set its name to "LightModule"
* Create a new LuaScript in the Scene Graph
    * Fix the "missing module" error in the LightControl script by selecting the "LightModule" from the dropdown list
    * Fix the id error by changing the light_id to a value between 1 and 3
* Modify the LightControl script from the monkey example to start with `modules("light")`
* Modify the LightControl script to use data/functions from the LightModule

