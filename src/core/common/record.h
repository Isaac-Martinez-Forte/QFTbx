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
 * the tests and anything linking the core without asking are silent.
 *
 * open() with an empty path closes the record, and a record that fails to
 * open simply stays closed, without an error; path() is empty while it is
 * closed. A line's stage is what ran ("templates", "loop shaping"), its
 * detail what it ran on (the algorithm's name where there is one), and its
 * numbers "key=value" pairs separated by spaces. number() writes the value
 * the same way whatever the locale, milliseconds() to the tenth of a
 * millisecond. Timed is declared where a stage begins and writes its line
 * when destroyed, where the stage ends; note() appends numbers to it.
 */

#ifndef QFTBX_RECORD_H
#define QFTBX_RECORD_H

#include <chrono>
#include <cstddef>
#include <string>

namespace qftbx::record {

void open(const std::string & path, std::size_t sizeLimitBytes);

void close();

bool isOpen();

const std::string & path();

void write(const std::string & stage, const std::string & detail,
           const std::string & numbers = std::string());

class Timed
{
public:
    Timed(std::string stage, std::string detail);
    ~Timed();

    Timed(const Timed &) = delete;
    Timed & operator=(const Timed &) = delete;

    void note(const std::string & numbers);

    double elapsedMilliseconds() const;

private:
    std::string m_stage;
    std::string m_detail;
    std::string m_numbers;
    std::chrono::steady_clock::time_point m_started;
};

std::string milliseconds(double value);

std::string number(const std::string & key, double value);
std::string number(const std::string & key, long long value);
std::string number(const std::string & key, int value);
std::string number(const std::string & key, std::size_t value);

}

#endif
