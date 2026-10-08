/**
 * @file
 * @brief The design problems that ship with the program, and where they
 * live.
 *
 * The examples travel with the installation and are never written to, so
 * the interface has to find them wherever the packaging of each system put
 * them and offer them by name rather than as a path the user has to know.
 * examplesDirectory() is the first that exists of the environment variable
 * QFTBX_EXAMPLES_DIR, the path from the executable the build baked in, and
 * the examples of the source tree, and empty when none does. Each example
 * is read only as far as the root element, which carries its name, its
 * description and the article it comes from, so listing them costs nothing
 * next to opening one; the name falls back to the file's when the project
 * carries none, and the DOI is empty when the file names none or names one
 * that is not a DOI.
 * shippedExamples() is ordered by name, and empty when the directory is
 * missing, which the interface reports. doiAddress() builds the address
 * from the identifier, never from the file, so a project can never make
 * the interface follow an arbitrary address; it is empty when the text is
 * not a DOI. The saving path asks isShippedExample() so that it never
 * writes over a file that came with the program.
 */

#ifndef QFTBX_GUI_EXAMPLES_LIBRARY_H
#define QFTBX_GUI_EXAMPLES_LIBRARY_H

#include <vector>

#include <QString>

namespace qftbx {

struct Example
{
    QString name;
    QString description;
    QString doi;
    QString path;
};

QString doiAddress(const QString & doi);

QString examplesDirectory();

std::vector<Example> shippedExamples();

bool isShippedExample(const QString & path);

}

#endif
