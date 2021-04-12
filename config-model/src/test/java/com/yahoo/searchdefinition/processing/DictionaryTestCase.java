// Copyright Verizon Media. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

package com.yahoo.searchdefinition.processing;

import com.yahoo.config.model.test.TestUtil;
import com.yahoo.searchdefinition.Search;
import com.yahoo.searchdefinition.SearchBuilder;
import com.yahoo.searchdefinition.derived.AttributeFields;
import com.yahoo.searchdefinition.document.Dictionary;
import com.yahoo.searchdefinition.parser.ParseException;
import com.yahoo.vespa.config.search.AttributesConfig;
import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.fail;

/**
 * Test configuration of dictionary control.
 *
 * @author baldersheim
 */
public class DictionaryTestCase {
    private static AttributesConfig getConfig(Search search) {
        AttributeFields attributes = new AttributeFields(search);
        AttributesConfig.Builder builder = new AttributesConfig.Builder();
        attributes.getConfig(builder);
        return builder.build();
    }
    private Search createSearch(String def) throws ParseException {
        SearchBuilder sb = SearchBuilder.createFromString(def);
        return sb.getSearch();
    }
    @Test
    public void testDefaultDictionarySettings() throws ParseException {
        String def = TestUtil.joinLines(
                        "search test {",
                        "    document test {",
                        "        field s1 type string {",
                        "            indexing: attribute | summary",
                        "        }",
                        "        field n1 type int {",
                        "            indexing: summary | attribute",
                        "        }",
                        "    }",
                        "}");
        Search search = createSearch(def);
        assertNull(search.getAttribute("s1").getDictionary());
        assertNull(search.getAttribute("n1").getDictionary());
        assertEquals(AttributesConfig.Attribute.Dictionary.Type.BTREE,
                getConfig(search).attribute().get(0).dictionary().type());
        assertEquals(AttributesConfig.Attribute.Dictionary.Type.BTREE,
                getConfig(search).attribute().get(1).dictionary().type());
        assertEquals(AttributesConfig.Attribute.Dictionary.Match.CASE_INSENSITIVE,
                getConfig(search).attribute().get(0).dictionary().match());
        assertEquals(AttributesConfig.Attribute.Dictionary.Match.CASE_INSENSITIVE,
                getConfig(search).attribute().get(1).dictionary().match());
    }

    Search verifyDictionaryControl(Dictionary.Type expected,
                                 AttributesConfig.Attribute.Dictionary.Type.Enum expectedConfig,
                                 String type,
                                 String ... cfg) throws ParseException
    {
        String def = TestUtil.joinLines(
                "search test {",
                "    document test {",
                "        field n1 type " + type + " {",
                "            indexing: summary | attribute",
                "            attribute:fast-search",
                TestUtil.joinLines(cfg),
                "        }",
                "    }",
                "}");
        Search search = createSearch(def);
        assertEquals(expected, search.getAttribute("n1").getDictionary().getType());
        assertEquals(expectedConfig, getConfig(search).attribute().get(0).dictionary().type());
        return search;
    }

    void verifyStringDictionaryControl(Dictionary.Type expectedType,Dictionary.Match expectedCase,
                                       AttributesConfig.Attribute.Dictionary.Type.Enum expectedTypeCfg,
                                       AttributesConfig.Attribute.Dictionary.Match.Enum expectedCaseCfg,
                                       String type,
                                       String ... cfg) throws ParseException
    {

        Search search = verifyDictionaryControl(expectedType, expectedTypeCfg, type, cfg);
        assertEquals(expectedCase, search.getAttribute("n1").getDictionary().getMatch());
        assertEquals(expectedCaseCfg, getConfig(search).attribute().get(0).dictionary().match());
    }

    @Test
    public void testCasedBtreeSettings() throws ParseException {
        verifyDictionaryControl(Dictionary.Type.BTREE,
                AttributesConfig.Attribute.Dictionary.Type.BTREE,
                "int",
                "dictionary:cased");
    }

