/**
 * @file
 * @brief Writes the verdict, the stability and the search of a design as
 * one report.
 *
 * A list of headings, paragraphs and tables, drawn as rich text for the tab
 * and as plain text for the clipboard, so the two never say different
 * things. Numbers in exponent notation are raised to powers of ten in the
 * tab and left as a spreadsheet reads them in the plain text. Levels in
 * decibels go at the fixed resolution of shownDbText(), so the margin of a
 * design on its bound, the tolerance the exact reading keeps from it, reads
 * as on the bound and not as a margin, and an excess too small to show is
 * said to be less than the resolution, never zero. The table gives the
 * margin, the bound less the value, as a designer reads it: positive where
 * a specification is met, and negative, in the theme's red, where it is
 * exceeded, with the digits it has when it is too small to show. A change of theme draws
 * the same blocks again without reading the result, which may be gone with
 * a project being replaced. A result read from a file reports what it did
 * when it was found; only a file older than the certificate lacks it. The
 * distance to the least gain is the returned gain over the certificate's
 * lower bound, which counts the boxes the nominal criterion rejected on its
 * grid; the higher bound that takes them as right is shown apart.
 */

#include "src/gui/loopshaping/loop_shaping_report.h"

#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QPalette>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

#include "src/core/loopshaping/algorithm_name.h"
#include "src/gui/application/theme.h"
#include "src/gui/common/field_mark.h"
#include "src/gui/common/number_text.h"

namespace qftbx {

LoopShapingReport::LoopShapingReport(QWidget * parent)
    : QWidget(parent)
{
    auto * layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_verdict = new QLabel(this);
    m_verdict->setObjectName("reportVerdict");
    m_verdict->setTextFormat(Qt::RichText);
    m_verdict->setWordWrap(true);
    m_verdict->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_verdict);

    m_text = new QTextBrowser(this);
    m_text->setObjectName("reportText");
    m_text->setOpenLinks(false);
    layout->addWidget(m_text, 1);

    auto * buttons = new QHBoxLayout();
    buttons->addStretch(1);
    m_copy = new QPushButton(tr("Copy the report"), this);
    m_copy->setObjectName("reportCopy");
    m_copy->setToolTip(tr("Puts the whole report on the clipboard, with its tables."));
    buttons->addWidget(m_copy);
    layout->addLayout(buttons);

    connect(m_copy, &QPushButton::clicked, this, &LoopShapingReport::copy);

    clear();
}

void LoopShapingReport::setResult(const LoopShapingResult * result)
{
    m_result = result;
    refresh();
}

void LoopShapingReport::clear()
{
    setResult(nullptr);
}

void LoopShapingReport::refresh()
{
    m_blocks = blocks();

    if (m_result == nullptr) {
        m_verdict->clear();
        m_verdict->setToolTip(QString());
        markAs(m_verdict, "verdict", QString());
        m_text->setHtml("<p>" + tr("No design yet: the report appears when the search returns one.").toHtmlEscaped() + "</p>");
        m_copy->setEnabled(false);
        return;
    }

    m_verdict->setText(withPowersOfTen(summaryOf(m_result->check())));
    m_verdict->setToolTip("<p>" + withPowersOfTen(verdictOf(m_result->check())) + "</p>");
    markAs(m_verdict, "verdict", markOf(m_result->check()));
    m_text->setHtml(html());
    m_copy->setEnabled(true);
}

void LoopShapingReport::changeEvent(QEvent * event)
{
    if (event->type() == QEvent::PaletteChange && !m_blocks.empty()) {
        m_text->setHtml(html());
    }
    QWidget::changeEvent(event);
}

void LoopShapingReport::copy()
{
    if (m_blocks.empty()) {
        return;
    }
    auto * data = new QMimeData();
    data->setHtml(html());
    data->setText(plainText());
    QApplication::clipboard()->setMimeData(data);
}

QString LoopShapingReport::summaryOf(const std::optional<SpecificationCheck> & check)
{
    if (!check.has_value()) {
        return tr("Not checked against the specifications.");
    }

    QStringList parts;
    if (!std::isfinite(check->worstExcessDb)) {
        parts << tr("No specification active");
    } else if (check->worstExcessDb <= 0.0) {
        parts << tr("Meets");
    } else if (const SpecificationExcess * worst = tightest(*check)) {
        parts << tr("Exceeds by %1 dB at w = %2").arg(excessText(worst->excessDb), shownText(worst->omega));
    } else {
        parts << tr("Exceeds by %1 dB").arg(excessText(check->worstExcessDb));
    }

    if (check->nominal.checked && !check->nominal.stable) {
        parts << tr("nominal UNSTABLE");
    }

    if (!check->family.checked) {
        parts << tr("family not checked");
    } else if (check->family.unstableMembers == 0) {
        parts << tr("%1/%2 stable").arg(check->family.members).arg(check->family.members);
    } else {
        parts << tr("%1/%2 UNSTABLE").arg(check->family.unstableMembers).arg(check->family.members);
    }

    return (check->satisfied() ? QStringLiteral("✓ ") : QStringLiteral("✗ ")) + parts.join(QStringLiteral(" · "));
}

