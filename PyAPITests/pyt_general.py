#
# SPDX-License-Identifier: MPL-2.0
#
# This file is part of Ramses Composer
# (see https://github.com/bmwcarit/ramses-composer).
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
# If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
# 
import unittest
import raco
import os

class GeneralTests(unittest.TestCase):
    def setUp(self):
        raco.reset()

    def cwd(self):
        return os.getcwd()

    def test_reset(self):
        raco.reset()
        self.assertEqual(len(raco.instances()), 1)
        self.assertEqual(raco.instances()[0].typeName(), "ProjectSettings")


    def test_load(self):
        raco.load(self.cwd() + "/../resources/example_scene.rca")

        with self.assertRaises(RuntimeError):
            raco.load("")

        with self.assertRaises(RuntimeError):
            raco.load(self.cwd() + "/wrong-extension.abc")

        with self.assertRaises(RuntimeError):
            raco.load(self.cwd() + "/no-such-file.rca")

        with self.assertRaises(RuntimeError):
            raco.load(self.cwd() + "/../resources/future-version.rca")

        with self.assertRaises(RuntimeError):
            raco.load(self.cwd() + "/../resources/no-json.rca")

        with self.assertRaises(RuntimeError):
            raco.load(self.cwd() + "/../resources/json-but-invalid-raco.rca")

        with self.assertRaises(RuntimeError):
            raco.load(self.cwd() + "/../resources/too-short.rca")

        with self.assertRaises(RuntimeError):
            raco.load(self.cwd() + "/../resources/fake-zip.rca")

        with self.assertRaises(RuntimeError):
            raco.load(self.cwd() + "/../resources/multi-file-zip.rca")

    def test_project_path(self):
        self.assertEqual(raco.projectPath(), "")

        raco.load(self.cwd() + "/../resources/example_scene.rca")
        self.assertEqual(os.path.normpath(raco.projectPath()), os.path.normpath(self.cwd() + "/../resources/example_scene.rca"))

    def test_instances(self):
        self.assertTrue(len(raco.instances()) > 0)
        
    def test_create_object(self):
        node = raco.create("Node", "my_node")
        self.assertTrue(node.objectName, "my_node")
        self.assertEqual(node.typeName(), "Node")
        self.assertEqual(node.parent(), None)
        self.assertEqual(node.children(), [])
        self.assertTrue(node.objectID() != None)
        self.assertEqual(dir(node), ['enabled', 'objectName', 'rotation', 'scaling', 'translation', 'visibility'])

    def test_create_fail(self):
        with self.assertRaises(RuntimeError):
            raco.create("ProjectSettings", "mysettings")
        with self.assertRaises(RuntimeError):
            raco.create("UnknownType", "myname")
        
    def test_delete(self):
        node = raco.create("Node", "my_node")
        self.assertEqual(raco.delete(node), 1)

        with self.assertRaises(RuntimeError):
            raco.delete(node)

        node1 = raco.create("Node", "my_node1")
        node2 = raco.create("Node", "my_node2")
        self.assertEqual(raco.delete([node1, node2]), 2)
        
        with self.assertRaises(RuntimeError):
            raco.delete([node1, node2])
        
    def test_delete_all(self):
        raco.delete(raco.instances())
        self.assertEqual(len(raco.instances()), 1)

    def test_prop_handle_operators(self):
        node = raco.create("Node", "my_node")
        self.assertEqual(node.translation, node.translation)
        self.assertEqual(node.translation, getattr(node, "translation"))
        self.assertEqual(node.translation.x, node.translation.x)
        
        node2 = raco.create("Node", "my_node")
        self.assertNotEqual(node.translation, node2.translation)

    def test_prop_handle_functions(self):
        node = raco.create("Node", "my_node")
        self.assertEqual(node.translation.object(), node)
        self.assertEqual(node.translation.x.object(), node)
        self.assertEqual(node.translation.propName(), "translation")
        self.assertEqual(dir(node.translation), ["x", "y", "z"])
        self.assertEqual(node.translation.typeName(), "Vec3f")
        self.assertEqual(node.translation.x.typeName(), "Double")

    def test_prop_get(self):
        node = raco.create("Node", "my_node")
        self.assertEqual(node.visibility.value(), True)
        self.assertEqual(node.visibility.value(), getattr(node, "visibility").value())
        self.assertEqual(node.translation.x.value(), getattr(node.translation, "x").value())
        self.assertEqual(node.translation.x.value(), getattr(node, "translation").x.value())

    def test_prop_get_top_fail(self):
        node = raco.create("Node", "my_node")
        with self.assertRaises(RuntimeError):
            node.nosuch
 
    def test_prop_set_top_fail(self):
        node = raco.create("Node", "my_node")
        with self.assertRaises(RuntimeError):
            node.nosuch = 42
        with self.assertRaises(RuntimeError):
            node.objectID = "123"

        raco.delete(node)
        
        with self.assertRaises(RuntimeError):
            node.no_such_property = True
        with self.assertRaises(RuntimeError):
            node.visibility = True

    def test_set_int_enum_fail(self):
        material = raco.create("Material", "my_mat")

        self.assertEqual(material.options.cullmode.value(), 2)
        material.options.cullmode = 1
        self.assertEqual(material.options.cullmode.value(), 1)
        with self.assertRaises(RuntimeError):
            material.options.cullmode = 42
        self.assertEqual(material.options.cullmode.value(), 1)

    def test_set_int_enum(self):
        material = raco.create("Material", "my_mat")

        self.assertEqual(material.options.cullmode.value(), raco.ECullMode.Back)
        material.options.cullmode = raco.ECullMode.Front
        self.assertEqual(material.options.cullmode.value(), raco.ECullMode.Front)

    def test_prop_set_top_bool(self):
        node = raco.create("Node", "my_node")
        
        self.assertEqual(node.visibility.value(), True)

        node.visibility = False
        self.assertEqual(node.visibility.value(), False)
        setattr(node, "visibility", True)
        self.assertEqual(node.visibility.value(), True)

        # conversion: int to bool
        node.visibility = 0
        self.assertEqual(node.visibility.value(), False)
        node.visibility = 42
        self.assertEqual(node.visibility.value(), True)
        
        # conversion: float to bool
        node.visibility = 0.0
        self.assertEqual(node.visibility.value(), False)
        node.visibility = 2.345
        self.assertEqual(node.visibility.value(), True)


    def test_prop_get_nested_fail(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"

        self.assertTrue(lua.inputs != None)
        with self.assertRaises(RuntimeError):
            lua.inputs.nosuch
 
    def test_prop_set_nested_fail(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"
        
        self.assertTrue(lua.inputs != None)
        with self.assertRaises(RuntimeError):
            lua.inputs.nosuch = 42

        raco.delete(lua)
        
        with self.assertRaises(RuntimeError):
            lua.inputs.nosuch = 42


    def test_prop_set_nested_bool(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"

        lua.inputs.bool = False
        self.assertEqual(lua.inputs.bool.value(), False)
        setattr(lua.inputs, "bool", True)
        self.assertEqual(lua.inputs.bool.value(), True)

        # conversion: int to bool
        lua.inputs.bool = 0
        self.assertEqual(lua.inputs.bool.value(), False)
        lua.inputs.bool = 42
        self.assertEqual(lua.inputs.bool.value(), True)
        
        # conversion: float to bool
        lua.inputs.bool = 0.0
        self.assertEqual(lua.inputs.bool.value(), False)
        lua.inputs.bool = 2.345
        self.assertEqual(lua.inputs.bool.value(), True)
    

    def test_move_scenegraph(self):
        parent = raco.create("Node", "parent")
        child = raco.create("MeshNode", "child")
        self.assertEqual(parent.children(), [])
        self.assertEqual(child.parent(), None)
        
        raco.moveScenegraph(child, parent)
        self.assertEqual(parent.children(), [child])
        self.assertEqual(child.parent(), parent)

        raco.moveScenegraph(child, None)
        self.assertEqual(parent.children(), [])
        self.assertEqual(child.parent(), None)
        
    
    def test_move_scenegraph_fail_invalid_parent(self):
        parent = raco.create("Node", "parent")
        child = raco.create("MeshNode", "child")
        raco.delete(parent)
        with self.assertRaises(RuntimeError):
            raco.moveScenegraph(child, parent)
            
    def test_move_scenegraph_fail_invalid_object(self):
        parent = raco.create("Node", "parent")
        child = raco.create("MeshNode", "child")
        raco.delete(child)
        with self.assertRaises(RuntimeError):
            raco.moveScenegraph(child, parent)
    
    def test_move_scenegraph_with_index(self):
        parent = raco.create("Node", "parent")
        child = raco.create("MeshNode", "child")

        with self.assertRaises(RuntimeError):
            raco.moveScenegraph(child, parent, -2)

        raco.moveScenegraph(child, parent, -1)
        raco.moveScenegraph(child, None)
        
        raco.moveScenegraph(child, parent, 0)
        raco.moveScenegraph(child, None)

        with self.assertRaises(RuntimeError):
            raco.moveScenegraph(child, parent, 1)
        
        with self.assertRaises(RuntimeError):
            raco.moveScenegraph(child, None, 0)

        
    def test_link_create_remove(self):
        node = raco.create("Node", "node")
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"
        
        link = raco.addLink(lua.outputs.ovector3f, node.translation)
        self.assertEqual(link.start, lua.outputs.ovector3f)
        self.assertEqual(link.end, node.translation)
        self.assertTrue(link.valid)
        self.assertFalse(link.weak)
        
        queried_link = raco.getLink(node.translation)
        self.assertEqual(link, queried_link)
        self.assertEqual(raco.links(), [link])
        
        raco.removeLink(node.translation)
        self.assertEqual(raco.links(), [])

    def test_addLink_weak(self):
        lua_start = raco.create("LuaScript", "start")
        lua_start.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"

        lua_end = raco.create("LuaScript", "end")
        lua_end.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"
        
        link_strong = raco.addLink(lua_start.outputs.ofloat, lua_end.inputs.float)
        link_weak = raco.addLink(lua_end.outputs.ofloat, lua_start.inputs.float, True)
        self.assertFalse(link_strong.weak)
        self.assertTrue(link_weak.weak)
        
        self.assertEqual(len(raco.links()), 2)
        queried_link_strong = raco.getLink(lua_end.inputs.float)
        self.assertEqual(link_strong, queried_link_strong)
        queried_link_weak = raco.getLink(lua_start.inputs.float)
        self.assertEqual(link_weak, queried_link_weak)

    
    def test_getLink_fail(self):
        node = raco.create("Node", "node")
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"
        
        link = raco.addLink(lua.outputs.ovector3f, node.translation)
        raco.delete(node)
        with self.assertRaises(RuntimeError):
            raco.getLink(node.translation)
    
    def test_addLink_fail_no_start(self):
        node = raco.create("Node", "node")
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"
        
        start = lua.outputs.ovector3f
        end = node.translation
        
        raco.delete(lua)
        with self.assertRaises(RuntimeError):
            raco.addLink(start, end)
        
    def test_addLink_fail_no_end(self):
        node = raco.create("Node", "node")
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"
        
        start = lua.outputs.ovector3f
        end = node.translation
        
        raco.delete(lua)
        with self.assertRaises(RuntimeError):
            raco.addLink(start, end)

    def test_addLink_fail_lua_output_as_end(self):
        lua_start = raco.create("LuaScript", "start")
        lua_start.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"

        lua_end = raco.create("LuaScript", "end")
        lua_end.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"

        with self.assertRaises(RuntimeError):
            raco.addLink(lua_start.outputs.ovector3f, lua_end.outputs.ovector3f)
        
    def test_link_remove_fail_no_end(self):
        node = raco.create("Node", "node")
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"
        
        link = raco.addLink(lua.outputs.ovector3f, node.translation)
            
        raco.delete(node)
        with self.assertRaises(RuntimeError):
            raco.removeLink(node.translation)
        
    def test_link_remove_fail_end_readonly(self):
        prefab = raco.create("Prefab", "prefab")
        node = raco.create("Node", "node")
        raco.moveScenegraph(node, prefab)
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"
        raco.moveScenegraph(lua, prefab)
        
        link = raco.addLink(lua.outputs.ovector3f, node.translation)
    
        inst = raco.create("PrefabInstance", "inst")
        inst.template = prefab
        
        inst_lua = [child for child in inst.children() if child.typeName() == "LuaScript"][0]
        inst_node = [child for child in inst.children() if child.typeName() == "Node"][0]
    
        inst_link = raco.getLink(inst_node.translation)
        self.assertEqual(inst_link.start, inst_lua.outputs.ovector3f)
    
        with self.assertRaises(RuntimeError):
            raco.removeLink(inst_node.translation)
        
    def test_engine_update_lua_outputs(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/SimpleScript.lua"

        self.assertEqual(lua.outputs.out_float.value(), 0.0)
        lua.inputs.in_float = 2.0
        self.assertEqual(lua.outputs.out_float.value(), 2.0)

    def test_engine_update_lua_link(self):
        start = raco.create("LuaScript", "lua_start")
        start.uri = self.cwd() + R"/../resources/scripts/SimpleScript.lua"
        end = raco.create("LuaScript", "lua_end")
        end.uri = self.cwd() + R"/../resources/scripts/SimpleScript.lua"
        
        self.assertTrue(raco.addLink(start.outputs.out_float, end.inputs.in_float) != None)

        self.assertEqual(end.inputs.in_float.value(), 0.0)
        start.inputs.in_float = 2.0
        self.assertEqual(end.inputs.in_float.value(), 2.0)
    
        raco.removeLink(end.inputs.in_float)
        start.inputs.in_float = 3.0
        self.assertEqual(end.inputs.in_float.value(), 2.0)


    def test_feature_level_2_node_enabled(self):
        node = raco.create("Node", "node")
        self.assertEqual(node.enabled.value(), True)

    def test_feature_level_2_perspective_camera_frustum(self):
        camera = raco.create("PerspectiveCamera", "camera")

        self.assertEqual(camera.frustumType.value(), raco.EFrustumType.Aspect_FoV)
        self.assertEqual(camera.frustum.aspectRatio.value(), 2.0)
        with self.assertRaises(RuntimeError):
            dummy = camera.frustum.leftPlane.value()
        camera.frustum.aspectRatio = 3.0
        
        camera.frustumType = raco.EFrustumType.Planes
        
        with self.assertRaises(RuntimeError):
                dummy = camera.frustum.aspectRatio.value()
        self.assertEqual(camera.frustum.leftPlane.value(), -10.0)
        camera.frustum.leftPlane = -12.0

        # check value caching
        camera.frustumType = raco.EFrustumType.Aspect_FoV
        self.assertEqual(camera.frustum.aspectRatio.value(), 3.0)
        
    def test_feature_level_2_anchor_point(self):
        node = raco.create("Node", "node")
        anchor = raco.create("AnchorPoint", "test_anchor")
        
        anchor.node = node
        
    def test_feature_level_2_renderpass_prop(self):
        renderpass = raco.create("RenderPass", "render_pass")
        self.assertEqual(renderpass.renderOnce.value(), False)


    def test_feature_level_2_renderpass_link(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"
        
        renderpass = raco.create("RenderPass", "render_pass")

        self.assertEqual(renderpass.enabled.value(), True)
        link_enabled = raco.addLink(lua.outputs.obool, renderpass.enabled)
        
        self.assertEqual(renderpass.renderOrder.value(), 1)
        link_order = raco.addLink(lua.outputs.ointeger, renderpass.renderOrder)
        
        self.assertEqual(renderpass.clearColor.x.value(), 0.0)
        link_color = raco.addLink(lua.outputs.ovector4f, renderpass.clearColor)
        
        self.assertEqual(len(raco.links()), 3)
        
        queried_link_enabled = raco.getLink(renderpass.enabled)
        self.assertEqual(link_enabled, queried_link_enabled)
        
        queried_link_order = raco.getLink(renderpass.renderOrder)
        self.assertEqual(link_order, queried_link_order)
        
        queried_link_color = raco.getLink(renderpass.clearColor)
        self.assertEqual(link_color, queried_link_color)
        
        
        







