// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.prelude.hitfield;

import java.util.List;
import java.util.ListIterator;
import java.util.NoSuchElementException;

/**
 * A specialized list iterator to manipulate tokens in HitField objects.
 *
 * @author Steinar Knutsen
 */
public class TokenFieldIterator implements ListIterator<FieldPart> {

    private int index = 0;
    private int prevReturned = 0;
    private final List<FieldPart> fieldList;
    private final HitField hitField;

    public TokenFieldIterator(List<FieldPart> fieldList, HitField hitField) {
        this.fieldList = fieldList;
        this.hitField = hitField;
    }

    public void add(FieldPart o) {
        fieldList.add(index, o);
        index++;
        hitField.markDirty();
    }

    public boolean hasNext() {
        int i = index;
        while (i < fieldList.size()) {
            if (fieldList.get(i).isToken())
                return true;
            i++;
        }
        return false;
    }

    public boolean hasPrevious() {
        int i = index;
        while (i > 0) {
            i--;
            if (fieldList.get(i).isToken())
                return true;
        }
        return false;
    }

    public FieldPart next() {
        int i = index;
        while (i < fieldList.size()) {
            if (fieldList.get(i).isToken()) {
                index = i + 1;
                prevReturned = i;
                return fieldList.get(i);
            }
            i++;
        }
        throw new NoSuchElementException("No more tokens available.");
    }

    public int nextIndex() {
        int i = index;
        while (i < fieldList.size()) {
            if (fieldList.get(i).isToken())
                return i;
            i++;
        }
        return fieldList.size();
    }

    public FieldPart previous() {
        int i = index;
        while (i > 0) {
            i--;
            if (fieldList.get(i).isToken()) {
                index = i;
                prevReturned = i;
                return fieldList.get(i);
            }
        }
        throw new NoSuchElementException("Trying to go before first token available.");
    }

    public int previousIndex() {
        int i = index;
        while (i > 0) {
            i--;
            if (fieldList.get(i).isToken())
                return i;
        }
        return -1;
    }

    public void remove() {
        fieldList.remove(prevReturned);
        if (prevReturned < index)
            index--;
        hitField.markDirty();
    }

    public void set(FieldPart o) {
        fieldList.set(prevReturned, o);
        hitField.markDirty();
    }

}
