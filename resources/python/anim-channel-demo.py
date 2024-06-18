import sys
import raco
import os

def print_data(obj):
    print(obj.objectName.value())
    print("  timeStamps: ", obj.getAnimationTimeStamps())
    data = obj.getAnimationOutputData()
    print("  keyframes:  ", data[0])
    print("  tangentIn:  ", data[1])
    print("  tangentOut: ", data[2])
    print()

# Float Linear
channel = raco.create("AnimationChannelRaco", "channel_float_linear")
channel.componentType = raco.EAnimationComponentType.Float
channel.interpolationType = raco.EAnimationInterpolationType.Linear
channel.setAnimationData([0,0.5,1], [4, 5, 6])
print_data(channel)

# Float Cubic 
channel_f_c = raco.create("AnimationChannelRaco", "channel_float_cubic")
channel_f_c.componentType = raco.EAnimationComponentType.Float
channel_f_c.interpolationType = raco.EAnimationInterpolationType.CubicSpline
channel_f_c.setAnimationData([0,0.5,1], [4, 5,6], [0,0,0], [1,1,1])
print_data(channel_f_c)

# Vec2f linear
channel_v2f = raco.create("AnimationChannelRaco", "channel_vec2f_linear")
channel_v2f.componentType = raco.EAnimationComponentType.Vec2f
channel_v2f.interpolationType = raco.EAnimationInterpolationType.Linear
channel_v2f.setAnimationData([0,0.5,1], [[4, 5], [5,6], [6,7]])
print_data(channel_v2f)

# Vec2f cubic
channel_v2f_c = raco.create("AnimationChannelRaco", "channel_vec2f_cubic")
channel_v2f_c.componentType = raco.EAnimationComponentType.Vec2f
channel_v2f_c.interpolationType = raco.EAnimationInterpolationType.CubicSpline
channel_v2f_c.setAnimationData([0,0.5,1], [[4.0,5], [5,6], [6,7]], [[0,0], [0,0], [0,0]], [[1,1], [1,1], [1,1]])
print_data(channel_v2f_c)

# Vec3f linear
channel_v3f = raco.create("AnimationChannelRaco", "channel_vec3f_linear")
channel_v3f.componentType = raco.EAnimationComponentType.Vec3f
channel_v3f.interpolationType = raco.EAnimationInterpolationType.Linear
channel_v3f.setAnimationData([0,0.5,1], [[4, 5, 6], [5,6,7], [6,7,8]])
print_data(channel_v3f)

# Vec3f cubic
channel_v3f_c = raco.create("AnimationChannelRaco", "channel_vec3f_cubic")
channel_v3f_c.componentType = raco.EAnimationComponentType.Vec3f
channel_v3f_c.interpolationType = raco.EAnimationInterpolationType.CubicSpline
channel_v3f_c.setAnimationData([0,0.5,1], [[4.0,5,6], [5,6,7], [6,7,8]], [[0,0,0], [0,0,0], [0,0,0]], [[1,1,1], [1,1,1], [1,1,1]])
print_data(channel_v3f_c)

# Vec4f linear
channel_v4f = raco.create("AnimationChannelRaco", "channel_vec4f_linear")
channel_v4f.componentType = raco.EAnimationComponentType.Vec4f
channel_v4f.interpolationType = raco.EAnimationInterpolationType.Linear
channel_v4f.setAnimationData([0,0.5,1], [[4, 5, 6, 7], [5,6,7,8], [6,7,8,9]])
print_data(channel_v4f)

# Vec4f cubic
channel_v4f_c = raco.create("AnimationChannelRaco", "channel_vec4f_cubic")
channel_v4f_c.componentType = raco.EAnimationComponentType.Vec4f
channel_v4f_c.interpolationType = raco.EAnimationInterpolationType.CubicSpline
channel_v4f_c.setAnimationData([0,0.5,1], [[4, 5, 6, 7], [5,6,7,8], [6,7,8,9]], [[0,0,0,0], [0,0,0,0], [0,0,0,0]], [[1,1,1,1], [1,1,1,1], [1,1,1,1]])
print_data(channel_v4f_c)


# Array(float) linear
channel_a = raco.create("AnimationChannelRaco", "channel_array_linear")
channel_a.componentType = raco.EAnimationComponentType.Array
channel_a.interpolationType = raco.EAnimationInterpolationType.Linear
channel_a.componentArraySize = 5
channel_a.setAnimationData([0,0.5,1], [[4.0,5,6,7,8], [5,6,7,8,9], [6,7,8,9,10]])
print_data(channel_a)

