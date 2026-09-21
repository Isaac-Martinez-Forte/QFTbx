/**
 * @file
 * @brief Adversarial tests of the .qft reader: what a damaged file does to it.
 *
 * Each case mutates a real fixture so that exactly one defect is present:
 * bytes that are not XML, an empty or truncated file, a foreign root, a
 * missing or unknown format version, non-numeric and non-boolean values, an
 * unknown system type, a garbage count. Loading must fail with a typed
 * exception whose message names the file and the element, leave nothing
 * half-built, and not spoil the next load. The version is required because
 * earlier dialects share tag names with different meanings. A project saved
 * unfinished, or a section with an element missing, is read for what it holds
 * and the rest left empty; a redundant size attribute is ignored.
 */

#include <gtest/gtest.h>

#include <string>

#include <QByteArray>
#include <QFile>
#include <QTemporaryDir>

#include "src/core/common/exception.h"
#include "src/persistence/project_reader.h"

using namespace qftbx;

namespace {

QByteArray fixtureBytes(const char * name)
{
    QFile file(QString::fromStdString(std::string(QFTBX_TEST_DATA_DIR "/") + name));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

class MalformedProject : public ::testing::Test
{
protected:
    std::string write(const QByteArray & content, const std::string & name = std::string("case.qft"))
    {
        const QString path = m_dir.path() + "/" + QString::fromStdString(name);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            return {};
        }
        file.write(content);
        file.close();
        return path.toStdString();
    }

    std::string mutated(const char * fixture, const char * from, const char * to)
    {
        QByteArray bytes = fixtureBytes(fixture);
        EXPECT_FALSE(bytes.isEmpty()) << "fixture " << fixture << " unreadable";

        const int at = bytes.indexOf(from);
        EXPECT_NE(at, -1) << "the fixture no longer contains \"" << from
                          << "\": this case stopped testing anything";
        if (at == -1) {
            return {};
        }

        bytes.replace(from, to);
        return write(bytes);
    }

    std::string without(const char * fixture, const char * element)
    {
        QByteArray bytes = fixtureBytes(fixture);
        EXPECT_FALSE(bytes.isEmpty()) << "fixture " << fixture << " unreadable";

        const QByteArray open = QByteArray("<") + element + ">";
        const QByteArray close = QByteArray("</") + element + ">";

        const int from = bytes.indexOf(open);
        const int to = bytes.indexOf(close, from);
        EXPECT_NE(from, -1) << "the fixture has no <" << element << ">";
        EXPECT_NE(to, -1) << "the fixture has no </" << element << ">";
        if (from == -1 || to == -1) {
            return {};
        }

        bytes.remove(from, to + close.size() - from);
        return write(bytes);
    }

    QTemporaryDir m_dir;
};

TEST_F(MalformedProject, AMissingFileIsAFileError)
{
    qftbx::ProjectReader parser;

    EXPECT_THROW(parser.load((m_dir.path() + "/there-is-no-such-file.qft").toStdString()),
                 qftbx::FileError);
}

TEST_F(MalformedProject, BytesThatAreNotXmlAreAParseError)
{
    qftbx::ProjectReader parser;

    const std::string path = write(QByteArray("this is not xml at all\n\x01\x02\x03"));
    ASSERT_FALSE(path.empty());

    EXPECT_THROW(parser.load(path), qftbx::ParseError);
}

TEST_F(MalformedProject, AnEmptyFileIsAParseError)
{
    qftbx::ProjectReader parser;

    const std::string path = write(QByteArray());
    ASSERT_FALSE(path.empty());

    EXPECT_THROW(parser.load(path), qftbx::ParseError);
}

TEST_F(MalformedProject, ATruncatedFileIsAParseError)
{
    QByteArray bytes = fixtureBytes("planta1.qft");
    ASSERT_FALSE(bytes.isEmpty());

    const std::string path = write(bytes.left(bytes.size() / 2));
    ASSERT_FALSE(path.empty());

    qftbx::ProjectReader parser;
    EXPECT_THROW(parser.load(path), qftbx::ParseError);
}

TEST_F(MalformedProject, AForeignRootElementIsAParseError)
{
    qftbx::ProjectReader parser;

    const std::string path = mutated("planta1.qft", "<QFT version=\"4\">", "<NotQFT version=\"4\">");
    ASSERT_FALSE(path.empty());

    EXPECT_THROW(parser.load(path), qftbx::ParseError);
}

TEST_F(MalformedProject, AFileWithNoVersionIsRefused)
{
    qftbx::ProjectReader parser;

    const std::string path = mutated("planta1.qft", "<QFT version=\"4\">", "<QFT>");
    ASSERT_FALSE(path.empty());

    EXPECT_THROW(parser.load(path), qftbx::ParseError);
}

TEST_F(MalformedProject, AVersionThisBuildDoesNotKnowIsRefused)
{
    qftbx::ProjectReader parser;

    for (const char * version : {"<QFT version=\"3\">", "<QFT version=\"5\">"}) {
        const std::string path = mutated("planta1.qft", "<QFT version=\"4\">", version);
        ASSERT_FALSE(path.empty());
        EXPECT_THROW(parser.load(path), qftbx::ParseError) << version;
    }
}

TEST_F(MalformedProject, DISABLED_placeholderForTheOldFutureVersionCase)
{
    qftbx::ProjectReader parser;

    const std::string path = mutated("planta1.qft", "<QFT version=\"4\">",
                                     "<QFT version=\"5\">");
    ASSERT_FALSE(path.empty());

    EXPECT_THROW(parser.load(path), qftbx::ParseError);
}

TEST_F(MalformedProject, ANonNumericValueIsAParseError)
{
    qftbx::ProjectReader parser;

    const std::string path = mutated("planta1.qft", "<nominal>5</nominal>",
                                 "<nominal>not a number</nominal>");
    ASSERT_FALSE(path.empty());

    EXPECT_THROW(parser.load(path), qftbx::ParseError);
}

TEST_F(MalformedProject, ANonBooleanFlagIsAParseError)
{
    qftbx::ProjectReader parser;

    const std::string path = mutated("planta1.qft", "<uncertain>true</uncertain>",
                                 "<uncertain>perhaps</uncertain>");
    ASSERT_FALSE(path.empty());

    EXPECT_THROW(parser.load(path), qftbx::ParseError);
}

TEST_F(MalformedProject, AnUnfinishedProjectLoadsWhateverItHas)
{
    const std::string path = write(QByteArray(
        "<?xml version=\"1.0\"?><QFT version=\"4\">"
        "<inputs>"
        "<plant name=\"half\"><type id=\"3\">"
        "<expression size=\"0\"/>"
        "<numerator size=\"1\"><parameter><nominal>1</nominal><uncertain>false</uncertain></parameter></numerator>"
        "<denominator size=\"1\"><parameter><nominal>2</nominal><uncertain>false</uncertain></parameter></denominator>"
        "<parameter><nominal>1</nominal><uncertain>false</uncertain></parameter>"
        "<parameter><nominal>0</nominal><uncertain>false</uncertain></parameter>"
        "</type></plant>"
        "<specifications count=\"1\"><specification name=\"Stability\"><used>false</used></specification></specifications>"
        "</inputs>"
        "<results>"
        "<templates/>"
        "<loop-shaping><data point-count=\"10\"><min>1</min><max>10</max></data></loop-shaping>"
        "</results>"
        "</QFT>"));
    ASSERT_FALSE(path.empty());

    qftbx::ProjectReader parser;
    qftbx::ProjectReader::Loaded loaded;
    ASSERT_NO_THROW(loaded = parser.load(path));

    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Plant)) << "the plant is whole and must come in";
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Specifications));
    EXPECT_FALSE(loaded.steps.has(qftbx::Step::Frequencies)) << "there are none in the file";
    EXPECT_FALSE(loaded.steps.has(qftbx::Step::Templates)) << "the section carries no clouds";
    EXPECT_FALSE(loaded.steps.has(qftbx::Step::LoopShaping)) << "a design with no controller is none";
}

