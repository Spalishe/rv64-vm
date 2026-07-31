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

#include "../include/libfdt.h"
#include <cassert>
#include <cstring>

// --- internal helpers ----------------------------------------------------

static inline uint32_t to_be32(uint32_t v)
{
	return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8) | ((v & 0x00FF0000u) >> 8) | ((v & 0xFF000000u) >> 24);
}

static inline uint64_t to_be64(uint64_t v)
{
	return ((uint64_t)to_be32(uint32_t(v & 0xFFFFFFFFull)) << 32) | (uint64_t)to_be32(uint32_t((v >> 32) & 0xFFFFFFFFull));
}

static inline size_t align_up(size_t v, size_t align)
{
	return (v + align - 1) & ~(align - 1);
}

static void write_be32_to_buf(uint8_t* dst, uint32_t v)
{
	uint32_t be = to_be32(v);
	std::memcpy(dst, &be, sizeof(be));
}
static void write_be64_to_buf(uint8_t* dst, uint64_t v)
{
	uint64_t be = to_be64(v);
	std::memcpy(dst, &be, sizeof(be));
}

static std::string name_with_addr(const std::string& name, uint64_t addr)
{
	// produce "name@hexaddr" (lowercase hex)
	char buf[64];
	// we use lowercase hex without 0x, mimic original behaviour
	std::snprintf(buf, sizeof(buf), "%s@%llx", name.c_str(), (unsigned long long)addr);
	return std::string(buf);
}

// --- Node implementation ------------------------------------------------

Node::Node(const std::string& name_) : name(name_), parent(nullptr)
{
}

std::unique_ptr<Node> Node::create_reg(const std::string& name, uint64_t addr)
{
	return std::make_unique<Node>(name_with_addr(name, addr));
}

void Node::add_child(std::unique_ptr<Node> child)
{
	if(!child) return;
	child->parent = this;
	children.push_back(std::move(child));
}

Node* Node::find(const std::string& name_)
{
	for(auto& c : children)
	{
		if(c->name == name_) return c.get();
	}
	return nullptr;
}

Node* Node::find_reg(const std::string& name_, uint64_t addr)
{
	std::string n = name_with_addr(name_, addr);
	return find(n);
}

Node* Node::find_reg_any(const std::string& name_)
{
	std::string prefix = name_ + "@";
	for(auto& c : children)
	{
		if(c->name.rfind(prefix, 0) == 0)
		{ // startswith
			return c.get();
		}
	}
	return nullptr;
}

static bool is_illegal_phandle(uint32_t ph)
{
	return ph == 0 || ph == 0xFFFFFFFFu;
}

static uint32_t allocate_new_phandle(Node* node)
{
	// trace to root
	Node* r = node;
	while(r->parent)
		r = r->parent;
	if(!r->name.empty())
	{
		// std::cerr << "fdt: allocate_new_phandle: root node has name; invalid hierarchy\n";
	}
	// increment last phandle stored in root
	r->phandle++;
	return r->phandle;
}

uint32_t Node::get_phandle()
{
	if(parent == NULL || name.empty())
	{
		// root node: no phandle
		return 0;
	}
	if(is_illegal_phandle(phandle))
	{
		phandle = allocate_new_phandle(this);
		// also add property "phandle"
		add_prop_u32("phandle", phandle);
	}
	return phandle;
}

// property helpers
void Node::add_prop(const std::string& name_, const void* data, uint32_t len)
{
	if(name_.empty()) return;
	// replace if exists
	for(auto& p : props)
	{
		if(p.name == name_)
		{
			p.data.assign((const uint8_t*)data, (const uint8_t*)data + len);
			return;
		}
	}
	Prop np;
	np.name = name_;
	np.data.assign((const uint8_t*)data, (const uint8_t*)data + len);
	props.push_back(std::move(np));
}

void Node::add_prop_u32(const std::string& name_, uint32_t val)
{
	uint8_t buf[4];
	write_be32_to_buf(buf, val);
	add_prop(name_, buf, sizeof(buf));
}

void Node::add_prop_u64(const std::string& name_, uint64_t val)
{
	uint8_t buf[8];
	write_be64_to_buf(buf, val);
	add_prop(name_, buf, sizeof(buf));
}