QString LoopShapingReport::verdictOf(const std::optional<SpecificationCheck> & check)
{
    if (!check.has_value()) {
        return tr("Not checked against the specifications (no templates to check over).");
    }

    QStringList sentences{specificationsText(*check), familyText(check->family)};
    if (check->nominal.checked) {
        sentences << nominalText(check->nominal);
    }
    return sentences.join(' ');
}

QString LoopShapingReport::specificationsText(const SpecificationCheck & check)
{
    if (!std::isfinite(check.worstExcessDb)) {
        return tr("No specification was active at any design frequency.");
    }
    if (const SpecificationExcess * worst = tightest(check)) {
        const QString omega = shownText(worst->omega);
        const QString title = specificationTitle(worst->type);
        if (worst->excessDb > 0.0) {
            return tr("EXCEEDS a specification over the template: w = %1 rad/s, %2, by %3 dB.")
                   .arg(omega, title, excessText(worst->excessDb));
        }
        return shownAsZeroDb(worst->excessDb)
                ? tr("Satisfies every specification over the template; the tightest, %2 at w = %1 rad/s, sits on its bound.")
                  .arg(omega, title)
                : tr("Satisfies every specification over the template: tightest at w = %1 rad/s, %2, %3 dB of margin.")
                  .arg(omega, title, shownDbText(-worst->excessDb));
    }
    if (check.worstExcessDb > 0.0) {
        return tr("EXCEEDS a specification over the template by %1 dB.").arg(excessText(check.worstExcessDb));
    }
    return shownAsZeroDb(check.worstExcessDb)
            ? tr("Satisfies every specification over the template; the tightest sits on its bound.")
            : tr("Satisfies every specification over the template, by %1 dB.").arg(shownDbText(-check.worstExcessDb));
}

QString LoopShapingReport::marginText(double excessDb)
{
    return excessDb > 0.0 && shownAsZeroDb(excessDb) ? "-" + shownText(excessDb) : shownDbText(-excessDb);
}

QString LoopShapingReport::excessText(double excessDb)
{
    return shownAsZeroDb(excessDb) ? tr("less than %1").arg(shownDbText(shownDbResolution())) : shownDbText(excessDb);
}

QString LoopShapingReport::markOf(const std::optional<SpecificationCheck> & check)
{
    if (!check.has_value()) {
        return QString();
    }
    const bool unstable = check->family.unstableMembers > 0 || (check->nominal.checked && !check->nominal.stable);
    if (unstable || check->worstExcessDb > 0.0) {
        return QStringLiteral("exceeded");
    }
    return std::isfinite(check->worstExcessDb) ? QStringLiteral("met") : QString();
}

LoopShapingReport::Block LoopShapingReport::heading(const QString & text)
{
    Block block;
    block.kind = Block::Heading;
    block.text = text;
    return block;
}

LoopShapingReport::Block LoopShapingReport::paragraph(const QString & text)
{
    Block block;
    block.text = text;
    return block;
}

std::vector<LoopShapingReport::Block> LoopShapingReport::blocks() const
{
    std::vector<Block> out;
    if (m_result == nullptr) {
        return out;
    }
    addSpecifications(out);
    addStability(out);
    addSearch(out);
    return out;
}

void LoopShapingReport::addSpecifications(std::vector<Block> & out) const
{
    out.push_back(heading(tr("Specifications")));

    const std::optional<SpecificationCheck> & check = m_result->check();
    if (!check.has_value()) {
        out.push_back(paragraph(tr("Not checked against the specifications (no templates to check over).")));
        return;
    }

    out.push_back(paragraph(specificationsText(*check)));

    if (check->entries.empty()) {
        return;
    }

    Block table;
    table.kind = Block::Table;
    table.header = QStringList{tr("w (rad/s)"), tr("Specification"), tr("Value (dB)"), tr("Bound (dB)"), tr("Margin (dB)")};
    const SpecificationExcess * worst = tightest(*check);
    for (const SpecificationExcess & e : check->entries) {
        const int row = static_cast<int>(table.rows.size());
        if (&e == worst) {
            table.highlighted = row;
        }
        if (e.excessDb > 0.0) {
            table.exceeded.push_back(row);
        }
        table.rows.push_back(QStringList{shownText(e.omega), specificationTitle(e.type), shownDbText(e.valueDb),
                                         shownDbText(e.boundDb), marginText(e.excessDb)});
    }
    out.push_back(table);
}

