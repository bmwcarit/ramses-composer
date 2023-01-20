import raco

def find_object(name):
	for obj in raco.instances():
		if obj.objectName.value() == name:
			return obj

def printProperties(handle, depth = 1):
    for name in handle.keys():
        childHandle = handle[name]
        if childHandle.hasSubstructure():
            print("    " * depth, name, "  ", childHandle)
            printProperties(childHandle, depth + 1)
        else:
            print("    " * depth, name, "  ", childHandle, "=", childHandle.value())

def create_node(parent, shift, mesh, material, texture_filename, texture_format, name):
    print(name)
    texture = raco.create("Texture", name)
    texture.uri = texture_filename
    texture.textureFormat = texture_format
    #printProperties(texture)

    meshnode = raco.create("MeshNode", name)
    meshnode.translation.x = shift
    raco.moveScenegraph(meshnode, parent)
    meshnode.mesh = mesh
    meshnode.materials.material.material = material
    meshnode.materials.material.private = True
    meshnode.materials.material.uniforms.u_Tex = texture
    meshnode.materials.material.options.blendOperationColor = raco.EBlendOperation.Add
    meshnode.materials.material.options.blendOperationAlpha = raco.EBlendOperation.Add
    #printProperties(meshnode)

root = find_object("root-node")
    
mesh = find_object("Mesh")    
material = find_object("Material")
 
def create_group_8bit(parent, vert_shift, delta, mesh, material, filename, name): 
    row_node = raco.create("Node", name)
    row_node.translation.y = vert_shift
    raco.moveScenegraph(row_node, parent)
    
    offset = -1.5 * delta
    create_node(row_node, offset, mesh, material, filename, raco.ETextureFormat.R8, name + "-r8")
    create_node(row_node, offset + delta, mesh, material, filename, raco.ETextureFormat.RG8, name + "-rg8")
    create_node(row_node, offset + 2*delta, mesh, material, filename, raco.ETextureFormat.RGB8, name + "-rgb8")
    create_node(row_node, offset + 3*delta, mesh, material, filename, raco.ETextureFormat.RGBA8, name + "-rgba8")

root_8 = raco.create("Node", "8bit")
raco.moveScenegraph(root_8, root)

#create_group_8bit(root_8, 3, mesh, material, "images/PngSuite-2017jul19/basn2c08.png", "rgb")
#create_group_8bit(root_8, 3, mesh, material, "images/PngSuite-2017jul19/basn6a08.png", "rgba")
#create_group_8bit(root_8, 3, mesh, material, "images/PngSuite-2017jul19/basn0g08.png", "r")
#create_group_8bit(root_8, 3, mesh, material, "images/PngSuite-2017jul19/basn4a08.png", "rg")
#create_group_8bit(root_8, 3, mesh, material, "images/PngSuite-2017jul19/basn3p08.png", "rgb-p")


def create_8bit_data(delta):
    data = [
         ("basn0g08.png", "g-8bit"),
         ("basn4a08.png", "ga-8bit"),
         ("basn2c08.png", "rgb-8bit"),
         ("basn6a08.png", "rgba-8bit"),
         ("basn3p08.png", "rgb-pal")]
    offset = (len(data) - 1)/2.0 * delta
    for ((filename, name), index) in zip(data, range(len(data))):
        create_group_8bit(root_8, offset - delta * index, delta, mesh, material, "images/PngSuite-2017jul19/" + filename, name)
        
create_8bit_data(3.0)

def create_group_16bit(parent, vert_shift, mesh, material, filename, name): 
    row_node = raco.create("Node", name)
    row_node.translation.y = vert_shift
    raco.moveScenegraph(row_node, parent)
    
    create_node(row_node, +1.5, mesh, material, filename, raco.ETextureFormat.RGB16F, name + "-rgb16")
    create_node(row_node, +4.5, mesh, material, filename, raco.ETextureFormat.RGBA16F, name + "-rgba16")

root_16 = raco.create("Node", "16bit")
raco.moveScenegraph(root_16, root)

create_group_16bit(root_16, +4.5, mesh, material, "images/PngSuite-2017jul19/basn2c16.png", "rgb")
create_group_16bit(root_16, +1.5, mesh, material, "images/PngSuite-2017jul19/basn6a16.png", "rgba")
#create_group_16bit(root_16, -1.5, mesh, material, "images/PngSuite-2017jul19/basn0g16.png", "r")
#create_group_16bit(root_16, -4.5, mesh, material, "images/PngSuite-2017jul19/basn4a16.png", "rg")