void Node::add_prop_cells(const std::string& name_, std::vector<uint32_t> cells, uint32_t count)
{
	if(count == 0)
	{
		add_prop(name_, nullptr, 0);
		return;
	}
	std::vector<uint8_t> buf(count * 4);
	for(uint32_t i = 0; i < count; ++i)
	{
		write_be32_to_buf(buf.data() + i * 4, cells[i]);
	}
	add_prop(name_, buf.data(), (uint32_t)buf.size());
}

void Node::add_prop_str(const std::string& name_, const std::string& val)
{
	add_prop(name_, val.c_str(), (uint32_t)(val.size() + 1));
}

void Node::add_prop_reg(const std::string& name_, uint64_t begin, uint64_t size)
{
	uint8_t buf[16];
	write_be64_to_buf(buf + 0, begin);
	write_be64_to_buf(buf + 8, size);
	add_prop(name_, buf, sizeof(buf));
}

const void* Node::get_prop_data(const std::string& name_)
{
	for(auto& p : props)
		if(p.name == name_) return p.data.empty() ? nullptr : p.data.data();
	return nullptr;
}

size_t Node::get_prop_size(const std::string& name_)
{
	for(auto& p : props)
		if(p.name == name_) return p.data.size();
	return 0;
}

bool Node::del_prop(const std::string& name_)
{
	for(size_t i = 0; i < props.size(); ++i)
	{
		if(props[i].name == name_)
		{
			props.erase(props.begin() + i);
			return true;
		}
	}
	return false;
}

void Node::free_node(std::unique_ptr<Node>& n)
{
	n.reset();
}

// --- serialization ------------------------------------------------------

static const uint32_t FDT_MAGIC		   = 0xD00DFEED;
static const uint32_t FDT_VERSION	   = 17;
static const uint32_t FDT_COMP_VERSION = 16;
static const uint32_t FDT_BEGIN_NODE   = 1;
static const uint32_t FDT_END_NODE	   = 2;
static const uint32_t FDT_PROP		   = 3;
static const uint32_t FDT_NOP		   = 4;
static const uint32_t FDT_END		   = 9;
static const uint32_t FDT_HDR_SIZE	   = 40;
static const uint32_t FDT_RSV_SIZE	   = 16;

struct size_desc
{
	uint32_t struct_size = 0;
	uint32_t string_size = 0;
};

static void get_tree_size(Node* node, size_desc& desc)
{
	size_t name_len = align_up(node->name.empty() ? 1 : node->name.size() + 1, sizeof(uint32_t));
	desc.struct_size += sizeof(uint32_t) + (uint32_t)name_len; // begin node + name

	for(auto& p : node->props)
	{
		desc.struct_size += sizeof(uint32_t) * 3; // FDT_PROP + prop desc (len + nameoff)
		desc.struct_size += (uint32_t)align_up(p.data.size(), sizeof(uint32_t));
		desc.string_size += (uint32_t)align_up(p.name.size() + 1, sizeof(uint32_t));
	}

	for(auto& c : node->children)
	{
		get_tree_size(c.get(), desc);
	}

	desc.struct_size += sizeof(uint32_t); // FDT_END_NODE
}

struct serializer_ctx
{
	uint8_t* buf		   = nullptr;
	uint32_t struct_off	   = 0;
	uint32_t strings_begin = 0;
	uint32_t strings_off   = 0;
	uint32_t reserve_off   = 0;
};

static void ser_u32(serializer_ctx& ctx, uint32_t value)
{
	write_be32_to_buf(ctx.buf + ctx.struct_off, value);
	ctx.struct_off += 4;
}

static void ser_string(serializer_ctx& ctx, const char* s)
{
	if(!s) s = "";
	size_t len = std::strlen(s);
	std::memcpy(ctx.buf + ctx.struct_off, s, len + 1);
	ctx.struct_off = (uint32_t)align_up(ctx.struct_off + len + 1, sizeof(uint32_t));
}

static void ser_data(serializer_ctx& ctx, const uint8_t* data, uint32_t len)
{
	if(len && data) std::memcpy(ctx.buf + ctx.struct_off, data, len);
	ctx.struct_off = (uint32_t)align_up(ctx.struct_off + len, sizeof(uint32_t));
}

