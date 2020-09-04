// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "generic_tensor_attribute.h"
#include "generic_tensor_attribute_saver.h"
#include "tensor_attribute.hpp"
#include "blob_sequence_reader.h"
#include <vespa/eval/tensor/tensor.h>
#include <vespa/fastlib/io/bufferedfile.h>
#include <vespa/searchlib/attribute/readerbase.h>
#include <vespa/searchlib/util/fileutil.h>
#include <vespa/vespalib/util/rcuvector.hpp>

using vespalib::eval::ValueType;
using vespalib::tensor::Tensor;

namespace search::tensor {

namespace {

constexpr uint32_t TENSOR_ATTRIBUTE_VERSION = 0;

}

GenericTensorAttribute::GenericTensorAttribute(stringref name, const Config &cfg)
    : TensorAttribute(name, cfg, _serializedTensorStore)
{
}


GenericTensorAttribute::~GenericTensorAttribute()
{
    getGenerationHolder().clearHoldLists();
    _tensorStore.clearHoldLists();
}

void
GenericTensorAttribute::setTensor(DocId docId, const Tensor &tensor)
{
    checkTensorType(tensor);
    EntryRef ref = _serializedTensorStore.setTensor(tensor);
    setTensorRef(docId, ref);
}


std::unique_ptr<Tensor>
GenericTensorAttribute::getTensor(DocId docId) const
{
    EntryRef ref;
    if (docId < getCommittedDocIdLimit()) {
        ref = _refVector[docId];
    }
    if (!ref.valid()) {
        return std::unique_ptr<Tensor>();
    }
    return _serializedTensorStore.getTensor(ref);
}

void
GenericTensorAttribute::getTensor(DocId, vespalib::tensor::MutableDenseTensorView &) const
{
    notImplemented();
}

bool
GenericTensorAttribute::onLoad()
{
    BlobSequenceReader tensorReader(*this);
    if (!tensorReader.hasData()) {
        return false;
    }
    setCreateSerialNum(tensorReader.getCreateSerialNum());
    assert(tensorReader.getVersion() == TENSOR_ATTRIBUTE_VERSION);
    uint32_t numDocs(tensorReader.getDocIdLimit());
    _refVector.reset();
    _refVector.unsafe_reserve(numDocs);
    for (uint32_t lid = 0; lid < numDocs; ++lid) {
        uint32_t tensorSize = tensorReader.getNextSize();
        auto raw = _serializedTensorStore.allocRawBuffer(tensorSize);
        if (tensorSize != 0) {
            tensorReader.readBlob(raw.data, tensorSize);
        }
        _refVector.push_back(raw.ref);
    }
    setNumDocs(numDocs);
    setCommittedDocIdLimit(numDocs);
    return true;
}


std::unique_ptr<AttributeSaver>
GenericTensorAttribute::onInitSave(vespalib::stringref fileName)
{
    vespalib::GenerationHandler::Guard guard(getGenerationHandler().
                                             takeGuard());
    return std::make_unique<GenericTensorAttributeSaver>
        (std::move(guard),
         this->createAttributeHeader(fileName),
         getRefCopy(),
         _serializedTensorStore);
}

void
GenericTensorAttribute::compactWorst()
{
    doCompactWorst<SerializedTensorStore::RefType>();
}

}