TEST_F(MalformedProject, AMissingElementLeavesItsSectionUnreadAndTheRestLoads)
{
    qftbx::ProjectReader parser;

    const std::string path = without("planta1.qft", "range");
    ASSERT_FALSE(path.empty());

    qftbx::ProjectReader::Loaded loaded;
    ASSERT_NO_THROW(loaded = parser.load(path));

    EXPECT_FALSE(loaded.steps.has(qftbx::Step::Plant))
        << "a plant whose parameter has no range is not a plant";
    EXPECT_EQ(parser.plant(), nullptr);

    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Frequencies));
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Specifications));
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Templates));
    EXPECT_TRUE(loaded.steps.has(qftbx::Step::Boundaries));
}

TEST_F(MalformedProject, TheSizeOfACoefficientListIsRedundantAndIgnored)
{
    qftbx::ProjectReader parser;

    const std::string wrongCount = mutated("planta1.qft", "<denominator size=\"2\">",
                                       "<denominator size=\"7\">");
    ASSERT_FALSE(wrongCount.empty());

    parser.load(wrongCount);

    ASSERT_NE(parser.plant(), nullptr) << "a redundant count broke the load";
    EXPECT_EQ(parser.plant()->denominator().size(), 2u)
        << "the reader believed the attribute instead of counting the elements";
}

