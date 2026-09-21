/**
 * @file
 * @brief Smoke tests of the XML library the .qft reader is built on.
 *
 * Every shipped fixture must parse as well-formed XML with the `QFT` root and
 * its sections must be reachable by name in any order; a well-formed file with
 * a foreign root is accepted here, since refusing it is the project reader's
 * job; malformed input must fail with an offset and a description rather than
 * crash.
 */

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include <pugixml.hpp>

namespace {

pugi::xml_parse_result load(pugi::xml_document& doc, const char* name)
{
    const std::string path = std::string(QFTBX_TEST_DATA_DIR "/") + name;
    return doc.load_file(path.c_str());
}

TEST(PugixmlSmoke, EveryFixtureParsesWithTheQftRoot)
{
    const char* fixtures[] = {"cervera.qft", "planta2.qft", "multivaluados.qft",
                              "planta1.qft", "corrupt_omega.qft", "short_specs.qft"};

    for (const char* name : fixtures) {
        pugi::xml_document doc;
        const pugi::xml_parse_result result = load(doc, name);
        ASSERT_TRUE(result) << name << ": " << result.description();
        EXPECT_STREQ(doc.document_element().name(), "QFT") << name;
    }
}

TEST(PugixmlSmoke, SectionsAreReachableByName)
{
    pugi::xml_document doc;
    ASSERT_TRUE(load(doc, "multivaluados.qft"));

    const pugi::xml_node root = doc.document_element();
    const pugi::xml_node inputs = root.child("inputs");
    const pugi::xml_node results = root.child("results");
    ASSERT_TRUE(inputs);
    ASSERT_TRUE(results);

    EXPECT_TRUE(inputs.child("plant"));
    EXPECT_TRUE(inputs.child("specifications"));
    EXPECT_TRUE(inputs.child("omega"));
    EXPECT_TRUE(inputs.child("controller"));
    EXPECT_TRUE(results.child("templates"));
    EXPECT_TRUE(results.child("boundaries"));

    const pugi::xml_node spec = inputs.child("specifications").child("specification");
    ASSERT_TRUE(spec);
    EXPECT_GT(spec.child("max-frequency").text().as_double(), 0.0);
}

TEST(PugixmlSmoke, WrongRootStaysAReaderLevelError)
{
    pugi::xml_document doc;
    ASSERT_TRUE(load(doc, "invalid.qft"));
    EXPECT_STRNE(doc.document_element().name(), "QFT");
}

TEST(PugixmlSmoke, MalformedInputFailsWithALocatedError)
{
    const char* broken = "<?xml version=\"1.0\"?>\n<QFT>\n  <Planta>\n</QFT>\n";

    pugi::xml_document doc;
    const pugi::xml_parse_result result = doc.load_string(broken);
    EXPECT_FALSE(result);
    EXPECT_GT(result.offset, 0);
    EXPECT_NE(result.description(), nullptr);
}

}
