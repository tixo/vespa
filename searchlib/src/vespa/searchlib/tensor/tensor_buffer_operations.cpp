// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "tensor_buffer_operations.h"
#include <vespa/eval/eval/fast_value.hpp>
#include <vespa/eval/eval/value.h>
#include <vespa/eval/eval/value_codec.h>
#include <vespa/eval/eval/value_type.h>
#include <vespa/eval/streamed/streamed_value_view.h>
#include <vespa/vespalib/util/arrayref.h>
#include <vespa/vespalib/util/shared_string_repo.h>
#include <algorithm>

using vespalib::ArrayRef;
using vespalib::ConstArrayRef;
using vespalib::MemoryUsage;
using vespalib::SharedStringRepo;
using vespalib::StringIdVector;
using vespalib::eval::FastAddrMap;
using vespalib::eval::FastValueIndex;
using vespalib::eval::StreamedValueView;
using vespalib::eval::TypedCells;
using vespalib::eval::Value;
using vespalib::eval::ValueType;
using vespalib::eval::self_memory_usage;
using vespalib::string_id;

namespace search::tensor {

namespace {

uint32_t
adjust_min_alignment(size_t min_alignment)
{
    // Also apply alignment for num_subspaces and labels
    return std::max(std::max(sizeof(uint32_t), sizeof(string_id)), min_alignment);
}

struct FastValueView final : Value {
    const ValueType& _type;
    StringIdVector   _labels;
    FastValueIndex   _index;
    TypedCells       _cells;
    FastValueView(const ValueType& type, ConstArrayRef<string_id> labels, TypedCells cells, size_t num_mapped_dimensions, size_t num_subspaces);
    const ValueType& type() const override { return _type; }
    const Value::Index& index() const override { return _index; }
    TypedCells cells() const override { return _cells; }
    MemoryUsage get_memory_usage() const override {
        MemoryUsage usage = self_memory_usage<FastValueView>();
        usage.merge(_index.map.estimate_extra_memory_usage());
        return usage;
    }
};

FastValueView::FastValueView(const ValueType& type, ConstArrayRef<string_id> labels, TypedCells cells, size_t num_mapped_dimensions, size_t num_subspaces)
    : Value(),
      _type(type),
      _labels(labels.begin(), labels.end()),
      _index(num_mapped_dimensions, _labels, num_subspaces),
      _cells(cells)
{
    for (size_t i = 0; i < num_subspaces; ++i) {
        ConstArrayRef<string_id> addr(_labels.data() + (i * num_mapped_dimensions), num_mapped_dimensions);
        _index.map.add_mapping(FastAddrMap::hash_labels(addr));
    }
    assert(_index.map.size() == num_subspaces);
}

}

TensorBufferOperations::TensorBufferOperations(const vespalib::eval::ValueType& tensor_type)
    : _num_mapped_dimensions(tensor_type.count_mapped_dimensions()),
      _cell_mem_size(vespalib::eval::CellTypeUtils::mem_size(tensor_type.cell_type(), 1u)),
      _min_alignment(adjust_min_alignment(vespalib::eval::CellTypeUtils::alignment(tensor_type.cell_type()))),
      _dense_subspace_size(tensor_type.dense_subspace_size()),
      _cell_type(tensor_type.cell_type()),
      _addr(_num_mapped_dimensions),
      _addr_refs()
{
    _addr_refs.reserve(_addr.size());
    for (auto& label : _addr) {
        _addr_refs.push_back(&label);
    }
}

TensorBufferOperations::~TensorBufferOperations() = default;

uint32_t
TensorBufferOperations::get_num_subspaces(ConstArrayRef<char> buf) const noexcept
{
    assert(buf.size() >= get_num_subspaces_size());
    return *reinterpret_cast<const uint32_t*>(buf.data());
}

void
TensorBufferOperations::store_tensor(ArrayRef<char> buf, const vespalib::eval::Value& tensor)
{
    uint32_t num_subspaces = tensor.index().size();
    auto labels_end_offset = get_labels_offset() + get_labels_mem_size(num_subspaces);
    auto cells_size = num_subspaces * _dense_subspace_size;
    auto cells_mem_size = cells_size * _cell_mem_size; // Size measured in bytes
    auto alignment = select_alignment(cells_mem_size);
    auto cells_start_offset = calc_aligned(labels_end_offset, alignment);
    auto cells_end_offset = cells_start_offset + cells_mem_size;
    auto store_end = calc_aligned(cells_end_offset, alignment);
    assert(store_end == get_array_size(num_subspaces));
    assert(buf.size() >= store_end);
    *reinterpret_cast<uint32_t*>(buf.data()) = num_subspaces;
    auto labels = reinterpret_cast<string_id*>(buf.data() + get_labels_offset());
    size_t subspace = 0;
    size_t num_subspaces_visited = 0;
    auto view = tensor.index().create_view({});
    view->lookup({});
    while (view->next_result(_addr_refs, subspace)) {
        assert(subspace < num_subspaces);
        auto subspace_labels = labels + subspace * _num_mapped_dimensions;
        for (auto& label : _addr) {
            SharedStringRepo::unsafe_copy(label); // tensor has an existing ref
            *subspace_labels = label;
            ++subspace_labels;
        }
        ++num_subspaces_visited;
    }
    assert(num_subspaces_visited == num_subspaces);
    if (labels_end_offset != cells_start_offset) {
        memset(buf.data() + labels_end_offset, 0, cells_start_offset - labels_end_offset);
    }
    auto cells = tensor.cells();
    assert(cells_size == cells.size);
    memcpy(buf.data() + cells_start_offset, cells.data, cells_mem_size);
    if (cells_end_offset != store_end) {
        memset(buf.data() + cells_end_offset, 0, store_end - cells_end_offset);
    }
}

std::unique_ptr<vespalib::eval::Value>
TensorBufferOperations::make_fast_view(ConstArrayRef<char> buf, const vespalib::eval::ValueType& tensor_type) const
{
    auto num_subspaces = get_num_subspaces(buf);
    assert(buf.size() >= get_array_size(num_subspaces));
    ConstArrayRef<string_id> labels(reinterpret_cast<const string_id*>(buf.data() + get_labels_offset()), num_subspaces * _num_mapped_dimensions);
    auto cells_size = num_subspaces * _dense_subspace_size;
    auto cells_mem_size = cells_size * _cell_mem_size; // Size measured in bytes
    auto alignment = select_alignment(cells_mem_size);
    auto cells_start_offset = get_cells_offset(num_subspaces, alignment);
    TypedCells cells(buf.data() + cells_start_offset, _cell_type, cells_size);
    assert(cells_start_offset + cells_mem_size <= buf.size());
    return std::make_unique<FastValueView>(tensor_type, labels, cells, _num_mapped_dimensions, num_subspaces);
}

void
TensorBufferOperations::copied_labels(ConstArrayRef<char> buf) const
{
    auto num_subspaces = get_num_subspaces(buf);
    ConstArrayRef<string_id> labels(reinterpret_cast<const string_id*>(buf.data() + get_labels_offset()), num_subspaces * _num_mapped_dimensions);
    for (auto& label : labels) {
        SharedStringRepo::unsafe_copy(label); // Source buffer has an existing ref
    }
}

void
TensorBufferOperations::reclaim_labels(ArrayRef<char> buf) const
{
    auto num_subspaces = get_num_subspaces(buf);
    ArrayRef<string_id> labels(reinterpret_cast<string_id*>(buf.data() + get_labels_offset()), num_subspaces * _num_mapped_dimensions);
    for (auto& label : labels) {
        SharedStringRepo::unsafe_reclaim(label);
        label = string_id(); // Clear label to avoid double reclaim
    }
}

void
TensorBufferOperations::encode_stored_tensor(ConstArrayRef<char> buf, const vespalib::eval::ValueType& tensor_type, vespalib::nbostream& target) const
{
    auto num_subspaces = get_num_subspaces(buf);
    assert(buf.size() >= get_array_size(num_subspaces));
    ConstArrayRef<string_id> labels(reinterpret_cast<const string_id*>(buf.data() + get_labels_offset()), num_subspaces * _num_mapped_dimensions);
    auto cells_size = num_subspaces * _dense_subspace_size;
    auto cells_mem_size = cells_size * _cell_mem_size; // Size measured in bytes
    auto alignment = select_alignment(cells_mem_size);
    auto cells_start_offset = get_cells_offset(num_subspaces, alignment);
    TypedCells cells(buf.data() + cells_start_offset, _cell_type, cells_size);
    assert(cells_start_offset + cells_mem_size <= buf.size());
    StringIdVector labels_copy(labels.begin(), labels.end());
    StreamedValueView streamed_value_view(tensor_type, _num_mapped_dimensions, cells, num_subspaces, labels_copy);
    vespalib::eval::encode_value(streamed_value_view, target);
}

}