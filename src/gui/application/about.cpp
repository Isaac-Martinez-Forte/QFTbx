#include "src/gui/application/about.h"

#include <QCoreApplication>
#include <QMessageBox>
#include <QStringList>

namespace qftbx {

namespace {

const char * const kRepository = "https://github.com/Isaac-Martinez-Forte/QFTbx";
const char * const kDocumentation = "https://github.com/Isaac-Martinez-Forte/QFTbx/tree/Development/docs";

} // namespace

QString aboutText()
{
    //Each text translated where it stands, so lupdate files it under the
    //context "About" the translation is looked up in.
    QStringList paragraphs;
    paragraphs << QStringLiteral("<h2>QFTbx</h2>");
    paragraphs << QStringLiteral("<p>%1</p>").arg(QCoreApplication::translate("About",
        "A toolbox for the design and analysis of robust controllers with "
        "Quantitative Feedback Theory (QFT)."));
    paragraphs << QStringLiteral("<p>%1</p>").arg(QCoreApplication::translate("About",
        "It walks a design through the QFT pipeline: the plant with its "
        "parametric uncertainty and the design frequencies; the templates, "
        "the value sets of the plant at each frequency; the specifications "
        "of stability and performance; the boundaries they impose on the "
        "nominal loop in the Nichols chart; and the automatic loop shaping, "
        "which finds a controller of a given structure with the least "
        "high-frequency gain and certifies it with interval arithmetic."));
    paragraphs << QStringLiteral("<p><b>%1</b><br/>"
                                 "Isaac Mart&iacute;nez Forte &lt;<a href=\"mailto:isaac.martinez@upct.es\">isaac.martinez@upct.es</a>&gt;<br/>"
                                 "Joaqu&iacute;n Cervera L&oacute;pez &lt;<a href=\"mailto:jcervera@um.es\">jcervera@um.es</a>&gt;</p>")
                          .arg(QCoreApplication::translate("About", "Authors"));
    paragraphs << QStringLiteral("<p>%1</p>").arg(QCoreApplication::translate("About",
        "The algorithms come from the authors' work at the University of "
        "Murcia: the degree project of 2013, the master's thesis of 2014 and "
        "the doctoral thesis of 2022, with the article in the International "
        "Journal of Robust and Nonlinear Control (2021)."));
    paragraphs << QStringLiteral("<p>%1</p>").arg(QCoreApplication::translate("About",
        "QFTbx is software under development: some parts are incomplete or "
        "experimental, and its results should be checked before they are "
        "relied on."));
    paragraphs << QStringLiteral("<p>%1</p>").arg(QCoreApplication::translate("About",
        "Source code and documentation: <a href=\"%1\">%1</a>. The guides "
        "to building, configuring and measuring the toolbox, and the "
        "description of every algorithm with the work it comes from, are in "
        "<a href=\"%2\">docs</a>.").arg(QString::fromLatin1(kRepository), QString::fromLatin1(kDocumentation)));
    paragraphs << QStringLiteral("<p>%1</p>").arg(QCoreApplication::translate("About",
        "Free software under the GNU General Public License, version 3. "
        "Built with Qt, QCustomPlot, kv and pugixml."));
    return paragraphs.join(QString());
}

void showAbout(QWidget * parent)
{
    QMessageBox::about(parent, QCoreApplication::translate("About", "About QFTbx"), aboutText());
}

} // namespace qftbx