void LoopShapingReport::addStability(std::vector<Block> & out) const
{
    out.push_back(heading(tr("Closed-loop stability")));

    const std::optional<SpecificationCheck> & check = m_result->check();
    if (!check.has_value()) {
        out.push_back(paragraph(tr("Not checked: there are no templates, and so no record of the plants of the sweep.")));
        return;
    }

    out.push_back(paragraph(familyText(check->family)));

    if (check->family.checked && !check->family.worstMember.empty()) {
        QStringList values;
        for (const auto & [name, value] : check->family.worstMember) {
            values << QString::fromStdString(name) + " = " + shownText(value);
        }
        out.push_back(paragraph(tr("The plant nearest to instability: %1.").arg(values.join(", "))));
    }

    if (check->nominal.checked) {
        out.push_back(paragraph(nominalText(check->nominal)));
    }
}

void LoopShapingReport::addSearch(std::vector<Block> & out) const
{
    out.push_back(heading(tr("Search")));

    const LoopShapingResult::Run & run = m_result->run();
    QString algorithm = QString::fromLatin1(algorithmName(run.algorithm)).toUpper();
    algorithm.replace('_', '-');
    QString how = tr("Algorithm %1, epsilon %2.").arg(algorithm, shownText(run.epsilon));
    if (run.pointReading == Settings::Research::PointReading::Exact && run.algorithm == mc2) {
        how += " " + tr("The points were read exactly over the template.");
    } else if (run.conservativeColumns) {
        how += " " + tr("The boundaries were read on their phase grid, at the worst of the columns around each phase.");
    }
    out.push_back(paragraph(how));

    addCertificate(out);
}

void LoopShapingReport::addCertificate(std::vector<Block> & out) const
{
    out.push_back(heading(tr("Certificate")));

    const LoopShapingStatistics::Certificate & certificate = m_result->statistics().certificate;
    if (!certificate.kept) {
        out.push_back(paragraph(m_result->run().algorithm == mc2
                                ? tr("This file does not carry the certificate of the search: it was saved before QFTbx kept it. Run the search again to have it.")
                                : tr("This algorithm keeps no certificate: nothing bounds how far its gain is from the least one.")));
        return;
    }
    if (!certificate.finished) {
        out.push_back(paragraph(tr("The search did not reach its end, so the certificate bounds nothing.")));
        return;
    }

    const double gain = m_result->controller()->gain().nominal();
    const double bound = certificate.lowerBoundStrict;
    const auto distance = [gain](double below) {
        return QStringList{shownText(100.0 * (gain / below - 1.0)), shownText(20.0 * std::log10(gain / below))};
    };

    if (!std::isfinite(bound)) {
        out.push_back(paragraph(tr("Nothing was set aside undecided, so no design of the search box has a gain below %1: this design has the least.")
                                .arg(shownText(gain))));
    } else if (bound > 0.0 && gain > 0.0) {
        const QStringList away = distance(bound);
        out.push_back(paragraph(tr("No design of the search box has a gain below %1, so this one, with %2, is at most %3 % (%4 dB) above the least.")
                                .arg(shownText(bound), shownText(gain), away.at(0), away.at(1))));
    } else {
        out.push_back(paragraph(tr("No design of the search box has a gain below %1; this one has %2.")
                                .arg(shownText(bound), shownText(gain))));
    }

    if (certificate.lowerBound > bound) {
        const QString trusted = shownText(certificate.lowerBound);
        out.push_back(paragraph(std::isfinite(certificate.lowerBound) && certificate.lowerBound > 0.0 && gain > 0.0
                                ? tr("Taking as right the boxes the nominal criterion rejected on its grid, which is no proof, the bound rises to %1 (%2 %).")
                                  .arg(trusted, distance(certificate.lowerBound).at(0))
                                : tr("Taking as right the boxes the nominal criterion rejected on its grid, which is no proof, the bound rises to %1.")
                                  .arg(trusted)));
    }

    const std::size_t undecided = certificate.epsilonResolved + certificate.residueNodes + certificate.unprovenDiscards
                                  + certificate.gridBackedPrunes;
    if (std::isfinite(bound) && undecided > 0) {
        out.push_back(paragraph(tr("The bound comes from %1 boxes the search set aside without deciding them, each counted with the least gain it could hold.")
                                .arg(undecided)));
    }

    out.push_back(paragraph(certificate.exactPoints
                            ? tr("It holds for the plants of the template at the design frequencies, not between them.")
                            : tr("It holds for the plants of the template at the design frequencies, not between them, and for the boundaries as read on their phase grid.")));
}

