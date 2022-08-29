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

class FeatureLevel_1(unittest.TestCase):
    def setUp(self):
        raco.reset()

    def cwd(self):
        return os.getcwd()

    def test_node_enabled_fail(self):
        node = raco.create("Node", "node")
        self.assertEqual(dir(node), ['objectName', 'rotation', 'scaling', 'translation', 'visibility'])

        with self.assertRaises(RuntimeError):
            tmp = node.enabled.value()

        with self.assertRaises(RuntimeError):
            node.enabled = False


    def test_anchor_point_fail(self):
        with self.assertRaises(RuntimeError):
            anchor = raco.create("AnchorPoint", "test_anchor")
            
            
    def test_perspective_camera_frustum_fail(self):
        camera = raco.create("PerspectiveCamera", "camera")

        with self.assertRaises(RuntimeError):
            tmp = camera.frustumType.value()
            
        with self.assertRaises(RuntimeError):
            camera.frustumType = raco.EFrustumType.Planes
        
        with self.assertRaises(RuntimeError):
            camera.frustum.leftPlane = 1.0

    def test_renderpass_prop_fail(self):
        renderpass = raco.create("RenderPass", "render_pass")
        
        with self.assertRaises(RuntimeError):
            dummy = renderpass.renderOnce.value()
            
        with self.assertRaises(RuntimeError):
            renderpass.renderOnce = True

    def test_renderpass_link_fail(self):
        lua = raco.create("LuaScript", "lua")
        lua.uri = self.cwd() + R"/../resources/scripts/types-scalar.lua"
        
        renderpass = raco.create("RenderPass", "render_pass")

        self.assertEqual(renderpass.enabled.value(), True)
        with self.assertRaises(RuntimeError):
            raco.addLink(lua.outputs.obool, renderpass.enabled)
        
        self.assertEqual(renderpass.renderOrder.value(), 1)
        with self.assertRaises(RuntimeError):
            raco.addLink(lua.outputs.ointeger, renderpass.renderOrder)
        
        self.assertEqual(renderpass.clearColor.x.value(), 0.0)
        with self.assertRaises(RuntimeError):
            raco.addLink(lua.outputs.ovector4f, renderpass.clearColor)
        
        