#include "src/core/common/record.h"

#include <cstdio>
#include <ctime>
#include <iomanip>
#include <locale>
#include <mutex>
#include <sstream>
#include <sys/stat.h>

namespace qftbx::record {

namespace {

std::mutex & guard()
{
    static std::mutex one;
    return one;
}

struct State {
    std::string path;
    std::size_t limit = 0;
    bool open = false;
};

State & state()
{
    static State one;
    return one;
}

std::string stamp()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  now.time_since_epoch()).count() % 1000;

    std::tm broken{};
    localtime_r(&seconds, &broken);

    char text[32];
    std::strftime(text, sizeof text, "%Y-%m-%d %H:%M:%S", &broken);

    char full[40];
    std::snprintf(full, sizeof full, "%s.%03d", text, int(milliseconds));
    return full;
}

//The directories of the path, made if they are not there. A record nobody
//can write is a record that stays closed, not a computation that fails.
void makeDirectories(const std::string & path)
{
    std::string::size_type at = path.find('/', 1);
    while (at != std::string::npos) {
        ::mkdir(path.substr(0, at).c_str(), 0755);
        at = path.find('/', at + 1);
    }
}

long long sizeOf(const std::string & path)
{
    struct stat information{};
    if (::stat(path.c_str(), &information) != 0) {
        return -1;
    }
    return static_cast<long long>(information.st_size);
}

void rotateIfFull()
{
    const State & now = state();
    if (now.limit == 0) {
        return;
    }

    const long long size = sizeOf(now.path);
    if (size < 0 || static_cast<std::size_t>(size) < now.limit) {
        return;
    }

    const std::string previous = now.path + ".1";
    std::remove(previous.c_str());
    std::rename(now.path.c_str(), previous.c_str());
}

std::ostringstream plainStream()
{
    std::ostringstream text;
    text.imbue(std::locale::classic());
    return text;
}

} // namespace

void open(const std::string & path, std::size_t sizeLimitBytes)
{
    const std::lock_guard<std::mutex> held(guard());

    if (path.empty()) {
        state() = State();
        return;
    }

    makeDirectories(path);

    std::FILE * file = std::fopen(path.c_str(), "a");
    if (file == nullptr) {
        state() = State();
        return;
    }
    std::fclose(file);

    state().path = path;
    state().limit = sizeLimitBytes;
    state().open = true;
}

void close()
{
    const std::lock_guard<std::mutex> held(guard());
    state() = State();
}

bool isOpen()
{
    const std::lock_guard<std::mutex> held(guard());
    return state().open;
}

const std::string & path()
{
    const std::lock_guard<std::mutex> held(guard());
    static const std::string none;
    return state().open ? state().path : none;
}

void write(const std::string & stage, const std::string & detail, const std::string & numbers)
{
    const std::lock_guard<std::mutex> held(guard());
    if (!state().open) {
        return;
    }

    rotateIfFull();

    std::FILE * file = std::fopen(state().path.c_str(), "a");
    if (file == nullptr) {
        return;
    }

    std::fprintf(file, "%s  %-14s %-12s %s\n", stamp().c_str(), stage.c_str(),
                 detail.c_str(), numbers.c_str());
    std::fclose(file);
}

Timed::Timed(std::string stage, std::string detail)
    : m_stage(std::move(stage)), m_detail(std::move(detail)),
      m_started(std::chrono::steady_clock::now())
{
}

void Timed::note(const std::string & numbers)
{
    if (numbers.empty()) {
        return;
    }
    if (!m_numbers.empty()) {
        m_numbers += ' ';
    }
    m_numbers += numbers;
}

double Timed::elapsedMilliseconds() const
{
    return std::chrono::duration<double, std::milli>(
               std::chrono::steady_clock::now() - m_started).count();
}

Timed::~Timed()
{
    if (!isOpen()) {
        return;
    }

    std::string line = milliseconds(elapsedMilliseconds());
    if (!m_numbers.empty()) {
        line += ' ';
        line += m_numbers;
    }

    write(m_stage, m_detail, line);
}

std::string milliseconds(double value)
{
    std::ostringstream text = plainStream();
    text << "ms=" << std::fixed << std::setprecision(1) << value;
    return text.str();
}

std::string number(const std::string & key, double value)
{
    std::ostringstream text = plainStream();
    text << key << '=' << std::setprecision(10) << value;
    return text.str();
}

std::string number(const std::string & key, long long value)
{
    std::ostringstream text = plainStream();
    text << key << '=' << value;
    return text.str();
}

std::string number(const std::string & key, int value)
{
    return number(key, static_cast<long long>(value));
}

std::string number(const std::string & key, std::size_t value)
{
    return number(key, static_cast<long long>(value));
}

} // namespace qftbx::record
