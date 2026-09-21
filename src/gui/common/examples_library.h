/**
 * @file
 * @brief The design problems that ship with the program, and where they live.
 *
 * The examples travel with the installation and are never written to, so the
 * interface has to find them wherever the packaging of each system put them
 * and offer them by name rather than as a path the user has to know. The
 * directory is resolved from the executable, with the tree itself standing in
 * for a build and an environment variable overriding both. Each example is
 * read only as far as the root element, which carries its name, its
 * description and the article it comes from, so listing them costs nothing
 * next to opening one.
 */

#ifndef QFTBX_GUI_EXAMPLES_LIBRARY_H
#define QFTBX_GUI_EXAMPLES_LIBRARY_H

#include <vector>

#include <QString>

namespace qftbx {

/**
 * @brief One shipped design problem: what to call it, what it is, and the
 * file it is in.
 *
 * The name falls back to the file's own when the project carries none, so a
 * file dropped into the directory by hand still lists.
 */
struct Example
{
    QString name;
    QString description;
    /// The digital object identifier of the work the problem is posed in,
    /// empty when the file names none or names one that is not a DOI.
    QString doi;
    QString path;
};

/**
 * @brief The address a DOI is read at, empty when the text is not one.
 *
 * The interface offers the source of an example as a link, and the address
 * is built here from the identifier rather than taken from the file, so
 * what a project carries can never become an arbitrary address to follow.
 */
QString doiAddress(const QString & doi);

/**
 * @brief The directory the shipped examples are in, empty when there is none.
 *
 * Three places, in order: the environment variable `QFTBX_EXAMPLES_DIR`,
 * which a test or a packager sets; the path from the executable that the
 * build baked in, which is what an installed program uses; and the examples
 * of the source tree, which is what a build has. The first that exists wins.
 */
QString examplesDirectory();

/**
 * @brief Every example of that directory, ordered by name.
 *
 * Empty when the directory is missing, which is what an incomplete
 * installation looks like and what the interface reports instead of
 * offering nothing without saying why.
 */
std::vector<Example> shippedExamples();

/**
 * @brief Whether a file is one of the shipped examples.
 *
 * What the saving path asks before it writes: a copy of an example goes
 * wherever the user says, and never over the file that came with the
 * program.
 */
bool isShippedExample(const QString & path);

}

#endif
