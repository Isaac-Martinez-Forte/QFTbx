/**
 * @file
 * @brief The record of what the engines did and how long it took.
 *
 * A file of one line per stage - when it ran, what ran, how long it took
 * and the numbers worth keeping - written by the engines instead of on
 * standard output, so nothing prints on somebody else's terminal and the
 * lines of parallel regions cannot interleave. The file is capped: past its
 * size limit it becomes the previous generation and a new one starts, two
 * generations in all. Nothing is written until an application opens it, so
 * the tests and anything linking the core without asking are silent. A
 * scoped timer writes a stage's line when the stage ends.
 */

#ifndef QFTBX_RECORD_H
#define QFTBX_RECORD_H

/// The record of what the engines did and how long it took.
/// 
/// Timings are not printed on standard output, which would be a library
/// writing on somebody else's terminal: nothing would show it in the
/// interface, nothing could silence it, the lines of parallel regions would
/// interleave, and anything that drives the core from another language would
/// get them in the middle of its own session. They come here instead.
/// 
/// The record is a file of one line per stage - when it ran, what ran, how
/// long it took and the numbers worth keeping - and it is capped: past its
/// size limit the file becomes the previous generation and a new one starts,
/// so the oldest lines fall off the end and the record never grows without
/// bound. Two generations, so the space it takes is twice the limit and no
/// more.
/// 
/// Nothing is written until an application opens it: the tests, and anything
/// that links the core without asking for a record, are silent.

#include <chrono>
#include <cstddef>
#include <string>

namespace qftbx::record {

/// Opens the record at that path, rotating it past that many bytes. An
/// empty path closes it. Failing to open is not an error worth stopping a
/// computation for: the record simply stays closed.
void open(const std::string & path, std::size_t sizeLimitBytes);

void close();

bool isOpen();

/// The path the record is being written to, empty when it is closed.
const std::string & path();

/// One line. The stage is what ran ("templates", "boundaries", "loop
/// shaping"), the detail what it ran on or with - the name of the algorithm
/// where there is one - and the numbers whatever is worth keeping, written
/// as "key=value" separated by spaces.
void write(const std::string & stage, const std::string & detail,
           const std::string & numbers = std::string());

/**
 * @brief Times a stage and writes its line when it ends.
 *
 * Declared where the stage begins and destroyed where it ends, which is
 * what makes the duration in the record the duration of the stage and not
 * of whatever the caller remembered to measure.
 */
class Timed
{
public:
    Timed(std::string stage, std::string detail);
    ~Timed();

    Timed(const Timed &) = delete;
    Timed & operator=(const Timed &) = delete;

    /// Numbers to put on the line, appended to what is already there.
    void note(const std::string & numbers);

    /// Milliseconds since it was declared.
    double elapsedMilliseconds() const;

private:
    std::string m_stage;
    std::string m_detail;
    std::string m_numbers;
    std::chrono::steady_clock::time_point m_started;
};

/// "ms=368.4": a duration, at the tenth of a millisecond that is the most
/// anybody reads off a record of stages.
std::string milliseconds(double value);

/// "key=value", with the value written the same way whatever the locale.
std::string number(const std::string & key, double value);
std::string number(const std::string & key, long long value);
std::string number(const std::string & key, int value);
std::string number(const std::string & key, std::size_t value);

}

#endif
