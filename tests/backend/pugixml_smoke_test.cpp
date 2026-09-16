// Smoke tests for the pugixml integration (phase 6): every shipped .qft
// fixture must parse as well-formed XML with the expected root, and the
// corrupt fixtures must fail with a located error instead of crashing.

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
    // DOM access by name, in any order: the property the streaming parser
    // never had (it forces one fixed section order).
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

    // Numeric conversion straight from the tree.
    const pugi::xml_node spec = inputs.child("specifications").child("specification");
    ASSERT_TRUE(spec);
    EXPECT_GT(spec.child("max-frequency").text().as_double(), 0.0);
}

TEST(PugixmlSmoke, WrongRootStaysAReaderLevelError)
{
    // invalid.qft is well-formed XML with the wrong root: pugixml accepts
    // it, and rejecting it is the project reader's job.
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
    EXPECT_GT(result.offset, 0);              // byte offset for ParseError lines
    EXPECT_NE(result.description(), nullptr); // human-readable reason
}

} // namespace
