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

class AnimationTests(unittest.TestCase):
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

    def setup_check_animation_channel(self, name, component_type, interpolation, size, time_stamps, keyframes, tangents_in = None, tangents_out = None):
        channel = raco.create("AnimationChannelRaco", name)
        channel.componentType = component_type
        channel.interpolationType = interpolation
        channel.componentArraySize = size
        if tangents_in == None: 
            channel.setAnimationData(time_stamps, keyframes)
        else:
            channel.setAnimationData(time_stamps, keyframes, tangents_in, tangents_out)

        time_stamps = channel.getAnimationTimeStamps()
        data = channel.getAnimationOutputData()

        self.assertEqual(time_stamps, channel.getAnimationTimeStamps())
        self.assertEqual(keyframes, data[0])
        if tangents_in != None:
            self.assertEqual(tangents_in, data[1])
            self.assertEqual(tangents_out, data[2])

    def test_anim_float_linear(self):
        self.setup_check_animation_channel(
            "channel_float_linear", raco.EAnimationComponentType.Float, raco.EAnimationInterpolationType.Linear, 0, 
            [0,0.5,1], [4, 5, 6])

    def test_anim_float_cubic(self):
        self.setup_check_animation_channel(
            "channel_float_cubic", raco.EAnimationComponentType.Float, raco.EAnimationInterpolationType.CubicSpline, 0, 
            [0,0.5,1], [4, 5, 6], [0,0,0], [1,1,1])

    def test_anim_vec2f_linear(self):
        self.setup_check_animation_channel(
            "channel_vec2f_linear", raco.EAnimationComponentType.Vec2f, raco.EAnimationInterpolationType.Linear, 0,
            [0,0.5,1], [[4, 5], [5,6], [6,7]])

    def test_anim_vec2f_cubic(self):
        self.setup_check_animation_channel(
            "channel_vec2f_cubic", raco.EAnimationComponentType.Vec2f, raco.EAnimationInterpolationType.CubicSpline, 0,
            [0,0.5,1], [[4.0,5], [5,6], [6,7]], [[0,0], [0,0], [0,0]], [[1,1], [1,1], [1,1]])

    def test_anim_vec3f_linear(self):
        self.setup_check_animation_channel(
            "channel_vec3f_linear", raco.EAnimationComponentType.Vec3f, raco.EAnimationInterpolationType.Linear, 0,
            [0,0.5,1], [[4, 5, 6], [5,6,7], [6,7,8]])

    def test_anim_vec3f_cubic(self):
        self.setup_check_animation_channel(
            "channel_vec3f_cubic", raco.EAnimationComponentType.Vec3f, raco.EAnimationInterpolationType.CubicSpline, 0,
            [0,0.5,1], [[4.0,5,6], [5,6,7], [6,7,8]], [[0,0,0], [0,0,0], [0,0,0]], [[1,1,1], [1,1,1], [1,1,1]])
            
    def test_anim_vec4f_linear(self):
        self.setup_check_animation_channel(
            "channel_vec4f_linear", raco.EAnimationComponentType.Vec4f, raco.EAnimationInterpolationType.Linear, 0,
            [0,0.5,1], [[4, 5, 6, 7], [5,6,7,8], [6,7,8,9]])

    def test_anim_vec4f_cubic(self):
        self.setup_check_animation_channel(
            "channel_vec4f_cubic", raco.EAnimationComponentType.Vec4f, raco.EAnimationInterpolationType.CubicSpline, 0,
            [0,0.5,1], [[4, 5, 6, 7], [5,6,7,8], [6,7,8,9]], [[0,0,0,0], [0,0,0,0], [0,0,0,0]], [[1,1,1,1], [1,1,1,1], [1,1,1,1]])

    def test_anim_array_linear(self):
        self.setup_check_animation_channel(
            "channel_array_linear", raco.EAnimationComponentType.Array, raco.EAnimationInterpolationType.Linear, 5,
            [0,0.5,1], [[4.0,5,6,7,8], [5,6,7,8,9], [6,7,8,9,10]])

    def test_anim_array_cubic(self):
        self.setup_check_animation_channel(
            "channel_array_cubic", raco.EAnimationComponentType.Array, raco.EAnimationInterpolationType.CubicSpline, 5,
            [0,0.5,1], [[4.0,5,6,7,8], [5,6,7,8,9], [6,7,8,9,10]], [[0,0,0,0,0], [0,0,0,0,0], [0,0,0,0,0]], [[1,1,1,1,1], [1,1,1,1,1], [1,1,1,1,1]])

    def test_anim_int_linear(self):
        self.setup_check_animation_channel(
            "channel_int_linear", raco.EAnimationComponentType.Int, raco.EAnimationInterpolationType.Linear, 0, 
            [0,0.5,1], [4, 5, 6])

    def test_anim_int_cubic(self):
        self.setup_check_animation_channel(
            "channel_int_cubic", raco.EAnimationComponentType.Int, raco.EAnimationInterpolationType.CubicSpline, 0, 
            [0,0.5,1], [4, 5, 6], [0,0,0], [1,1,1])

    def test_anim_vec2i_linear(self):
        self.setup_check_animation_channel(
            "channel_vec2i_linear", raco.EAnimationComponentType.Vec2i, raco.EAnimationInterpolationType.Linear, 0,
            [0,0.5,1], [[4, 5], [5,6], [6,7]])

    def test_anim_vec2i_cubic(self):
        self.setup_check_animation_channel(
            "channel_vec2i_cubic", raco.EAnimationComponentType.Vec2i, raco.EAnimationInterpolationType.CubicSpline, 0,
            [0,0.5,1], [[4,5], [5,6], [6,7]], [[0,0], [0,0], [0,0]], [[1,1], [1,1], [1,1]])

    def test_anim_vec3i_linear(self):
        self.setup_check_animation_channel(
            "channel_vec3i_linear", raco.EAnimationComponentType.Vec3i, raco.EAnimationInterpolationType.Linear, 0,
            [0,0.5,1], [[4, 5, 6], [5,6,7], [6,7,8]])

    def test_anim_vec3i_cubic(self):
        self.setup_check_animation_channel(
            "channel_vec3i_cubic", raco.EAnimationComponentType.Vec3i, raco.EAnimationInterpolationType.CubicSpline, 0,
            [0,0.5,1], [[4,5,6], [5,6,7], [6,7,8]], [[0,0,0], [0,0,0], [0,0,0]], [[1,1,1], [1,1,1], [1,1,1]])
            
    def test_anim_vec4i_linear(self):
        self.setup_check_animation_channel(
            "channel_vec4i_linear", raco.EAnimationComponentType.Vec4i, raco.EAnimationInterpolationType.Linear, 0,
            [0,0.5,1], [[4, 5, 6, 7], [5,6,7,8], [6,7,8,9]])

    def test_anim_vec4i_cubic(self):
        self.setup_check_animation_channel(
            "channel_vec4i_cubic", raco.EAnimationComponentType.Vec4i, raco.EAnimationInterpolationType.CubicSpline, 0,
            [0,0.5,1], [[4, 5, 6, 7], [5,6,7,8], [6,7,8,9]], [[0,0,0,0], [0,0,0,0], [0,0,0,0]], [[1,1,1,1], [1,1,1,1], [1,1,1,1]])
