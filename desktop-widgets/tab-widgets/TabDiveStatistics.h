// SPDX-License-Identifier: GPL-2.0
#ifndef TAB_DIVE_STATISTICS_H
#define TAB_DIVE_STATISTICS_H

#include "TabBase.h"
#include "core/subsurface-qt/DiveListNotifier.h"

namespace Ui {
	class TabDiveStatistics;
};

class TabDiveStatistics : public TabBase {
	Q_OBJECT
public:
	TabDiveStatistics(QWidget *parent = 0);
	~TabDiveStatistics();
	void updateData() override;
	void clear() override;

private slots:
	void divesChanged(const QVector<dive *> &dives, DiveField field);

private:
	Ui::TabDiveStatistics *ui;
};

#endif
