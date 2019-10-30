// SPDX-License-Identifier: GPL-2.0
#ifndef USERMANUAL_H
#define USERMANUAL_H

#include <QWebView>
#include <QDialog>
#include "ui_searchbar.h"

class SearchBar : public QWidget{
	Q_OBJECT
public:
	SearchBar(QWidget *parent = 0);
signals:
	void searchTextChanged(const QString& s);
	void searchNext();
	void searchPrev();
protected:
	void setVisible(bool visible);
private slots:
	void enableButtons(const QString& s);
private:
	Ui::SearchBar ui;
};

class UserManual : public QDialog {
	Q_OBJECT

public:
	explicit UserManual(QWidget *parent = 0);

#ifdef Q_OS_MAC
protected:
	void showEvent(QShowEvent *e);
	void hideEvent(QHideEvent *e);
	QAction *closeAction;
	QAction *filterAction;
#endif

private
slots:
	void searchTextChanged(const QString& s);
	void searchNext();
	void searchPrev();
	void linkClickedSlot(const QUrl& url);
private:
	SearchBar *searchBar;
	QString mLastText;
	QWebView *userManual;
	void search(QString, QWebPage::FindFlags);
};
#endif // USERMANUAL_H