# Array(float) cubic
channel_a_c = raco.create("AnimationChannelRaco", "channel_array_cubic")
channel_a_c.componentType = raco.EAnimationComponentType.Array
channel_a_c.interpolationType = raco.EAnimationInterpolationType.CubicSpline
channel_a_c.componentArraySize = 5
channel_a_c.setAnimationData([0,0.5,1], [[4.0,5,6,7,8], [5,6,7,8,9], [6,7,8,9,10]], [[0,0,0,0,0], [0,0,0,0,0], [0,0,0,0,0]], [[1,1,1,1,1], [1,1,1,1,1], [1,1,1,1,1]])
print_data(channel_a_c)


# Int Linear
channel_i = raco.create("AnimationChannelRaco", "channel_int_linear")
channel_i.componentType = raco.EAnimationComponentType.Int
channel_i.interpolationType = raco.EAnimationInterpolationType.Linear
channel_i.setAnimationData([0,0.5,1], [4, 5, 6])
print_data(channel_i)

# Int Cubic
channel_f_c = raco.create("AnimationChannelRaco", "channel_int_cubic")
channel_f_c.componentType = raco.EAnimationComponentType.Int
channel_f_c.interpolationType = raco.EAnimationInterpolationType.CubicSpline
channel_f_c.setAnimationData([0,0.5,1], [4, 5,6], [0,0,0], [1,1,1])
print_data(channel_f_c)

# Vec2i linear
channel_v2i = raco.create("AnimationChannelRaco", "channel_vec2i_linear")
channel_v2i.componentType = raco.EAnimationComponentType.Vec2i
channel_v2i.interpolationType = raco.EAnimationInterpolationType.Linear
channel_v2i.setAnimationData([0,0.5,1], [[4, 5], [5,6], [6,7]])
print_data(channel_v2i)

# Vec2i cubic
channel_v2i_c = raco.create("AnimationChannelRaco", "channel_vec2i_cubic")
channel_v2i_c.componentType = raco.EAnimationComponentType.Vec2i
channel_v2i_c.interpolationType = raco.EAnimationInterpolationType.CubicSpline
channel_v2i_c.setAnimationData([0,0.5,1], [[4,5], [5,6], [6,7]], [[0,0], [0,0], [0,0]], [[1,1], [1,1], [1,1]])
print_data(channel_v2i_c)

# Vec3i linear
channel_v3i = raco.create("AnimationChannelRaco", "channel_vec3i_linear")
channel_v3i.componentType = raco.EAnimationComponentType.Vec3i
channel_v3i.interpolationType = raco.EAnimationInterpolationType.Linear
channel_v3i.setAnimationData([0,0.5,1], [[4, 5, 6], [5,6,7], [6,7,8]])
print_data(channel_v3i)

# Vec3i cubic
channel_v3i_c = raco.create("AnimationChannelRaco", "channel_vec3i_cubic")
channel_v3i_c.componentType = raco.EAnimationComponentType.Vec3i
channel_v3i_c.interpolationType = raco.EAnimationInterpolationType.CubicSpline
channel_v3i_c.setAnimationData([0,0.5,1], [[4,5,6], [5,6,7], [6,7,8]], [[0,0,0], [0,0,0], [0,0,0]], [[1,1,1], [1,1,1], [1,1,1]])
print_data(channel_v3i_c)

# Vec4i linear
channel_v4i = raco.create("AnimationChannelRaco", "channel_vec4i_linear")
channel_v4i.componentType = raco.EAnimationComponentType.Vec4i
channel_v4i.interpolationType = raco.EAnimationInterpolationType.Linear
channel_v4i.setAnimationData([0,0.5,1], [[4, 5, 6, 7], [5,6,7,8], [6,7,8,9]])
print_data(channel_v4i)

# Vec4i cubic
channel_v4i_c = raco.create("AnimationChannelRaco", "channel_vec4i_cubic")
channel_v4i_c.componentType = raco.EAnimationComponentType.Vec4i
channel_v4i_c.interpolationType = raco.EAnimationInterpolationType.CubicSpline
channel_v4i_c.setAnimationData([0,0.5,1], [[4, 5, 6, 7], [5,6,7,8], [6,7,8,9]], [[0,0,0,0], [0,0,0,0], [0,0,0,0]], [[1,1,1,1], [1,1,1,1], [1,1,1,1]])
print_data(channel_v4i_c)