    @Test
    public void testNumericBtreeSettings() throws ParseException {
        verifyDictionaryControl(Dictionary.Type.BTREE,
                AttributesConfig.Attribute.Dictionary.Type.BTREE,
                "int",
                "dictionary:btree");
    }
    @Test
    public void testNumericHashSettings() throws ParseException {
        verifyDictionaryControl(Dictionary.Type.HASH,
                AttributesConfig.Attribute.Dictionary.Type.HASH,
                "int",
                "dictionary:hash");
    }
    @Test
    public void testNumericBtreeAndHashSettings() throws ParseException {
        verifyDictionaryControl(Dictionary.Type.BTREE_AND_HASH,
                AttributesConfig.Attribute.Dictionary.Type.BTREE_AND_HASH,
                "int",
                "dictionary:btree", "dictionary:hash");
    }
    @Test
    public void testNumericArrayBtreeAndHashSettings() throws ParseException {
        verifyDictionaryControl(Dictionary.Type.BTREE_AND_HASH,
                AttributesConfig.Attribute.Dictionary.Type.BTREE_AND_HASH,
                "array<int>",
                "dictionary:btree", "dictionary:hash");
    }
    @Test
    public void testNumericWSetBtreeAndHashSettings() throws ParseException {
        verifyDictionaryControl(Dictionary.Type.BTREE_AND_HASH,
                AttributesConfig.Attribute.Dictionary.Type.BTREE_AND_HASH,
                "weightedset<int>",
                "dictionary:btree", "dictionary:hash");
    }
    @Test
    public void testStringBtreeSettings() throws ParseException {
        verifyStringDictionaryControl(Dictionary.Type.BTREE, Dictionary.Match.UNCASED,
                AttributesConfig.Attribute.Dictionary.Type.BTREE,
                AttributesConfig.Attribute.Dictionary.Match.UNCASED,
                "string",
                "dictionary:btree");
    }
    @Test
    public void testStringBtreeUnCasedSettings() throws ParseException {
        verifyStringDictionaryControl(Dictionary.Type.BTREE, Dictionary.Match.UNCASED,
                AttributesConfig.Attribute.Dictionary.Type.BTREE,
                AttributesConfig.Attribute.Dictionary.Match.UNCASED,
                "string",
                "dictionary { btree\nuncased\n}");
    }
    @Test
    public void testStringBtreeCasedSettings() throws ParseException {
        verifyStringDictionaryControl(Dictionary.Type.BTREE, Dictionary.Match.CASED,
                AttributesConfig.Attribute.Dictionary.Type.BTREE,
                AttributesConfig.Attribute.Dictionary.Match.CASED,
                "string",
                "dictionary { btree\ncased\n}");
    }
    @Test
    public void testStringHashSettings() throws ParseException {
        verifyStringDictionaryControl(Dictionary.Type.HASH, Dictionary.Match.UNCASED,
                AttributesConfig.Attribute.Dictionary.Type.HASH,
                AttributesConfig.Attribute.Dictionary.Match.UNCASED,
                "string",
                "dictionary:hash");
    }
    @Test
    public void testStringHashUnCasedSettings() throws ParseException {
        verifyStringDictionaryControl(Dictionary.Type.HASH, Dictionary.Match.UNCASED,
                AttributesConfig.Attribute.Dictionary.Type.HASH,
                AttributesConfig.Attribute.Dictionary.Match.UNCASED,
                "string",
                "dictionary { hash\nuncased\n}");
    }
    @Test
    public void testStringHashCasedSettings() throws ParseException {
        verifyStringDictionaryControl(Dictionary.Type.HASH, Dictionary.Match.CASED,
                AttributesConfig.Attribute.Dictionary.Type.HASH,
                AttributesConfig.Attribute.Dictionary.Match.CASED,
                "string",
                "dictionary { hash\ncased\n}");
    }
    @Test
    public void testStringBtreeHashSettings() throws ParseException {
        verifyStringDictionaryControl(Dictionary.Type.BTREE_AND_HASH, Dictionary.Match.UNCASED,
                AttributesConfig.Attribute.Dictionary.Type.BTREE_AND_HASH,
                AttributesConfig.Attribute.Dictionary.Match.UNCASED,
                "string",
                "dictionary{hash\nbtree\n}");
    }
    @Test
    public void testStringBtreeHashUnCasedSettings() throws ParseException {
        verifyStringDictionaryControl(Dictionary.Type.BTREE_AND_HASH, Dictionary.Match.UNCASED,
                AttributesConfig.Attribute.Dictionary.Type.BTREE_AND_HASH,
                AttributesConfig.Attribute.Dictionary.Match.UNCASED,
                "string",
                "dictionary { hash\nbtree\nuncased\n}");
    }
    @Test
    public void testStringBtreeHashCasedSettings() throws ParseException {
        verifyStringDictionaryControl(Dictionary.Type.BTREE_AND_HASH, Dictionary.Match.CASED,
                AttributesConfig.Attribute.Dictionary.Type.BTREE_AND_HASH,
                AttributesConfig.Attribute.Dictionary.Match.CASED,
                "string",
                "dictionary { btree\nhash\ncased\n}");
    }
    @Test
    public void testNonNumericFieldsFailsDictionaryControl() throws ParseException {
        String def = TestUtil.joinLines(
                "search test {",
                "    document test {",
                "        field n1 type bool {",
                "            indexing: summary | attribute",
                "            dictionary:btree",
                "        }",
                "    }",
                "}");
        try {
            SearchBuilder sb = SearchBuilder.createFromString(def);
            fail("Controlling dictionary for non-numeric fields are not yet supported.");
        } catch (IllegalArgumentException e) {
            assertEquals("For search 'test', field 'n1': You can only specify 'dictionary:' for numeric or string fields", e.getMessage());
        }
    }
    @Test
    public void testNonFastSearchNumericFieldsFailsDictionaryControl() throws ParseException {
        String def = TestUtil.joinLines(
                "search test {",
                "    document test {",
                "        field n1 type int {",
                "            indexing: summary | attribute",
                "            dictionary:btree",
                "        }",
                "    }",
                "}");
        try {
            SearchBuilder sb = SearchBuilder.createFromString(def);
            fail("Controlling dictionary for non-fast-search fields are not allowed.");
        } catch (IllegalArgumentException e) {
            assertEquals("For search 'test', field 'n1': You must specify 'attribute:fast-search' to allow dictionary control", e.getMessage());
        }
    }
}