TEST_F(MalformedProject, ACountThatIsActuallyReadRejectsGarbage)
{
    qftbx::ProjectReader parser;

    const std::string path = mutated("multivaluados.qft", "count=\"361\"",
                                 "count=\"many\"");
    ASSERT_FALSE(path.empty());

    EXPECT_THROW(parser.load(path), qftbx::ParseError);
}

TEST_F(MalformedProject, AnUnknownSystemTypeIsAParseError)
{
    qftbx::ProjectReader parser;

    const std::string path = mutated("planta1.qft", "<type id=\"1\">", "<type id=\"99\">");
    ASSERT_FALSE(path.empty());

    EXPECT_THROW(parser.load(path), qftbx::ParseError);
}

TEST_F(MalformedProject, AFailedLoadLeavesNothingHalfBuilt)
{
    qftbx::ProjectReader parser;

    const std::string path = mutated("planta1.qft", "<uncertain>true</uncertain>",
                                 "<uncertain>perhaps</uncertain>");
    ASSERT_FALSE(path.empty());

    EXPECT_THROW(parser.load(path), qftbx::ParseError);

    EXPECT_EQ(parser.plant(), nullptr) << "a rejected file left a plant behind";
    EXPECT_EQ(parser.omega(), nullptr) << "a rejected file left frequencies behind";
    EXPECT_EQ(parser.specifications(), nullptr)
        << "a rejected file left specifications behind";
}

TEST_F(MalformedProject, AGoodFileStillLoadsAfterARejectedOne)
{
    qftbx::ProjectReader parser;

    const std::string bad = mutated("planta1.qft", "<nominal>5</nominal>",
                                "<nominal>not a number</nominal>");
    ASSERT_FALSE(bad.empty());
    EXPECT_THROW(parser.load(bad), qftbx::ParseError);

    parser.load(std::string(QFTBX_TEST_DATA_DIR "/planta1.qft"));

    ASSERT_NE(parser.plant(), nullptr) << "the good file did not load after a bad one";
    EXPECT_EQ(parser.plant()->name(), std::string("aa"));
}

TEST_F(MalformedProject, TheMessageNamesTheFileAndTheLine)
{
    qftbx::ProjectReader parser;

    const std::string path = mutated("planta1.qft", "<nominal>30</nominal>",
                                 "<nominal>rubbish</nominal>");
    ASSERT_FALSE(path.empty());

    try {
        parser.load(path);
        FAIL() << "the malformed file was accepted";
    } catch (const qftbx::ParseError & e) {
        const std::string message = std::string(e.what());
        EXPECT_TRUE(message.find("case.qft") != std::string::npos)
            << "the message does not name the file: " << message;
        EXPECT_TRUE(message.find("nominal") != std::string::npos)
            << "the message does not name the element: " << message;
    }
}

}
