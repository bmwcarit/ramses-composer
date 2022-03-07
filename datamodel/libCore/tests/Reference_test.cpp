/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/BasicTypes.h"
#include "data_storage/Value.h"

#include "user_types/Node.h"
#include "user_types/MeshNode.h"
#include "user_types/Mesh.h"

#include <memory>
#include <stdexcept>
#include <tuple>
#include <vector>

#include "gtest/gtest.h"

using namespace raco::data_storage;
using namespace raco::user_types;

TEST(ReferenceTest, Value) {
	SNode node { new Node("node")};
	SNode node_b{new Node("node b")};
	SMeshNode meshnode { new MeshNode("meshnode")};
	SMeshNode meshnode_b{new MeshNode("meshnode b")};
	SMesh mesh { new Mesh("mesh")};

	Value<SNode> vnode = node;
	Value<SNode> vnode_b = node_b;
	Value<SMeshNode> vmeshnode = meshnode;
	Value<SMeshNode> vmeshnode_b = meshnode_b;
	Value<SMesh> vmesh = mesh;

	// Assigning shared_ptr<T> to Value<U>s

	// assign same class
	EXPECT_TRUE(vnode.canSetRef(node_b));
	vnode = node_b;
	EXPECT_EQ(*vnode, node_b);

	// assign unrelated class: doesn't compile:
	//vmesh = node;
	EXPECT_FALSE(vmesh.canSetRef(node));

	// assign child class 
	EXPECT_TRUE(vnode.canSetRef(meshnode_b));
	vnode = meshnode_b;
	EXPECT_EQ(*vnode, meshnode_b);

	// assign parent class via ValueBase& 
	EXPECT_TRUE(vmeshnode.canSetRef(meshnode_b));
	static_cast<ValueBase&>(vmeshnode) = meshnode_b;
	EXPECT_EQ(*vmeshnode, meshnode_b);

	EXPECT_FALSE(vmeshnode.canSetRef(node));
	EXPECT_THROW(static_cast<ValueBase&>(vmeshnode) = node, std::runtime_error);


	// restore test setup
	vnode = node;
	vmeshnode = meshnode;
	 

	// Assigning Value<T> to Value<U>
	
	// assign same class
	vnode = vnode_b;
	EXPECT_EQ(*vnode, node_b);

	// assign child class
	vnode = vmeshnode;
	EXPECT_EQ(*vnode, meshnode);

	// assign unrelated class
	EXPECT_THROW(vmesh = vnode, std::runtime_error);

	vnode = node;


	// Assigning ValueBase to ValueBase

	// assign parent class type containing parent instance pointer -> exception
	EXPECT_THROW(vmeshnode = vnode, std::runtime_error);

	// assign parent class type containing same type instance pointer -> works
	vnode = meshnode_b;
	vmeshnode = vnode;
	EXPECT_EQ(*vmeshnode, meshnode_b);
	vnode = node;
	vmeshnode = meshnode;


	// Check behaviour of cloned ValueBase objects
	auto vnode_clone{vnode.clone(nullptr)};

	EXPECT_THROW(*vnode_clone = mesh, std::runtime_error);
	*vnode_clone = meshnode;
	EXPECT_EQ(vnode_clone->asRef(), meshnode);

	auto vmeshnode_clone {vmeshnode.clone(nullptr)};
	*vmeshnode_clone = meshnode_b;
	EXPECT_EQ(vmeshnode_clone->asRef(), meshnode_b);

	EXPECT_THROW(*vmeshnode_clone = node, std::runtime_error);
}


TEST(ReferenceTest, Property) {
	SNode node{new Node("node")};
	SNode node_b{new Node("node b")};
	SMeshNode meshnode{new MeshNode("meshnode")};
	SMeshNode meshnode_b{new MeshNode("meshnode b")};
	SMesh mesh{new Mesh("mesh")};

	Property<SNode> vnode = node;
	Property<SNode> vnode_b = node_b;
	Property<SMeshNode> vmeshnode = meshnode;
	Property<SMeshNode> vmeshnode_b = meshnode_b;
	Property<SMesh> vmesh = mesh;

	// Assigning shared_ptr<T> to Property<U>s

	// assign unrelated class: doesn't compile:
	// vmesh = node;

	// assign child class
	vnode = meshnode;
	EXPECT_EQ(*vnode, meshnode);

	// restore test setup
	vnode = node;
	EXPECT_EQ(*vnode, node);


	// Assigning Property<T> to Property<U>

	// assign same class
	vnode = vnode_b;
	EXPECT_EQ(*vnode, node_b);

	// assign child class
	vnode = vmeshnode;
	EXPECT_EQ(*vnode, meshnode);

	// assign unrelated class: doesn't compile
	// vmesh = vnode;

	vnode = node;

	// assign parent class type containing parent instance pointer
	// direct: doesn't compile:
	// vmeshnode = vnode;
	// indirect via ValueBase& -> exception
	EXPECT_THROW(static_cast<ValueBase&>(vmeshnode) = static_cast<ValueBase&>(vnode), std::runtime_error);

	// assign parent class type containing same type instance pointer -> works
	vnode = meshnode_b;
	// direct: still doesn't compile:
	// vmeshnode = vnode;
	// indirect via ValueBase& -> works
	static_cast<ValueBase&>(vmeshnode) = static_cast<ValueBase&>(vnode);
	EXPECT_EQ(*vmeshnode, meshnode_b);
}

TEST(ReferenceTest, TypeEquality) {
	SNode node{new Node("node")};
	SNode node2{new Node("node2")};
	SMeshNode meshnode{new MeshNode("meshnode")};

	Property<SNode> pnode = node;
	Property<SNode> pnode2 = node2;
	Value<SNode> vnode = node;
	Property<SNode, RangeAnnotation<double>> pnoderange{node, RangeAnnotation<double>()};
	Property<SNode, HiddenProperty> pnodehidden {node, HiddenProperty()};
	Property<SMeshNode> pmeshnode = meshnode;

	EXPECT_TRUE(ValueBase::classesEqual(pnode, pnode2));
	EXPECT_FALSE(ValueBase::classesEqual(vnode, pnode));
	EXPECT_FALSE(ValueBase::classesEqual(pnode, pmeshnode));

	EXPECT_FALSE(ValueBase::classesEqual(pnode, pnoderange));
	EXPECT_FALSE(ValueBase::classesEqual(pnode, pnodehidden));

	auto pnodehidden_clone{pnodehidden.clone(nullptr)};
	EXPECT_TRUE(ValueBase::classesEqual(pnodehidden, *pnodehidden_clone));
}

