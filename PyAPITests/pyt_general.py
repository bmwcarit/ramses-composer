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

    def findObjectByName(self, name):
        for object in raco.instances():
            if object.objectName.value() == name:
                return object

    def findObjectByType(self, type):
        for object in raco.instances():
            if object.typeName() == type:
                return object

    def instance_names(self):
        return [obj.objectName.value() for obj in raco.instances()]

    def test_reset(self):
        raco.reset()
        self.assertEqual(len(raco.instances()), 1)
        self.assertEqual(raco.instances()[0].typeName(), "ProjectSettings")

    def test_reset_feature_levels(self):
        raco.reset(1)
        self.assertEqual(raco.projectFeatureLevel(), 1)
        raco.reset(2)
        self.assertEqual(raco.projectFeatureLevel(), 2)

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

    def test_load_feature_levels(self):
        raco.load(self.cwd() + "/../resources/empty-fl1.rca")
        self.assertEqual(raco.projectFeatureLevel(), 1)

        raco.load(self.cwd() + "/../resources/empty-fl1.rca", 1)
        self.assertEqual(raco.projectFeatureLevel(), 1)

        raco.load(self.cwd() + "/../resources/empty-fl1.rca", 2)
        self.assertEqual(raco.projectFeatureLevel(), 2)

        raco.load(self.cwd() + "/../resources/empty-fl2.rca")
        self.assertEqual(raco.projectFeatureLevel(), 2)

        with self.assertRaises(RuntimeError):
            raco.load(self.cwd() + "/../resources/empty-fl2.rca", 1)
        self.assertEqual(raco.projectFeatureLevel(), 1)

        raco.load(self.cwd() + "/../resources/empty-fl2.rca", 2)
        self.assertEqual(raco.projectFeatureLevel(), 2)

    def test_save_check_object_id(self):
        objectInitID = self.findObjectByType("ProjectSettings").objectID()
        raco.save(self.cwd() + "/project.rca")
        self.assertEqual(self.findObjectByType("ProjectSettings").objectID(), objectInitID)

    def test_save_check_object_name(self):
        raco.reset()
        raco.save(self.cwd() + "/project.rca")
        self.assertEqual("project", self.findObjectByType("ProjectSettings").objectName.value())
        raco.load(self.cwd() + "/project.rca")
        self.assertEqual("project", self.findObjectByType("ProjectSettings").objectName.value())

    def test_save_as_with_new_id(self):
        objectInitID = self.findObjectByType("ProjectSettings").objectID()
        raco.save(self.cwd() + "/project.rca", True)
        self.assertNotEqual(self.findObjectByType("ProjectSettings").objectID(), objectInitID)

    def test_save_as_with_new_id_same_path(self):
        objectInitID = self.findObjectByType("ProjectSettings").objectID()
        raco.save(self.cwd() + "/project.rca", True)
        objectNewID1 = self.findObjectByType("ProjectSettings").objectID()
        raco.save(self.cwd() + "/project.rca", True)
        objectNewID2 = self.findObjectByType("ProjectSettings").objectID()
        self.assertNotEqual(objectNewID1, objectNewID2)

    def test_project_path(self):
        self.assertEqual(raco.projectPath(), "")

        raco.load(self.cwd() + "/../resources/example_scene.rca")
        self.assertEqual(os.path.normpath(raco.projectPath()), os.path.normpath(self.cwd() + "/../resources/example_scene.rca"))

    def test_instances(self):
        self.assertTrue(len(raco.instances()) > 0)

    def test_create_object(self):
        node = raco.create("Node", "my_node")
        self.assertTrue(node.objectName, "my_node")
        self.assertTrue(node["objectName"], "my_node")

    def test_object_functions(self):
        node = raco.create("Node", "my_node")

        self.assertEqual(node.typeName(), "Node")
        self.assertEqual(node.parent(), None)
        self.assertEqual(node.children(), [])
        self.assertTrue(node.objectID() != None)
        self.assertEqual(dir(node), ['enabled', 'objectName', 'rotation', 'scaling', 'translation', 'visibility'])
        self.assertEqual(node.keys(), ['objectName', 'visibility', 'enabled', 'translation', 'rotation', 'scaling'])

        self.assertTrue(node.objectName, "my_node")
        self.assertTrue(node["objectName"], "my_node")

        raco.delete(node)
        
        with self.assertRaises(RuntimeError):
            node.typeName()
        with self.assertRaises(RuntimeError):
            node.parent()
        with self.assertRaises(RuntimeError):
            node.children()
        with self.assertRaises(RuntimeError):
            node.objectID()
        with self.assertRaises(RuntimeError):
            dir(node)
        with self.assertRaises(RuntimeError):
            node.keys()

        with self.assertRaises(RuntimeError):
            node.objectName
        with self.assertRaises(RuntimeError):
            node.objectName = "your_node"
        with self.assertRaises(RuntimeError):
            node["objectName"]
        with self.assertRaises(RuntimeError):
            node["onjectName"] = "your_node"

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
        self.assertEqual(node.translation.typeName(), "Vec3f")
        self.assertEqual(node.translation.propName(), "translation")
        self.assertEqual(node.translation.hasSubstructure(), True)
        self.assertEqual(dir(node.translation), ["x", "y", "z"])
        self.assertEqual(node.translation.keys(), ["x", "y", "z"])

        self.assertEqual(node.translation.x.object(), node)
        self.assertEqual(node.translation.x.typeName(), "Double")

        raco.delete(node)

        with self.assertRaises(RuntimeError):
            node.translation.object()
        with self.assertRaises(RuntimeError):
            node.translation.typeName()
        with self.assertRaises(RuntimeError):
            node.translation.propName()
        with self.assertRaises(RuntimeError):
            node.translation.hasSubstructure()
        with self.assertRaises(RuntimeError):
            node.translation.value()
        with self.assertRaises(RuntimeError):
            dir(node.translation)
        with self.assertRaises(RuntimeError):
            node.translation.keys()


    def test_prop_handle_invalid_functions(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"

        handle = lua.inputs.float
        vecHandle = lua.inputs.vector3f

        self.assertEqual(handle.object(), lua)
        self.assertEqual(handle.typeName(), "Double")
        self.assertEqual(handle.propName(), "float")
        self.assertEqual(handle.hasSubstructure(), False)
        self.assertEqual(handle.value(), 0.0)
        self.assertEqual(dir(vecHandle), ["x", "y", "z"])
        self.assertEqual(vecHandle.keys(), ["x", "y", "z"])

        lua.uri = ""

        self.assertEqual(handle.object(), lua)
        with self.assertRaises(RuntimeError):
            handle.typeName()
        with self.assertRaises(RuntimeError):
            handle.propName()
        with self.assertRaises(RuntimeError):
            handle.hasSubstructure()
        with self.assertRaises(RuntimeError):
            handle.value()
        with self.assertRaises(RuntimeError):
            dir(vecHandle)
        with self.assertRaises(RuntimeError):
            vecHandle.keys()
        
    def test_prop_get(self):
        node = raco.create("Node", "my_node")
        self.assertEqual(node.visibility.value(), True)
        self.assertEqual(node.visibility.value(), getattr(node, "visibility").value())
        self.assertEqual(node.translation.x.value(), getattr(node.translation, "x").value())
        self.assertEqual(node.translation.x.value(), getattr(node, "translation").x.value())

    def test_prop_get_types_scalar_via_attribute(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"

        self.assertEqual(lua.inputs.float.value(), 0.0)
        self.assertEqual(lua.inputs.integer.value(), 0)
        self.assertEqual(lua.inputs.integer64.value(), 0)
        self.assertEqual(lua.inputs.bool.value(), False)
        self.assertEqual(lua.inputs.s.value(), "")

        with self.assertRaises(RuntimeError):
            lua.inputs.vector2f.value()
        with self.assertRaises(RuntimeError):
            lua.inputs.vector3f.value()
        with self.assertRaises(RuntimeError):
            lua.inputs.vector4f.value()
        with self.assertRaises(RuntimeError):
            lua.inputs.vector2i.value()
        with self.assertRaises(RuntimeError):
            lua.inputs.vector3i.value()
        with self.assertRaises(RuntimeError):
            lua.inputs.vector4i.value()

        self.assertEqual(lua.inputs.float, getattr(lua.inputs, "float"))
        self.assertEqual(lua.inputs.integer, getattr(lua.inputs, "integer"))
        self.assertEqual(lua.inputs.integer64, getattr(lua.inputs, "integer64"))
        self.assertEqual(lua.inputs.bool, getattr(lua.inputs, "bool"))
        self.assertEqual(lua.inputs.s, getattr(lua.inputs, "s"))
        self.assertEqual(lua.inputs.vector2f, getattr(lua.inputs, "vector2f"))
        self.assertEqual(lua.inputs.vector3f, getattr(lua.inputs, "vector3f"))
        self.assertEqual(lua.inputs.vector4f, getattr(lua.inputs, "vector4f"))
        self.assertEqual(lua.inputs.vector2i, getattr(lua.inputs, "vector2i"))
        self.assertEqual(lua.inputs.vector3i, getattr(lua.inputs, "vector3i"))
        self.assertEqual(lua.inputs.vector4i, getattr(lua.inputs, "vector4i"))

    def test_prop_get_types_scalar_via_index(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"

        self.assertEqual(lua.inputs["float"].value(), 0.0)
        self.assertEqual(lua.inputs["integer"].value(), 0)
        self.assertEqual(lua.inputs["integer64"].value(), 0)
        self.assertEqual(lua.inputs["bool"].value(), False)
        self.assertEqual(lua.inputs["s"].value(), "")

        with self.assertRaises(RuntimeError):
            lua.inputs["vector2f"].value()
        with self.assertRaises(RuntimeError):
            lua.inputs["vector3f"].value()
        with self.assertRaises(RuntimeError):
            lua.inputs["vector4f"].value()
        with self.assertRaises(RuntimeError):
            lua.inputs["vector2i"].value()
        with self.assertRaises(RuntimeError):
            lua.inputs["vector3i"].value()
        with self.assertRaises(RuntimeError):
            lua.inputs["vector4i"].value()

    def test_prop_get_conflicting_names(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/name-conflict.lua"

        self.assertEqual(lua.inputs['value'].value(), 0.0)
        self.assertEqual(lua.inputs['object'].value(), 0)

    def test_prop_get_set_array_via_index(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/array.lua"

        for index in range(1,6):
            self.assertEqual(lua.inputs.float_array[str(index)].value(), 0.0)

        for index in range(1,6):
            lua.inputs.float_array[str(index)] = index

        for index in range(1,6):
            self.assertEqual(lua.inputs.float_array[str(index)].value(), index)

    def test_prop_get_set_array_of_array_via_index(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/array-of-array.lua"

        for i in range(1,3):
            for j in range(1,2):
                self.assertEqual(lua.inputs.float_array[str(i)][str(j)].value(), 0.0)

        for i in range(1,3):
            for j in range(1,2):
                lua.inputs.float_array[str(i)][str(j)] = 10*i + j

        for i in range(1,3):
            for j in range(1,2):
                self.assertEqual(lua.inputs.float_array[str(i)][str(j)].value(), 10*i + j)

    def test_prop_get_set_array_of_struct_via_index(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/array-of-structs.lua"

        for index in range(1,3):
            self.assertEqual(lua.inputs.array[str(index)].a.value(), 0.0)
            self.assertEqual(lua.inputs.array[str(index)].b.value(), 0.0)

        for index in range(1,3):
            lua.inputs.array[str(index)].a = 2 * index
            lua.inputs.array[str(index)].b = 3 * index

        for index in range(1,3):
            self.assertEqual(lua.inputs.array[str(index)].a.value(), 2 * index)
            self.assertEqual(lua.inputs.array[str(index)].b.value(), 3 * index)

    def test_prop_set_types_scalar_via_attribute(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"

        lua.inputs.float = 3.0
        self.assertEqual(lua.inputs.float.value(), 3.0)
        lua.inputs.integer = 4
        self.assertEqual(lua.inputs.integer.value(), 4)
        lua.inputs.integer64 = 0xabcd12345678
        self.assertEqual(lua.inputs.integer64.value(), 0xabcd12345678)
        lua.inputs.bool = True
        self.assertEqual(lua.inputs.bool.value(), True)
        lua.inputs.s = "test string"
        self.assertEqual(lua.inputs.s.value(), "test string")

    def test_prop_set_types_scalar_via_index(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"

        lua.inputs["float"] = 3.0
        self.assertEqual(lua.inputs.float.value(), 3.0)
        lua.inputs["integer"]= 4
        self.assertEqual(lua.inputs.integer.value(), 4)
        lua.inputs["integer64"] = 0xabcd12345678
        self.assertEqual(lua.inputs.integer64.value(), 0xabcd12345678)
        lua.inputs["bool"] = True
        self.assertEqual(lua.inputs.bool.value(), True)
        lua.inputs['s'] = "test string"
        self.assertEqual(lua.inputs.s.value(), "test string")

    def test_prop_set_conflicting_names(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/name-conflict.lua"

        lua.inputs['value'] = 3.0
        self.assertEqual(lua.inputs['value'].value(), 3.0)
        lua.inputs['object'] = 4
        self.assertEqual(lua.inputs['object'].value(), 4)

    def test_prop_get_top_fail(self):
        node = raco.create("Node", "my_node")
        with self.assertRaises(RuntimeError):
            node.nosuch
    
        raco.delete(node)
        
        with self.assertRaises(RuntimeError):
            node.visibility
 
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

    def test_vec_prop_set(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"

        # Vector assignment from list
        lua.inputs.vector2i = [1, 2]
        self.assertEqual(lua.inputs.vector2i.i1.value(), 1)

        lua.inputs.vector2f = [1.5, 2]
        lua.inputs.vector3i = [1, 2, 3]
        lua.inputs.vector3f = [1.5, 2, 3]
        lua.inputs.vector4i = [1, 2, 3, 4]
        lua.inputs.vector4f = [1.5, 2, 3, 4]
        self.assertEqual(lua.inputs.vector4f.w.value(), 4)

        # Vector assignment from tuple
        lua.inputs.vector2f = (3.5, 4)
        self.assertEqual(lua.inputs.vector2f.x.value(), 3.5)

        # Vector dimension must match list length
        with self.assertRaises(RuntimeError):
            lua.inputs.vector2f = (1, 2, 3)

    def test_array_prop_set_from_list_fail(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/array.lua"

        # Array of size 5 exists
        self.assertEqual(len(lua.inputs.float_array.keys()), 5)

        # Array assignment is not supported
        with self.assertRaises(RuntimeError):
            lua.inputs.float_array = [1, 2, 3, 4, 5]

    def test_struct_prop_set_from_list_fail(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/struct.lua"

        # Struct with 2 elements exists
        self.assertEqual(len(lua.inputs.struct.keys()), 2)

        # Struct assignment is not supported
        with self.assertRaises(RuntimeError):
            lua.inputs.struct = [1, 2]

    def test_vec_prop_set_from_non_iterable_fail(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"

        # Assigning non-iterable results in error
        with self.assertRaises(TypeError):
            lua.inputs.vector2i = 1

    def test_vec_prop_set_from_wrong_type_fail(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"

        # Assigning non-iterable results in error
        with self.assertRaises(RuntimeError):
            lua.inputs.vector2i = [1, "abc"]

    def test_set_int_enum_fail(self):
        material = raco.create("Material", "my_mat")

        self.assertEqual(material.options.cullmode.value(), raco.ECullMode.Back)
        material.options.cullmode = 1
        self.assertEqual(material.options.cullmode.value(), raco.ECullMode.Front)
        with self.assertRaises(RuntimeError):
            material.options.cullmode = 42
        self.assertEqual(material.options.cullmode.value(), raco.ECullMode.Front)

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
        
        raco.moveScenegraph(child, None, 0)

    def test_move_scenegraph_toplevel(self):
        node1 = raco.create("Node", "node1")
        node2 = raco.create("Node", "node2")
        self.assertEqual(self.instance_names(), ["", "node1", "node2"])

        raco.moveScenegraph(node2, node1, 0)
        self.assertEqual(self.instance_names(), ["", "node1", "node2"])

        raco.moveScenegraph(node2, None, 0)
        self.assertEqual(self.instance_names(), [ "node2", "", "node1"])

        raco.moveScenegraph(node2, node1, 0)
        self.assertEqual(self.instance_names(), [ "node2", "", "node1"])

        raco.moveScenegraph(node2, None, 3)
        self.assertEqual(self.instance_names(), ["", "node1", "node2"])

        raco.moveScenegraph(node1, None, -1)
        self.assertEqual(self.instance_names(), ["", "node2", "node1"])

        with self.assertRaises(RuntimeError):
            raco.moveScenegraph(node2, None, 4)

        with self.assertRaises(RuntimeError):
            raco.moveScenegraph(node2, None, -2)

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

    def test_errors_object_and_property(self):
        lua = raco.create("LuaScript", "lua_script")
        lua.uri = self.cwd() + R"/../resources/scripts/interface-color.lua"

        interface = raco.create("LuaInterface", "lua_interface")

        scene_errors = raco.getErrors()
        self.assertEqual(len(scene_errors), 2)
        print(scene_errors)

        for error in scene_errors:
            if error.handle() == lua:
                self.assertEqual(error.category(), raco.ErrorCategory.PARSING)
                self.assertEqual(error.level(), raco.ErrorLevel.ERROR)
                self.assertEqual(error.message(), "[lua_script] No 'run' function defined!")
            elif error.handle() == interface.uri:
                self.assertEqual(error.category(), raco.ErrorCategory.FILESYSTEM)
                self.assertEqual(error.level(), raco.ErrorLevel.WARNING)
                self.assertEqual(error.message(), "Empty URI.")
            else:
                self.assertTrue(False)

    def test_errors_fail_handle_no_object(self):
        lua = raco.create("LuaScript", "lua_script")
        lua.uri = self.cwd() + R"/../resources/scripts/interface-color.lua"

        errors = raco.getErrors()
        self.assertEqual(len(errors), 1)
        error = errors[0]

        self.assertEqual(error.handle(), lua)

        raco.delete(lua)

        with self.assertRaises(RuntimeError):
            error.handle()


    def test_raco_gui_should_be_unavailable_in_headless(self):
        with self.assertRaises(ModuleNotFoundError):
            import raco_gui

    def test_is_running_in_ui(self):
        self.assertFalse(raco.isRunningInUi())

    def test_min_and_max_feature_level(self):
        min = raco.minFeatureLevel()
        max = raco.maxFeatureLevel()
        self.assertTrue(min >= 1)
        self.assertTrue(max >= min)

    def test_get_instance_by_id(self):
        node1 = raco.create("Node", "node1")
        node2 = raco.create("Node", "node2")
        self.assertEqual(node1, raco.getInstanceById(node1.objectID()))
        self.assertEqual(node2, raco.getInstanceById(node2.objectID()))

    def test_get_instance_by_id_invalid(self):
        self.assertEqual(None, raco.getInstanceById("this should not exist"))

    def test_is_resource(self):
        mesh = raco.create("Mesh", "mesh")
        node = raco.create("Node", "node")
        self.assertTrue(mesh.isResource())
        self.assertFalse(node.isResource())

        raco.delete(node)

        with self.assertRaises(RuntimeError):
            node.isResource()

    def test_is_external_reference(self):
        node = raco.create("Node", "node")
        self.assertFalse(node.isExternalReference())

        raco.delete(node)

        with self.assertRaises(RuntimeError):
            node.isExternalReference()

    def test_is_valid_link_end(self):
        node = raco.create("Node", "node")
        self.assertTrue(node.visibility.isValidLinkEnd())

        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"

        self.assertTrue(lua.inputs.float.isValidLinkEnd())
        self.assertTrue(lua.inputs.s.isValidLinkEnd())
        self.assertFalse(lua.outputs.ofloat.isValidLinkEnd())
        self.assertFalse(lua.outputs.ointeger.isValidLinkEnd())

        handle = lua.inputs.float

        lua.uri = ""

        with self.assertRaises(RuntimeError):
            handle.isValidLinkEnd()

    def test_is_valid_link_start(self):
        node = raco.create("Node", "node")
        self.assertFalse(node.visibility.isValidLinkStart())

        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"

        self.assertFalse(lua.inputs.float.isValidLinkStart())
        self.assertFalse(lua.inputs.s.isValidLinkStart())
        self.assertTrue(lua.outputs.ofloat.isValidLinkStart())
        self.assertTrue(lua.outputs.ointeger.isValidLinkStart())

        handle = lua.inputs.float

        lua.uri = ""

        with self.assertRaises(RuntimeError):
            handle.isValidLinkEnd()


    def test_is_readonly_object(self):
        node = raco.create("Node", "node1")
        self.assertFalse(node.isReadOnly())

        prefab = raco.create("Prefab", "prefab")
        raco.moveScenegraph(node, prefab)

        instance = raco.create("PrefabInstance", "instance")
        instance.template = prefab
        for child in instance.children():
            self.assertTrue(child.isReadOnly())

        raco.delete(node)
        with self.assertRaises(RuntimeError):
            node.isReadOnly()

    def test_is_readonly_property(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"
        self.assertEqual(None, lua.getPrefabInstance())

        prefab = raco.create("Prefab", "prefab")
        raco.moveScenegraph(lua, prefab)
        self.assertEqual(None, lua.getPrefabInstance())

        instance = raco.create("PrefabInstance", "instance")
        instance.template = prefab
        for child in instance.children():
            self.assertTrue(child.objectName.isReadOnly())
            self.assertTrue(child.inputs.float)

        handle = instance.children()[0].inputs.float

        lua.uri = ""
        with self.assertRaises(RuntimeError):
            handle.isReadOnly()


    def test_get_prefab(self):
        node = raco.create("Node", "node1")
        self.assertEqual(None, node.getPrefab())

        prefab = raco.create("Prefab", "prefab")
        raco.moveScenegraph(node, prefab)
        self.assertEqual(prefab, node.getPrefab())

        instance = raco.create("PrefabInstance", "instance")
        instance.template = prefab
        self.assertEqual(None, instance.getPrefab())
        for child in instance.children():
            self.assertEqual(None, child.getPrefab())

        raco.delete(node)
        with self.assertRaises(RuntimeError):
            node.getPrefab()

    def test_get_prefab_instance(self):
        node = raco.create("Node", "node1")
        self.assertEqual(None, node.getPrefabInstance())

        prefab = raco.create("Prefab", "prefab")
        raco.moveScenegraph(node, prefab)
        self.assertEqual(None, node.getPrefabInstance())

        instance = raco.create("PrefabInstance", "instance")
        instance.template = prefab
        for child in instance.children():
            self.assertEqual(instance, child.getPrefabInstance())

        raco.delete(node)
        with self.assertRaises(RuntimeError):
            node.getPrefabInstance()

    def test_get_outer_containing_prefab_instance(self):
        node = raco.create("Node", "node1")
        prefab = raco.create("Prefab", "prefab")
        raco.moveScenegraph(node, prefab)
        self.assertEqual(None, node.getOuterContainingPrefabInstance())

        instance = raco.create("PrefabInstance", "instance")
        instance.template = prefab

        node2 = raco.create("Node", "node2")
        prefab2 = raco.create("Prefab", "prefab2")
        raco.moveScenegraph(node2, prefab2)

        nested_instance = raco.create("PrefabInstance", "nestedInstance")
        nested_instance.template = prefab2
        raco.moveScenegraph(nested_instance, prefab)

        self.assertEqual(instance, instance.getOuterContainingPrefabInstance())
        self.assertEqual(instance, instance.children()[1].getOuterContainingPrefabInstance())
        self.assertEqual(nested_instance, nested_instance.getOuterContainingPrefabInstance())

        def check_all_children(obj, expected_outer_instance):
            outer_instance = obj.getOuterContainingPrefabInstance()
            self.assertEqual(outer_instance.objectID(), expected_outer_instance.objectID())
            for child in obj.children():
                check_all_children(child, outer_instance)

        check_all_children(instance, instance)
        check_all_children(nested_instance, nested_instance)

        raco.delete(node)
        with self.assertRaises(RuntimeError):
            node.getOuterContainingPrefabInstance()

    def assertToyCarImportWorked(self):
        instances = raco.instances()
        self.assertTrue(any(filter(lambda x: x.objectName.value() == "ToyCar.gltf" and x.typeName() == "Node", instances)))
        self.assertTrue(any(filter(lambda x: x.objectName.value() == "ToyCar" and x.typeName() == "MeshNode", instances)))
        self.assertTrue(any(filter(lambda x: x.objectName.value() == "Fabric" and x.typeName() == "MeshNode", instances)))
        self.assertTrue(any(filter(lambda x: x.objectName.value() == "Glass" and x.typeName() == "MeshNode", instances)))
        self.assertTrue(any(filter(lambda x: x.objectName.value() == "ToyCar" and x.typeName() == "Mesh", instances)))
        self.assertTrue(any(filter(lambda x: x.objectName.value() == "Fabric" and x.typeName() == "Mesh", instances)))
        self.assertTrue(any(filter(lambda x: x.objectName.value() == "Glass" and x.typeName() == "Mesh", instances)))

    def test_import_gltf_relative_path(self):
        raco.load(self.cwd() + "/../resources/example_scene.rca")
        raco.importGLTF("ToyCar/ToyCar.gltf")
        self.assertToyCarImportWorked()

    def test_import_gltf_relative_path_invalid_project_mesh_path(self):
        raco.load(self.cwd() + "/../resources/example_scene_invalid_mesh_path.rca")
        with self.assertRaises(ValueError):
            raco.importGLTF("ToyCar/ToyCar.gltf")

    def test_import_gltf_relative_path_with_parent(self):
        raco.load(self.cwd() + "/../resources/example_scene.rca")
        parent = raco.create("Node", "parent")
        raco.importGLTF("ToyCar/ToyCar.gltf", parent)
        self.assertToyCarImportWorked()
        self.assertEqual("ToyCar.gltf", parent.children()[0].objectName.value())

    def test_import_gltf_absolute_path(self):
        raco.importGLTF(self.cwd() + R"/../resources/meshes/ToyCar/ToyCar.gltf")
        self.assertToyCarImportWorked()

    def test_import_gltf_invalid_path(self):
        with self.assertRaises(ValueError):
            raco.importGLTF("thisShouldNotExist.gltf")

    def test_get_mesh_metadata(self):
        node = raco.create("Node", "node1")
        self.assertEqual(None, node.metadata())

        mesh = raco.create("Mesh", "mesh1")
        self.assertEqual(None, mesh.metadata())

        mesh.uri = self.cwd() + R"/../resources/meshes/CesiumMilkTruck/CesiumMilkTruck.gltf"
        mesh.bakeMeshes = False
        mesh.meshIndex = 1
        self.assertEqual({'prop1': 'truck mesh property'}, mesh.metadata())

    def test_setTags(self):
        node = raco.create("Node", "node")
        node.setTags(['foo', 'bar'])
        self.assertEqual(node.getTags(), ['foo', 'bar'])

        material = raco.create("Material", "mat")
        material.setTags(['foo', 'bar'])
        self.assertEqual(material.getTags(), ['foo', 'bar'])

        layer = raco.create("RenderLayer", "layer")
        layer.setTags(['foo', 'bar'])
        self.assertEqual(layer.getTags(), ['foo', 'bar'])

    def test_setMaterialFilterTags(self):
        layer = raco.create("RenderLayer", "layer")
        layer.setMaterialFilterTags(['foo', 'bar'])
        self.assertEqual(layer.getMaterialFilterTags(), ['foo', 'bar'])

    def test_setRenderableTags(self):
        layer = raco.create("RenderLayer", "layer")
        self.assertEqual(layer.renderableTags.keys(), [])

        layer.setRenderableTags([("red", 1), ("blue", 3)])

        self.assertEqual(layer.renderableTags.keys(), ["red", "blue"])
        self.assertEqual(layer.renderableTags.red.value(), 1)
        self.assertEqual(layer.renderableTags.blue.value(), 3)

    def test_setUserTags(self):
        # Check type that also has 'tags' property
        node = raco.create("Node", "node")
        node.setUserTags(['foo', 'bar'])
        self.assertEqual(node.getUserTags(), ['foo', 'bar'])

        # Check type that does not have 'tags' property, only 'userTags'
        texture = raco.create("Texture", "texture")
        texture.setUserTags(['foo', 'bar'])
        self.assertEqual(texture.getUserTags(), ['foo', 'bar'])

    def test_feature_level_3_render_order(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"
        
        layer = raco.create("RenderLayer", "layer")
        layer.setRenderableTags([("red", 1), ("blue", 3)])

        self.assertEqual(layer.getRenderableTags(), [("red", 1), ("blue", 3)])
        self.assertEqual(layer.renderableTags.keys(), ["red", "blue"])
        self.assertEqual(layer.renderableTags.red.value(), 1)
        self.assertEqual(layer.renderableTags.blue.value(), 3)

        link_order = raco.addLink(lua.outputs.ointeger, layer.renderableTags.red)

        self.assertEqual(layer.renderableTags.red.value(), 0)
        lua.inputs.integer = 3
        self.assertEqual(layer.renderableTags.red.value(), 6)
        self.assertEqual(layer.getRenderableTags(), [("red", 6), ("blue", 3)])

    def test_add_external_project(self):
        raco.addExternalProject(self.cwd() + R"/../resources/example_scene.rca")

    def test_add_external_project_invalid_path(self):
        with self.assertRaises(ValueError):
            raco.addExternalProject("thisShouldNotExist.rca")

    def test_add_external_references_empty_project(self):
        with self.assertRaises(ValueError):
            raco.addExternalReferences("thisShouldNotExist.rca", "Texture")

    def test_add_external_references(self):
        result = raco.addExternalReferences(self.cwd() + R"/../resources/example_scene.rca", "Texture")
        self.assertEqual(1, len(result))
        self.assertTrue(any(filter(lambda x: x.objectName.value() == "DuckTexture" and x.typeName() == "Texture", result)))

        instances = raco.instances()
        self.assertTrue(any(filter(lambda x: x.objectName.value() == "DuckTexture" and x.typeName() == "Texture", instances)))
        self.assertFalse(any(filter(lambda x: x.objectName.value() == "DuckMaterial" and x.typeName() == "Material", instances)))
        self.assertFalse(any(filter(lambda x: x.objectName.value() == "DuckMesh" and x.typeName() == "Mesh", instances)))

    def test_add_external_references_should_also_add_dependencies(self):
        raco.addExternalReferences(self.cwd() + R"/../resources/example_scene.rca", "Material")
        instances = raco.instances()
        self.assertTrue(any(filter(lambda x: x.objectName.value() == "DuckTexture" and x.typeName() == "Texture", instances)))
        self.assertTrue(any(filter(lambda x: x.objectName.value() == "DuckMaterial" and x.typeName() == "Material", instances)))
        self.assertFalse(any(filter(lambda x: x.objectName.value() == "DuckMesh" and x.typeName() == "Mesh", instances)))

    def test_add_external_references_multiple_times_different_types_should_work(self):
        raco.addExternalReferences(self.cwd() + R"/../resources/example_scene.rca", "Texture")
        raco.addExternalReferences(self.cwd() + R"/../resources/example_scene.rca", "Mesh")
        instances = raco.instances()
        self.assertTrue(any(filter(lambda x: x.objectName.value() == "DuckTexture" and x.typeName() == "Texture", instances)))
        self.assertFalse(any(filter(lambda x: x.objectName.value() == "DuckMaterial" and x.typeName() == "Material", instances)))
        self.assertTrue(any(filter(lambda x: x.objectName.value() == "DuckMesh" and x.typeName() == "Mesh", instances)))

    def test_add_external_references_multiple_types_should_work(self):
        result = raco.addExternalReferences(self.cwd() + R"/../resources/example_scene.rca", ["Texture", "Mesh"])
        self.assertEqual(2, len(result))

        instances = raco.instances()
        self.assertTrue(any(filter(lambda x: x.objectName.value() == "DuckTexture" and x.typeName() == "Texture", instances)))
        self.assertFalse(any(filter(lambda x: x.objectName.value() == "DuckMaterial" and x.typeName() == "Material", instances)))
        self.assertTrue(any(filter(lambda x: x.objectName.value() == "DuckMesh" and x.typeName() == "Mesh", instances)))

    def test_add_external_references_multiple_times_same_types_should_work(self):
        raco.addExternalReferences(self.cwd() + R"/../resources/example_scene.rca", "Texture")
        second_result = raco.addExternalReferences(self.cwd() + R"/../resources/example_scene.rca", "Texture")
        self.assertEqual(0, len(second_result))

    # Export with specified lua save mode, assert success and cleanup
    def export(self, name, save_mode):
        out_dir = self.cwd()
        logic_file = os.path.join(out_dir, f'{name}.rlogic')
        ramses_file = os.path.join(out_dir, f'{name}.ramses')
        if os.path.exists(logic_file):
            os.remove(logic_file)
        try:
            raco.export(
                ramses_file,
                logic_file,
                False,
                save_mode)
            self.assertEqual(os.path.exists(logic_file), True)
        except RuntimeError:
            raise
        finally:
            if os.path.exists(logic_file):
                os.remove(logic_file)
            if os.path.exists(ramses_file):
                os.remove(ramses_file)

    def test_feature_level_2_export_lua_save_mode(self):
        # Add a lua script to project
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"

        # All available modes are supported starting from Feature Level 2
        self.export("lua_SourceCodeOnly", raco.ELuaSavingMode.SourceCodeOnly)
        self.export("lua_ByteCodeOnly", raco.ELuaSavingMode.ByteCodeOnly)
        self.export("lua_SourceAndByteCode", raco.ELuaSavingMode.SourceAndByteCode)

    def test_feature_level_1_export_lua_save_mode(self):
        raco.reset(1)

        # Add a lua script to project
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"

        # In Feature Level 1 only Source Code lua saving mode is supported
        self.export("lua_SourceCodeOnly_FL1", raco.ELuaSavingMode.SourceCodeOnly)
        # Ramses Logic treats SourceAndByteCode as SourceCodeOnly and exports successfully
        self.export("lua_SourceAndByteCode_FL1", raco.ELuaSavingMode.SourceAndByteCode)

        # Ramses Logic rejects ByteCodeOnly
        with self.assertRaises(RuntimeError):
            self.export("lua_ByteCodeOnly_FL1", raco.ELuaSavingMode.ByteCodeOnly)
            
    def test_resolveUriPropertyToAbsolutePath (self):
        # load scene to obtain known current project folder:
        raco.load(self.cwd() + "/../resources/example_scene.rca")
        self.assertEqual(os.path.normpath(raco.projectPath()), os.path.normpath(self.cwd() + "/../resources/example_scene.rca"))

        lua = raco.create("LuaScript", "lua")

        # check absolute path
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"
        self.assertEqual(raco.resolveUriPropertyToAbsolutePath(lua.uri), lua.uri.value())

        # check relative path
        lua.uri = R"../scripts/types-scalar.lua"
        abs_path = raco.resolveUriPropertyToAbsolutePath(lua.uri)
        self.assertEqual(os.path.normpath(abs_path), os.path.normpath(self.cwd() + "/../scripts/types-scalar.lua"))

        with self.assertRaises(RuntimeError):
            raco.resolveUriPropertyToAbsolutePath(lua.objectName)