static void ser_name(serializer_ctx& ctx, const char* s)
{
	if(!s) s = "";
	size_t len = std::strlen(s);
	std::memcpy(ctx.buf + ctx.strings_off, s, len + 1);
	ctx.strings_off = (uint32_t)align_up(ctx.strings_off + len + 1, sizeof(uint32_t));
}

static void ser_tree(serializer_ctx& ctx, Node* node)
{
	ser_u32(ctx, FDT_BEGIN_NODE);
	ser_string(ctx, node->name.empty() ? nullptr : node->name.c_str());

	for(auto& p : node->props)
	{
		ser_u32(ctx, FDT_PROP);
		ser_u32(ctx, (uint32_t)p.data.size());
		ser_u32(ctx, ctx.strings_off - ctx.strings_begin); // name offset
		ser_data(ctx, p.data.empty() ? nullptr : p.data.data(), (uint32_t)p.data.size());
		ser_name(ctx, p.name.c_str());
	}

	for(auto& c : node->children)
	{
		ser_tree(ctx, c.get());
	}

	ser_u32(ctx, FDT_END_NODE);
}

size_t size_of_tree(Node* node)
{
	if(!node) return 0;
	size_desc desc{};
	get_tree_size(node, desc);
	desc.struct_size += sizeof(uint32_t); // FDT_END
	uint32_t buf_size = FDT_HDR_SIZE + FDT_RSV_SIZE + desc.struct_size + desc.string_size;
	return buf_size;
}

size_t serialize(Node* node, void* buffer, size_t size, uint32_t boot_cpuid)
{
	if(!node) return 0;
	size_desc desc{};
	get_tree_size(node, desc);
	desc.struct_size += sizeof(uint32_t); // FDT_END

	serializer_ctx ctx{};
	uint32_t buf_size = FDT_HDR_SIZE + FDT_RSV_SIZE + desc.struct_size;
	ctx.reserve_off	  = FDT_HDR_SIZE;
	ctx.struct_off	  = FDT_HDR_SIZE + FDT_RSV_SIZE;
	ctx.strings_begin = buf_size;
	ctx.strings_off	  = ctx.strings_begin;
	buf_size += desc.string_size;

	if(buffer)
	{
		if(buf_size > size)
		{
			return 0; // insufficient space
		}
		std::memset(buffer, 0, buf_size);
		ctx.buf = reinterpret_cast<uint8_t*>(buffer);
		write_be32_to_buf(ctx.buf + 0, FDT_MAGIC);
		write_be32_to_buf(ctx.buf + 4, buf_size);
		write_be32_to_buf(ctx.buf + 8, ctx.struct_off);
		write_be32_to_buf(ctx.buf + 12, ctx.strings_off);
		write_be32_to_buf(ctx.buf + 16, ctx.reserve_off);
		write_be32_to_buf(ctx.buf + 20, FDT_VERSION);
		write_be32_to_buf(ctx.buf + 24, FDT_COMP_VERSION);
		write_be32_to_buf(ctx.buf + 28, boot_cpuid);
		write_be32_to_buf(ctx.buf + 32, desc.string_size);
		write_be32_to_buf(ctx.buf + 36, desc.struct_size);

		ser_tree(ctx, node);
		ser_u32(ctx, FDT_END);
	}

	return buf_size;
}

struct fdt_node
{
	std::unique_ptr<Node> inner;
	Node* node;
};

fdt_node* fdt_node_create(const char* name)
{
	fdt_node* r = new fdt_node();
	r->inner	= std::make_unique<Node>(name ? std::string(name) : std::string());
	r->node		= r->inner.get();
	return r;
}

fdt_node* fdt_node_create_reg(const char* name, uint64_t addr)
{
	fdt_node* r = new fdt_node();
	r->inner	= Node::create_reg(name ? std::string(name) : std::string(), addr);
	r->node		= r->inner.get();
	return r;
}

