/*
Copyright 2026 Spalishe

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// simple property representation
struct Prop
{
	std::string name;
	std::vector<uint8_t> data;
};

// forward
struct Node;

// Node type
struct Node
{
	std::string name;			// node name, can be empty for root
	Node* parent	 = nullptr; // parent pointer, nullptr for root
	uint32_t phandle = 0;		// allocated phandle (0 = unassigned)
	std::vector<Prop> props;
	std::vector<std::unique_ptr<Node>> children;

	explicit Node(const std::string& name_ = "");
	// create name like "dev@1000"
	static std::unique_ptr<Node> create_reg(const std::string& name, uint64_t addr);

	// attach child (takes ownership)
	void add_child(std::unique_ptr<Node> child);

	// find child by exact name
	Node* find(const std::string& name);

	// find child by name@addr
	Node* find_reg(const std::string& name, uint64_t addr);

	// find child by name@* (first matching)
	Node* find_reg_any(const std::string& name);

	// get (and allocate if needed) phandle; returns 0 for root node
	uint32_t get_phandle();

	// properties
	void add_prop(const std::string& name, const void* data, uint32_t len);
	void add_prop_u32(const std::string& name, uint32_t val);
	void add_prop_u64(const std::string& name, uint64_t val);
	void add_prop_cells(const std::string& name, std::vector<uint32_t> cells, uint32_t count);
	void add_prop_str(const std::string& name, const std::string& val);
	void add_prop_reg(const std::string& name, uint64_t begin, uint64_t size);

	// queries
	const void* get_prop_data(const std::string& name);
	size_t get_prop_size(const std::string& name);
	bool del_prop(const std::string& name);

	// free - unique_ptr handles cleanup; but provide recursive free helper if needed
	static void free_node(std::unique_ptr<Node>& n);
};

// Serialization helpers
size_t size_of_tree(Node* node);
size_t serialize(Node* node, void* buffer, size_t size, uint32_t boot_cpuid);

struct fdt_node;
// creation
fdt_node* fdt_node_create(const char* name);
fdt_node* fdt_node_create_reg(const char* name, uint64_t addr);
void fdt_node_add_child(fdt_node* node, fdt_node* child);
fdt_node* fdt_node_find(fdt_node* node, const char* name);
fdt_node* fdt_node_find_reg(fdt_node* node, const char* name, uint64_t addr);
fdt_node* fdt_node_find_reg_any(fdt_node* node, const char* name);
uint32_t fdt_node_get_phandle(fdt_node* node);

void fdt_node_add_prop(fdt_node* node, const char* name, const void* data, uint32_t len);
void fdt_node_add_prop_u32(fdt_node* node, const char* name, uint32_t val);
void fdt_node_add_prop_u64(fdt_node* node, const char* name, uint64_t val);
void fdt_node_add_prop_cells(fdt_node* node, const char* name, std::vector<uint32_t> cells, uint32_t count);
void fdt_node_add_prop_str(fdt_node* node, const char* name, const char* val);
void fdt_node_add_prop_reg(fdt_node* node, const char* name, uint64_t begin, uint64_t size);

void* fdt_node_get_prop_data(fdt_node* node, const char* name);
size_t fdt_node_get_prop_size(fdt_node* node, const char* name);
bool fdt_node_del_prop(fdt_node* node, const char* name);

void fdt_node_free(fdt_node* node);
size_t fdt_size(fdt_node* node);
size_t fdt_serialize(fdt_node* node, void* buffer, size_t size, uint32_t boot_cpuid);
