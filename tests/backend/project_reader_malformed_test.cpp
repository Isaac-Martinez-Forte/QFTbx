// Adversarial tests of the .qft boundary.
//
// The file loader is the one input surface with no hostile tests: the golden
// tests only ever feed it WELL-FORMED files. The project's robustness policy
// says every external input is validated at the boundary, and the GUI half of
// that is covered by the smoke suite; this is the file half.
//
// Each case MUTATES a real fixture rather than hand-writing XML, so the rest
// of the document stays valid and the test isolates exactly one defect. What
// is asserted is not only that loading fails, but that it fails with a typed
// exception carrying a useful message, and that a failed load leaves nothing
// half-built behind.

#include <gtest/gtest.h>

#include <string>

#include <QByteArray>
#include <QFile>
#include <QTemporaryDir>

#include "src/core/exception.h"
#include "src/persistence/project_reader.h"

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
    //Writes content into the case's own temporary directory and returns its
    //path. The directory goes away with the fixture.
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

    //A fixture with one substring replaced. Fails the test if the substring
    //is not there, so a fixture edit cannot silently turn a case into a
    //no-op that still passes.
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

    QTemporaryDir m_dir;
};

// --- the file itself --------------------------------------------------------

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
    //Cut in the middle of the document: well-formed prefix, no closing tags.
    //This is what a crash or a full disk during a save leaves behind.
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

    const std::string path = mutated("planta1.qft", "<QFT version=\"2\">", "<NotQFT version=\"2\">");
    ASSERT_FALSE(path.empty());

    EXPECT_THROW(parser.load(path), qftbx::ParseError);
}

// --- the contents ----------------------------------------------------------

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

TEST_F(MalformedProject, AMissingRequiredElementIsAParseError)
{
    qftbx::ProjectReader parser;

    //The range of a parameter, removed whole.
    const std::string path = mutated("planta1.qft",
                                 "<range>\n                        <min>1</min>\n"
                                 "                        <max>5</max>\n                    </range>",
                                 "");
    ASSERT_FALSE(path.empty());

    EXPECT_THROW(parser.load(path), qftbx::ParseError);
}

TEST_F(MalformedProject, TheSizeOfACoefficientListIsRedundantAndIgnored)
{
    //The writer emits size="n" on <numerator>/<denominator>, but the reader
    //counts the <parameter> elements and never looks at it. That is the right
    //way round - the data decides, not a header that can disagree with it -
    //and it is pinned here so nobody "optimises" the reader into trusting the
    //attribute, which would make a stale count silently truncate a plant.
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
    //The attributes the reader DOES consume - the Nichols grid counts, the
    //expression size, the loop-shaping point count - must not accept
    //nonsense, because they size the vectors that follow.
    qftbx::ProjectReader parser;

    const std::string path = mutated("multivaluados.qft", "tamFas=\"361\"",
                                 "tamFas=\"many\"");
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

// --- what a failed load leaves behind --------------------------------------

TEST_F(MalformedProject, AFailedLoadLeavesNothingHalfBuilt)
{
    //The reader is reused after a failure - the main window keeps its
    //instance - so a rejected file must not leave a partial plant, frequency
    //set or specification list to be picked up as if it had loaded.
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
    //The state a failure leaves must not poison the next load, which is what
    //a user does: pick the wrong file, get the message, pick the right one.
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
    //A parse error the user cannot locate is barely better than a crash.
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

} // namespace
