/**
 * @file
 * @brief The choice of the GPU in a form, offered only where the program
 * can use it.
 *
 * A step chooses with a pair of buttons, GPU and CPU. offerGpu() shows them
 * only in a build with GPU support, and leaves the GPU disabled when the
 * card cannot be used, with the reason as its tooltip. chooseProcessor()
 * checks the one asked for, the CPU when the GPU cannot be had, and
 * processorChosen() reads the choice back: the GPU only when its button is
 * shown, enabled and checked.
 */

#ifndef QFTBX_GUI_GPU_CHOICE_H
#define QFTBX_GUI_GPU_CHOICE_H

#include <QAbstractButton>
#include <QWidget>

#include "src/core/pipeline/processor.h"

namespace qftbx {

void offerGpu(QAbstractButton & gpu, QWidget * companion = nullptr);

void chooseProcessor(QAbstractButton & gpu, QAbstractButton & cpu, Processor processor);

Processor processorChosen(const QAbstractButton & gpu);

}

#endif
