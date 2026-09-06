#include "src/gui/bench/benchmark_run.h"

#include <exception>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QStandardPaths>

#include "src/core/common/exception.h"

namespace qftbx {

BenchmarkRun::BenchmarkRun(QObject * parent) : QObject(parent) {}

BenchmarkRun::~BenchmarkRun()
{
    cancel();
    join();
}

void BenchmarkRun::setHandlers(StartedHandler started, FinishedHandler finished, DoneHandler done)
{
    m_started = std::move(started);
    m_finished = std::move(finished);
    m_done = std::move(done);
}

namespace {
QString g_workerProgram;
}

void BenchmarkRun::setWorkerProgram(const QString & program)
{
    g_workerProgram = program;
}

QString BenchmarkRun::workerProgram()
{
    if (!g_workerProgram.isEmpty()) {
        return g_workerProgram;
    }
    const QString name = QStringLiteral("qftbx-bench");
    const QString beside = QDir(QCoreApplication::applicationDirPath()).filePath(name);
    if (QFileInfo(beside).isExecutable()) {
        return beside;
    }
    return QStandardPaths::findExecutable(name);
}

void BenchmarkRun::join()
{
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void BenchmarkRun::start(const bench::Plan & plan, const QString & planPath, int jobs)
{
    if (m_running.load()) {
        throw InvalidInput("A benchmark run is in progress.");
    }
    const QString worker = workerProgram();
    if (worker.isEmpty()) {
        throw InvalidInput("The benchmark tool qftbx-bench is neither next to the application nor on the path.");
    }
    join();

    m_running.store(true);
    m_thread = std::thread([this, plan, path = planPath.toStdString(), worker = worker.toStdString(), jobs]() {
        std::size_t failures = 0;
        QString error;
        try {
            failures = m_runner.run(plan, path, worker, jobs, [this](const bench::Runner::Event & event) {
                //Copies: the runner's case and record live on its thread.
                if (event.kind == bench::Runner::Event::Kind::Started && event.c != nullptr) {
                    const bench::Case c = *event.c;
                    QMetaObject::invokeMethod(this, [this, c]() {
                        if (m_started) {
                            m_started(c);
                        }
                    }, Qt::QueuedConnection);
                } else if (event.kind == bench::Runner::Event::Kind::Finished && event.c != nullptr && event.record != nullptr) {
                    const bench::Case c = *event.c;
                    const bench::Record record = *event.record;
                    QMetaObject::invokeMethod(this, [this, c, record]() {
                        if (m_finished) {
                            m_finished(c, record);
                        }
                    }, Qt::QueuedConnection);
                }
            });
        } catch (const std::exception & failure) {
            error = QString::fromUtf8(failure.what());
        }
        QMetaObject::invokeMethod(this, [this, failures, error]() {
            m_running.store(false);
            if (m_done) {
                m_done(failures, error);
            }
        }, Qt::QueuedConnection);
    });
}

void BenchmarkRun::cancel()
{
    if (m_running.load()) {
        m_runner.cancel();
    }
}

} // namespace qftbx
