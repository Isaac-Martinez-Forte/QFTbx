/**
 * @file
 * @brief The whole verdict on a designed controller, beside its loop.
 *
 * Declares the tab that holds what the diagram's column has no room for:
 * the controller checked against the specifications, with the table of
 * every frequency and specification; the closed-loop stability of the
 * family and of the nominal plant; and the search that produced it, with
 * the certificate MC2 keeps, the least gain a design of the search box can
 * have and how far the returned one is from it. What the search cost is
 * not part of it. summaryOf() and
 * verdictOf() give the one line the column shows and the sentence behind
 * it. The text can be selected, and Copy puts it on the clipboard as rich
 * text and as plain text with tab-separated tables.
 */

#ifndef QFTBX_LOOP_SHAPING_REPORT_H
#define QFTBX_LOOP_SHAPING_REPORT_H

#include <optional>
#include <vector>

#include <QString>
#include <QStringList>
#include <QWidget>

#include "src/core/loopshaping/common/specification_checker.h"
#include "src/core/loopshaping/loop_shaping_result.h"

class QEvent;
class QLabel;
class QPushButton;
class QTextBrowser;

namespace qftbx {

class LoopShapingReport : public QWidget
{
    Q_OBJECT

public:
    explicit LoopShapingReport(QWidget * parent = nullptr);

    void setResult(const LoopShapingResult * result);

    void refresh();

    void clear();

    QString plainText() const;

    static QString summaryOf(const std::optional<SpecificationCheck> & check);

    static QString verdictOf(const std::optional<SpecificationCheck> & check);

    static QString markOf(const std::optional<SpecificationCheck> & check);

protected:
    void changeEvent(QEvent * event) override;

private slots:
    void copy();

private:
    struct Block
    {
        enum Kind { Heading, Paragraph, Table };
        Kind kind = Paragraph;
        QString text;
        QStringList header;
        std::vector<QStringList> rows;
        int highlighted = -1;
        std::vector<int> exceeded;
    };

    static Block heading(const QString & text);
    static Block paragraph(const QString & text);

    std::vector<Block> blocks() const;
    void addSpecifications(std::vector<Block> & out) const;
    void addStability(std::vector<Block> & out) const;
    void addSearch(std::vector<Block> & out) const;
    void addCertificate(std::vector<Block> & out) const;
    QString html() const;

    static QString specificationsText(const SpecificationCheck & check);
    static QString excessText(double excessDb);
    static QString marginText(double excessDb);
    static QString specificationTitle(SpecificationType type);
    static QString familyText(const FamilyStability & family);
    static QString nominalText(const NominalStability & nominal);
    static const SpecificationExcess * tightest(const SpecificationCheck & check);

    const LoopShapingResult * m_result = nullptr;
    std::vector<Block> m_blocks;
    QLabel * m_verdict = nullptr;
    QTextBrowser * m_text = nullptr;
    QPushButton * m_copy = nullptr;
};

}

#endif
