/**
 * @file
 * @brief The global count of digits a form shows.
 *
 * Four by default, which is what a gain, a margin or a coefficient is read
 * at. The project file is never written through it.
 */

#include "src/gui/common/number_text.h"

namespace qftbx {

namespace {

int g_shownDigits = 4;

}

int shownDigits()
{
    return g_shownDigits;
}

void setShownDigits(int digits)
{
    g_shownDigits = digits;
}

}
