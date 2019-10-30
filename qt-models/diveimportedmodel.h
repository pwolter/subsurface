#ifndef DIVEIMPORTEDMODEL_H
#define DIVEIMPORTEDMODEL_H

#include <QAbstractTableModel>
#include <vector>
#include "core/divesite.h"
#include "core/downloadfromdcthread.h"

class DiveImportedModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	enum roleTypes { DateTime = Qt::UserRole + 1, Duration, Depth, Selected};

	DiveImportedModel(QObject *parent = 0);
	int columnCount(const QModelIndex& index = QModelIndex()) const;
	int rowCount(const QModelIndex& index = QModelIndex()) const;
	QVariant data(const QModelIndex& index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	void setImportedDivesIndexes(int first, int last);
	Qt::ItemFlags flags(const QModelIndex &index) const;
	Q_INVOKABLE void clearTable();
	QHash<int, QByteArray> roleNames() const;
	void deleteDeselected();
	std::pair<struct dive_table, struct dive_site_table> consumeTables(); // Returns dives and sites and resets model.

	int numDives() const;
	Q_INVOKABLE void recordDives();
	Q_INVOKABLE void startDownload();

	DownloadThread thread;
public
slots:
	void changeSelected(QModelIndex clickedIndex);
	void selectRow(int row);
	void selectAll();
	void selectNone();

private
slots:
	void downloadThreadFinished();

signals:
	void downloadFinished();

private:
	std::vector<char> checkStates; // char instead of bool to avoid silly pessimization of std::vector.
	struct dive_table diveTable;
	struct dive_site_table sitesTable;
};

#endif
