// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include "utf8strchrfieldsearcher.h"
#include "tokenizereader.h"

using search::streaming::QueryTerm;
using search::streaming::QueryTermList;
using search::byte;

namespace vsm {

std::unique_ptr<FieldSearcher>
UTF8StrChrFieldSearcher::duplicate() const
{
    return std::make_unique<UTF8StrChrFieldSearcher>(*this);
}

size_t
UTF8StrChrFieldSearcher::matchTerms(const FieldRef & f, size_t mintsz)
{
    (void) mintsz;
    termcount_t words(0);
    if (f.size() >= _buf->size()) {
        _buf->reserve(f.size() + 1);
    }
    cmptype_t * fn = &(*_buf.get())[0];

    TokenizeReader reader(reinterpret_cast<const byte *> (f.data()), f.size(), fn);
    while ( reader.hasNext() ) {
        tokenize(reader);
        size_t fl = reader.complete();
        for (auto qt : _qtl) {
            const cmptype_t * term;
            termsize_t tsz = qt->term(term);
            if ((tsz <= fl) && (prefix() || qt->isPrefix() || (tsz == fl))) {
                const cmptype_t *tt=term, *et=term+tsz;
                for (const cmptype_t *fnt=fn; (tt < et) && (*tt == *fnt); tt++, fnt++);
                if (tt == et) {
                    addHit(*qt, words);
                }
            }
        }
        words++;
    }
    return words;
}

size_t
UTF8StrChrFieldSearcher::matchTerm(const FieldRef & f, QueryTerm & qt)
{
    return matchTermRegular(f, qt);
}

}