void fdt_node_add_child(fdt_node* node, fdt_node* child)
{
	if(!node || !child) return;
	if(!child->inner)
	{
		return;
	}
	// move child's unique_ptr into parent's children
	node->node->add_child(std::move(child->inner));
	// delete wrapper of child (it no longer owns inner)
	delete child;
}

fdt_node* to_wrapper(Node* n)
{
	if(!n) return nullptr;
	// create wrapper that does NOT take ownership (dangerous) - but for simplicity, create a temporary wrapper owning unique_ptr=null and point to same Node
	// To keep ownership semantics simple, we only return pointer to existing wrapper if the node is top-level wrapper: (we cannot reliably find wrapper)
	// Simpler approach: this wrapper type isn't used by callers beyond functions in this C API that accept fdt_node* from earlier creation. So we return nullptr in non-wrapper cases.
	// For this codebase we assume user uses nodes created via fdt_node_create* which return wrappers.
	return nullptr;
}

fdt_node* fdt_node_find(fdt_node* node, const char* name)
{
	if(!node || !name) return nullptr;
	Node* found = node->node->find(std::string(name));
	if(!found) return nullptr;
	fdt_node* w = new fdt_node();
	w->inner	= nullptr;
	w->node		= found;
	return w;
}

fdt_node* fdt_node_find_reg(fdt_node* node, const char* name, uint64_t addr)
{
	if(!node || !name) return nullptr;
	Node* found = node->node->find_reg(std::string(name), addr);
	if(!found) return nullptr;
	fdt_node* w = new fdt_node();
	w->inner	= nullptr;
	w->node		= found;
	return w;
}

fdt_node* fdt_node_find_reg_any(fdt_node* node, const char* name)
{
	if(!node || !name) return nullptr;
	Node* found = node->node->find_reg_any(std::string(name));
	if(!found) return nullptr;
	fdt_node* w = new fdt_node();
	w->inner	= nullptr;
	w->node		= found;
	return w;
}

uint32_t fdt_node_get_phandle(fdt_node* node)
{
	if(!node) return 0;
	return node->node->get_phandle();
}

void fdt_node_add_prop(fdt_node* node, const char* name, const void* data, uint32_t len)
{
	if(!node || !name) return;
	node->node->add_prop(std::string(name), data, len);
}
void fdt_node_add_prop_u32(fdt_node* node, const char* name, uint32_t val)
{
	if(!node || !name) return;
	node->node->add_prop_u32(std::string(name), val);
}
void fdt_node_add_prop_u64(fdt_node* node, const char* name, uint64_t val)
{
	if(!node || !name) return;
	node->node->add_prop_u64(std::string(name), val);
}
void fdt_node_add_prop_cells(fdt_node* node, const char* name, std::vector<uint32_t> cells, uint32_t count)
{
	if(!node || !name) return;
	node->node->add_prop_cells(std::string(name), cells, count);
}
void fdt_node_add_prop_str(fdt_node* node, const char* name, const char* val)
{
	if(!node || !name) return;
	node->node->add_prop_str(std::string(name), std::string(val ? val : ""));
}
void fdt_node_add_prop_reg(fdt_node* node, const char* name, uint64_t begin, uint64_t size)
{
	if(!node || !name) return;
	node->node->add_prop_reg(std::string(name), begin, size);
}

void* fdt_node_get_prop_data(fdt_node* node, const char* name)
{
	if(!node || !name) return nullptr;
	return const_cast<void*>(node->node->get_prop_data(std::string(name)));
}
size_t fdt_node_get_prop_size(fdt_node* node, const char* name)
{
	if(!node || !name) return 0;
	return node->node->get_prop_size(std::string(name));
}
bool fdt_node_del_prop(fdt_node* node, const char* name)
{
	if(!node || !name) return false;
	return node->node->del_prop(std::string(name));
}

void fdt_node_free(fdt_node* node)
{
	if(!node) return;
	// node->inner unique_ptr will free internal tree automatically
	delete node;
}

size_t fdt_size(fdt_node* node)
{
	if(!node) return 0;
	return size_of_tree(node->node);
}

size_t fdt_serialize(fdt_node* node, void* buffer, size_t size, uint32_t boot_cpuid)
{
	if(!node) return 0;
	return serialize(node->node, buffer, size, boot_cpuid);
}
