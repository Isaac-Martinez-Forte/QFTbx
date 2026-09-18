#include "src/gui/common/number_text.h"

namespace qftbx {

namespace {

//Four, because that is what a gain, a margin or a coefficient is read at.
//The project file is never written through this: it keeps every digit.
int g_shownDigits = 4;

} // namespace

int shownDigits()
{
    return g_shownDigits;
}

void setShownDigits(int digits)
{
    g_shownDigits = digits;
}

} // namespace qftbx