QString LoopShapingReport::html() const
{
    const QString shade = palette().color(QPalette::AlternateBase).name();
    const QString wrong = wrongColour(palette());
    QString out;
    for (const Block & block : m_blocks) {
        switch (block.kind) {
        case Block::Heading:
            out += "<h3>" + block.text.toHtmlEscaped() + "</h3>";
            break;
        case Block::Paragraph:
            out += "<p>" + withPowersOfTen(block.text) + "</p>";
            break;
        case Block::Table: {
            out += "<table cellspacing=\"0\" cellpadding=\"4\"><tr>";
            for (int column = 0; column < block.header.size(); ++column) {
                out += QString("<th align=\"%1\">").arg(column == 1 ? "left" : "right")
                       + block.header.at(column).toHtmlEscaped() + "</th>";
            }
            out += "</tr>";
            for (int row = 0; row < static_cast<int>(block.rows.size()); ++row) {
                out += row % 2 == 0 ? QString("<tr bgcolor=\"%1\">").arg(shade) : QString("<tr>");
                const QStringList & cells = block.rows.at(static_cast<std::size_t>(row));
                for (int column = 0; column < cells.size(); ++column) {
                    QString cell = withPowersOfTen(cells.at(column));
                    if (row == block.highlighted) {
                        cell = "<b>" + cell + "</b>";
                    }
                    if (std::find(block.exceeded.begin(), block.exceeded.end(), row) != block.exceeded.end()) {
                        cell = QString("<span style=\"color:%1\">").arg(wrong) + cell + "</span>";
                    }
                    out += QString("<td align=\"%1\">").arg(column == 1 ? "left" : "right") + cell + "</td>";
                }
                out += "</tr>";
            }
            out += "</table>";
            break;
        }
        }
    }
    return out;
}

QString LoopShapingReport::plainText() const
{
    QStringList out;
    for (const Block & block : m_blocks) {
        switch (block.kind) {
        case Block::Heading:
            out << (out.isEmpty() ? QString() : QStringLiteral("\n")) + block.text;
            break;
        case Block::Paragraph:
            out << block.text;
            break;
        case Block::Table:
            out << block.header.join('\t');
            for (const QStringList & cells : block.rows) {
                out << cells.join('\t');
            }
            break;
        }
    }
    return out.join('\n') + (out.isEmpty() ? QString() : QStringLiteral("\n"));
}

const SpecificationExcess * LoopShapingReport::tightest(const SpecificationCheck & check)
{
    const SpecificationExcess * worst = nullptr;
    for (const SpecificationExcess & e : check.entries) {
        if (worst == nullptr || e.excessDb > worst->excessDb) {
            worst = &e;
        }
    }
    return worst;
}

QString LoopShapingReport::familyText(const FamilyStability & family)
{
    if (!family.checked) {
        switch (family.notChecked) {
        case FamilyStability::NotChecked::NoSweepRecord:
            return tr("Closed-loop stability of the family not checked: the project has no record of the sweep; recompute the templates.");
        case FamilyStability::NotChecked::Delay:
            return tr("Closed-loop stability of the family not checked: the loop has a delay.");
        case FamilyStability::NotChecked::NotRational:
            return tr("Closed-loop stability of the family not checked: the loop is not a rational function.");
        case FamilyStability::NotChecked::No:
            break;
        }
        return tr("Closed-loop stability of the family not checked.");
    }
    if (family.unstableMembers == 0) {
        return tr("Every one of the %1 plants is closed-loop stable (worst real part %2).")
               .arg(family.members).arg(shownText(family.worstRealPart));
    }
    return tr("%1 of the %2 plants are CLOSED-LOOP UNSTABLE (worst real part %3).")
           .arg(family.unstableMembers).arg(family.members).arg(shownText(family.worstRealPart));
}

QString LoopShapingReport::nominalText(const NominalStability & nominal)
{
    if (nominal.stable) {
        return tr("The nominal closed loop is stable (worst real part %1).").arg(shownText(nominal.worstRealPart));
    }
    return tr("The NOMINAL CLOSED LOOP IS UNSTABLE (worst real part %1).").arg(shownText(nominal.worstRealPart));
}

QString LoopShapingReport::specificationTitle(SpecificationType type)
{
    switch (type) {
    case SpecificationType::TrackingLower:
    case SpecificationType::TrackingUpper:     return tr("tracking");
    case SpecificationType::Stability:         return tr("stability");
    case SpecificationType::SensorNoise:       return tr("sensor noise");
    case SpecificationType::OutputDisturbance: return tr("output disturbance");
    case SpecificationType::InputDisturbance:  return tr("input disturbance");
    case SpecificationType::ControlEffort:     return tr("control effort");
    }
    return QString();
}

}
