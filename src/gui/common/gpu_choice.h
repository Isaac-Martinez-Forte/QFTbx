/**
 * @file
 * @brief The choice of the GPU in a form, offered only where the program
 * can use it.
 *
 * offerGpu() shows the control, and its companion if it has one, only in a
 * build with GPU support, and leaves it disabled when the card cannot be
 * used, with the reason as its tooltip. processorChosen() reads it back:
 * the GPU only when the control is shown, enabled and checked.
 */

#ifndef QFTBX_GUI_GPU_CHOICE_H
#define QFTBX_GUI_GPU_CHOICE_H

#include <QAbstractButton>
#include <QWidget>

#include "src/core/pipeline/processor.h"

namespace qftbx {

void offerGpu(QAbstractButton & gpu, QWidget * companion = nullptr);

Processor processorChosen(const QAbstractButton & gpu);

}

#endif
