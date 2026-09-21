/**
 * @file
 * @brief Finding the shipped examples and reading what each one says it is.
 *
 * The name and the description are attributes of the root element, so the
 * pull parser is stopped at the first start element and the rest of the file,
 * which is the templates and the boundaries and weighs most of a megabyte, is
 * never read. A file whose root is not a project is skipped rather than
 * reported: the directory is ours, and what does not belong in it is not the
 * user's mistake to fix. The identifier of the article is checked against the
 * shape of a DOI and dropped when it does not fit, so the address the
 * interface offers is always one this code built.
 */

#include "src/gui/common/examples_library.h"

#include <algorithm>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QXmlStreamReader>

namespace qftbx {

namespace {

bool readHeader(const QString & path, QString & name, QString & description, QString & doi)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QXmlStreamReader xml(&file);
    while (!xml.atEnd()) {
        if (xml.readNext() != QXmlStreamReader::StartElement) {
            continue;
        }

        if (xml.name() != QLatin1String("QFT")) {
            return false;
        }

        const QXmlStreamAttributes attributes = xml.attributes();
        name = attributes.value(QLatin1String("name")).toString();
        description = attributes.value(QLatin1String("description")).toString();
        doi = attributes.value(QLatin1String("doi")).toString();

        return true;
    }

    return false;
}

QString directoryIfItExists(const QString & path)
{
    if (path.isEmpty()) {
        return QString();
    }

    const QFileInfo info(path);

    return info.isDir() ? info.canonicalFilePath() : QString();
}

}

QString doiAddress(const QString & doi)
{
    static const QRegularExpression shape(
                QStringLiteral("\\A10\\.\\d{4,9}/[-._;()/:A-Za-z0-9]+\\z"));

    if (!shape.match(doi).hasMatch()) {
        return QString();
    }

    return QStringLiteral("https://doi.org/") + doi;
}

QString examplesDirectory()
{
    const QString given = directoryIfItExists(
                qEnvironmentVariable("QFTBX_EXAMPLES_DIR"));
    if (!given.isEmpty()) {
        return given;
    }

    const QString installed = directoryIfItExists(
                QDir(QCoreApplication::applicationDirPath())
                .filePath(QStringLiteral(QFTBX_EXAMPLES_RELATIVE_DIR)));
    if (!installed.isEmpty()) {
        return installed;
    }

    return directoryIfItExists(QStringLiteral(QFTBX_EXAMPLES_SOURCE_DIR));
}

std::vector<Example> shippedExamples()
{
    std::vector<Example> examples;

    const QString directory = examplesDirectory();
    if (directory.isEmpty()) {
        return examples;
    }

    const QFileInfoList files = QDir(directory).entryInfoList(
                {QStringLiteral("*.qft")}, QDir::Files, QDir::Name);

    for (const QFileInfo & file : files) {
        Example example;
        example.path = file.absoluteFilePath();

        if (!readHeader(example.path, example.name, example.description, example.doi)) {
            continue;
        }

        if (doiAddress(example.doi).isEmpty()) {
            example.doi.clear();
        }

        if (example.name.isEmpty()) {
            example.name = file.completeBaseName();
        }

        examples.push_back(example);
    }

    std::sort(examples.begin(), examples.end(),
              [](const Example & a, const Example & b) {
        return a.name.localeAwareCompare(b.name) < 0;
    });

    return examples;
}

bool isShippedExample(const QString & path)
{
    const QString directory = examplesDirectory();
    if (directory.isEmpty() || path.isEmpty()) {
        return false;
    }

    const QFileInfo file(path);
    const QString parent = file.absoluteDir().canonicalPath();

    return !parent.isEmpty() && parent == directory;
}

}
