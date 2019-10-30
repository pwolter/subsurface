// SPDX-License-Identifier: GPL-2.0
#include <QFileDialog>
#include <QShortcut>
#include <QSettings>
#include <QtConcurrent>
#include <string.h> // Allows string comparisons and substitutions in TeX export

#include "ui_divelogexportdialog.h"
#include "core/divelogexportlogic.h"
#include "core/worldmap-save.h"
#include "core/save-html.h"
#include "core/settings/qPrefDisplay.h"
#include "core/save-profiledata.h"
#include "core/divesite.h"
#include "core/errorhelper.h"
#include "core/file.h"
#include "core/tag.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/divelogexportdialog.h"
#include "desktop-widgets/diveshareexportdialog.h"
#include "desktop-widgets/subsurfacewebservices.h"
#include "profile-widget/profilewidget2.h"

// Retrieves the current unit settings defined in the Subsurface preferences.
#define GET_UNIT(name, field, f, t)           \
	v = settings.value(QString(name));        \
	if (v.isValid())                          \
		field = (v.toInt() == 0) ? (t) : (f); \
	else                                      \
		field = default_prefs.units.field

DiveLogExportDialog::DiveLogExportDialog(QWidget *parent) : QDialog(parent),
	ui(new Ui::DiveLogExportDialog)
{
	ui->setupUi(this);
	showExplanation();
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), MainWindow::instance(), SLOT(close()));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));

	/* the names are not the actual values exported to the json files,The font-family property should hold several
	font names as a "fallback" system, to ensure maximum compatibility between browsers/operating systems */
	ui->fontSelection->addItem("Arial", "Arial, Helvetica, sans-serif");
	ui->fontSelection->addItem("Impact", "Impact, Charcoal, sans-serif");
	ui->fontSelection->addItem("Georgia", "Georgia, serif");
	ui->fontSelection->addItem("Courier", "Courier, monospace");
	ui->fontSelection->addItem("Verdana", "Verdana, Geneva, sans-serif");

	QSettings settings;
	settings.beginGroup("HTML");
	if (settings.contains("fontSelection")) {
		ui->fontSelection->setCurrentIndex(settings.value("fontSelection").toInt());
	}
	if (settings.contains("fontSizeSelection")) {
		ui->fontSizeSelection->setCurrentIndex(settings.value("fontSizeSelection").toInt());
	}
	if (settings.contains("themeSelection")) {
		ui->themeSelection->setCurrentIndex(settings.value("themeSelection").toInt());
	}
	if (settings.contains("subsurfaceNumbers")) {
		ui->exportSubsurfaceNumber->setChecked(settings.value("subsurfaceNumbers").toBool());
	}
	if (settings.contains("yearlyStatistics")) {
		ui->exportStatistics->setChecked(settings.value("yearlyStatistics").toBool());
	}
	if (settings.contains("listOnly")) {
		ui->exportListOnly->setChecked(settings.value("listOnly").toBool());
	}
	if (settings.contains("exportPhotos")) {
		ui->exportPhotos->setChecked(settings.value("exportPhotos").toBool());
	}
	settings.endGroup();
}

DiveLogExportDialog::~DiveLogExportDialog()
{
	delete ui;
}

void DiveLogExportDialog::showExplanation()
{
	if (ui->exportUDDF->isChecked()) {
		ui->description->setText(tr("Generic format that is used for data exchange between a variety of diving related programs."));
	} else if (ui->exportCSV->isChecked()) {
		ui->description->setText(tr("Comma separated values describing the dive profile."));
	} else if (ui->exportCSVDetails->isChecked()) {
		ui->description->setText(tr("Comma separated values of the dive information. This includes most of the dive details but no profile information."));
	} else if (ui->exportDivelogs->isChecked()) {
		ui->description->setText(tr("Send the dive data to divelogs.de website."));
	} else if (ui->exportDiveshare->isChecked()) {
		ui->description->setText(tr("Send the dive data to dive-share.appspot.com website."));
	} else if (ui->exportWorldMap->isChecked()) {
		ui->description->setText(tr("HTML export of the dive locations, visualized on a world map."));
	} else if (ui->exportSubsurfaceXML->isChecked()) {
		ui->description->setText(tr("Subsurface native XML format."));
	} else if (ui->exportSubsurfaceSitesXML->isChecked()) {
		ui->description->setText(tr("Subsurface dive sites native XML format."));
	} else if (ui->exportImageDepths->isChecked()) {
		ui->description->setText(tr("Write depths of images to file."));
	} else if (ui->exportTeX->isChecked()) {
		ui->description->setText(tr("Write dive as TeX macros to file."));
	} else if (ui->exportLaTeX->isChecked()) {
		ui->description->setText(tr("Write dive as LaTeX macros to file."));
	} else if (ui->exportProfile->isChecked()) {
		ui->description->setText(tr("Write the profile image as PNG file."));
	} else if (ui->exportProfileData->isChecked()) {
		ui->description->setText(tr("Write profile data to a CSV file."));
	}
}

void DiveLogExportDialog::exportHtmlInit(const QString &filename)
{
	struct htmlExportSetting hes;
	hes.themeFile = (ui->themeSelection->currentText() == tr("Light")) ? "light.css" : "sand.css";
	hes.exportPhotos = ui->exportPhotos->isChecked();
	hes.selectedOnly = ui->exportSelectedDives->isChecked();
	hes.listOnly = ui->exportListOnly->isChecked();
	hes.fontFamily = ui->fontSelection->itemData(ui->fontSelection->currentIndex()).toString();
	hes.fontSize = ui->fontSizeSelection->currentText();
	hes.themeSelection = ui->themeSelection->currentIndex();
	hes.subsurfaceNumbers = ui->exportSubsurfaceNumber->isChecked();
	hes.yearlyStatistics = ui->exportStatistics->isChecked();

	exportHtmlInitLogic(filename, hes);
}

void DiveLogExportDialog::on_exportGroup_buttonClicked(QAbstractButton*)
{
	showExplanation();
}

static std::vector<const dive_site *> getDiveSitesToExport(bool selectedOnly)
{
	std::vector<const dive_site *> res;

	if (selectedOnly && MultiFilterSortModel::instance()->diveSiteMode()) {
		// Special case in dive site mode: export all selected dive sites,
		// not the dive sites of selected dives.
		QVector<dive_site *> sites = MultiFilterSortModel::instance()->filteredDiveSites();
		res.reserve(sites.size());
		for (const dive_site *ds: sites)
			res.push_back(ds);
		return res;
	}

	res.reserve(dive_site_table.nr);
	for (int i = 0; i < dive_site_table.nr; i++) {
		struct dive_site *ds = get_dive_site(i, &dive_site_table);
		if (dive_site_is_empty(ds))
			continue;
		if (selectedOnly && !is_dive_site_selected(ds))
			continue;
		res.push_back(ds);
	}
	return res;
}

void DiveLogExportDialog::on_buttonBox_accepted()
{
	QString filename;
	QString stylesheet;
	QString lastDir = QDir::homePath();

	if (QDir(qPrefDisplay::lastDir()).exists())
		lastDir = qPrefDisplay::lastDir();

	switch (ui->tabWidget->currentIndex()) {
	case 0:
		if (ui->exportUDDF->isChecked()) {
			stylesheet = "uddf-export.xslt";
			filename = QFileDialog::getSaveFileName(this, tr("Export UDDF file as"), lastDir,
								tr("UDDF files") + " (*.uddf)");
		} else if (ui->exportCSV->isChecked()) {
			stylesheet = "xml2csv.xslt";
			filename = QFileDialog::getSaveFileName(this, tr("Export CSV file as"), lastDir,
								tr("CSV files") + " (*.csv)");
		} else if (ui->exportCSVDetails->isChecked()) {
			stylesheet = "xml2manualcsv.xslt";
			filename = QFileDialog::getSaveFileName(this, tr("Export CSV file as"), lastDir,
								tr("CSV files") + " (*.csv)");
		} else if (ui->exportDivelogs->isChecked()) {
			DivelogsDeWebServices::instance()->prepareDivesForUpload(ui->exportSelected->isChecked());
		} else if (ui->exportDiveshare->isChecked()) {
			DiveShareExportDialog::instance()->prepareDivesForUpload(ui->exportSelected->isChecked());
		} else if (ui->exportWorldMap->isChecked()) {
			filename = QFileDialog::getSaveFileName(this, tr("Export world map"), lastDir,
								tr("HTML files") + " (*.html)");
			if (!filename.isNull() && !filename.isEmpty())
				export_worldmap_HTML(qPrintable(filename), ui->exportSelected->isChecked());
		} else if (ui->exportSubsurfaceXML->isChecked()) {
			filename = QFileDialog::getSaveFileName(this, tr("Export Subsurface XML"), lastDir,
								tr("Subsurface files") + " (*.ssrf *.xml)");
			if (!filename.isNull() && !filename.isEmpty()) {
				if (!filename.contains('.'))
					filename.append(".ssrf");
				QByteArray bt = QFile::encodeName(filename);
				save_dives_logic(bt.data(), ui->exportSelected->isChecked(), ui->anonymize->isChecked());
			}
		} else if (ui->exportSubsurfaceSitesXML->isChecked()) {
			filename = QFileDialog::getSaveFileName(this, tr("Export Subsurface dive sites XML"), lastDir,
								tr("Subsurface files") + " (*.xml)");
			if (!filename.isNull() && !filename.isEmpty()) {
				if (!filename.contains('.'))
					filename.append(".xml");
				QByteArray bt = QFile::encodeName(filename);
				std::vector<const dive_site *> sites = getDiveSitesToExport(ui->exportSelected->isChecked());
				save_dive_sites_logic(bt.data(), &sites[0], (int)sites.size(), ui->anonymize->isChecked());
			}
		} else if (ui->exportImageDepths->isChecked()) {
			filename = QFileDialog::getSaveFileName(this, tr("Save image depths"), lastDir);
			if (!filename.isNull() && !filename.isEmpty())
				export_depths(qPrintable(filename), ui->exportSelected->isChecked());
		} else if (ui->exportTeX->isChecked() || ui->exportLaTeX->isChecked()) {
			filename = QFileDialog::getSaveFileName(this, tr("Export to TeX file"), lastDir, tr("TeX files") + " (*.tex)");
			if (!filename.isNull() && !filename.isEmpty())
				export_TeX(qPrintable(filename), ui->exportSelected->isChecked(), ui->exportTeX->isChecked());
		} else if (ui->exportProfile->isChecked()) {
			filename = QFileDialog::getSaveFileName(this, tr("Save profile image"), lastDir);
			if (!filename.isNull() && !filename.isEmpty())
				exportProfile(qPrintable(filename), ui->exportSelected->isChecked());
		} else if (ui->exportProfileData->isChecked()) {
			filename = QFileDialog::getSaveFileName(this, tr("Save profile data"), lastDir);
			if (!filename.isNull() && !filename.isEmpty())
				save_profiledata(qPrintable(filename), ui->exportSelected->isChecked());
		}
		break;
	case 1:
		filename = QFileDialog::getSaveFileName(this, tr("Export HTML files as"), lastDir,
							tr("HTML files") + " (*.html)");
		if (!filename.isNull() && !filename.isEmpty())
			exportHtmlInit(filename);
		break;
	}

	if (!filename.isNull() && !filename.isEmpty()) {
		// remember the last export path
		QFileInfo fileInfo(filename);
		qPrefDisplay::set_lastDir(fileInfo.dir().path());
		// the non XSLT exports are called directly above, the XSLT based ons are called here
		if (!stylesheet.isEmpty()) {
			future = QtConcurrent::run(export_dives_xslt, filename.toUtf8(), ui->exportSelected->isChecked(), ui->CSVUnits_2->currentIndex(), stylesheet.toUtf8(), ui->anonymize->isChecked());
			MainWindow::instance()->getNotificationWidget()->showNotification(tr("Please wait, exporting..."), KMessageWidget::Information);
			MainWindow::instance()->getNotificationWidget()->setFuture(future);
		}
	}
}

void DiveLogExportDialog::export_depths(const char *filename, const bool selected_only)
{
	FILE *f;
	struct dive *dive;
	depth_t depth;
	int i;
	const char *unit = NULL;

	struct membuffer buf = {};

	for_each_dive (i, dive) {
		if (selected_only && !dive->selected)
			continue;

		FOR_EACH_PICTURE (dive) {
			int n = dive->dc.samples;
			struct sample *s = dive->dc.sample;
			depth.mm = 0;
			while (--n >= 0 && (int32_t)s->time.seconds <= picture->offset.seconds) {
				depth.mm = s->depth.mm;
				s++;
			}
			put_format(&buf, "%s\t%.1f", picture->filename, get_depth_units(depth.mm, NULL, &unit));
			put_format(&buf, "%s\n", unit);
		}
	}

	f = subsurface_fopen(filename, "w+");
	if (!f) {
		report_error(qPrintable(tr("Can't open file %s")), filename);
	} else {
		flush_buffer(&buf, f); /*check for writing errors? */
		fclose(f);
	}
	free_buffer(&buf);
}

void DiveLogExportDialog::exportProfile(QString filename, const bool selected_only)
{
	struct dive *dive;
	int i;
	int count = 0;
	if (!filename.endsWith(".png", Qt::CaseInsensitive))
		filename = filename.append(".png");
	QFileInfo fi(filename);

	for_each_dive (i, dive) {
		if (selected_only && !dive->selected)
			continue;
		if (count)
			saveProfile(dive, fi.path() + QDir::separator() + fi.completeBaseName().append(QString("-%1.").arg(count)) + fi.suffix());
		else
			saveProfile(dive, filename);
		++count;
	}
}

void DiveLogExportDialog::saveProfile(const struct dive *dive, const QString filename)
{
	ProfileWidget2 *profile = MainWindow::instance()->graphics;
	profile->plotDive(dive, true, false, true);
	profile->setToolTipVisibile(false);
	QPixmap pix = profile->grab();
	profile->setToolTipVisibile(true);
	pix.save(filename);
}

void DiveLogExportDialog::export_TeX(const char *filename, const bool selected_only, bool plain)
{
	FILE *f;
	QDir texdir = QFileInfo(filename).dir();
	struct dive *dive;
	const struct units *units = get_units();
	const char *unit;
	const char *ssrf;
	int i;
	bool need_pagebreak = false;

	struct membuffer buf = {};

	if (plain) {
		ssrf = "";
		put_format(&buf, "\\input subsurfacetemplate\n");
		put_format(&buf, "%% This is a plain TeX file. Compile with pdftex, not pdflatex!\n");
		put_format(&buf, "%% You will also need a subsurfacetemplate.tex in the current directory.\n");
	} else {
		ssrf = "ssrf";
		put_format(&buf, "\\input subsurfacelatextemplate\n");
		put_format(&buf, "%% This is a plain LaTeX file. Compile with pdflatex, not pdftex!\n");
		put_format(&buf, "%% You will also need a subsurfacelatextemplate.tex in the current directory.\n");
	}
	put_format(&buf, "%% You can download an example from http://www.atdotde.de/~robert/subsurfacetemplate\n%%\n");
	put_format(&buf, "%%\n");
	put_format(&buf, "%% Notes: TeX/LaTex will not render the degree symbol correctly by default. In LaTeX, you may\n");
	put_format(&buf, "%% add the following line to the end of the preamble of your template to ensure correct output:\n");
	put_format(&buf, "%% \\usepackage[utf8]{inputenc}\n");
	put_format(&buf, "%% \\usepackage{gensymb}\n");
	put_format(&buf, "%% \\DeclareUnicodeCharacter{00B0}{\\degree}\n"); //replaces ° with \degree
	put_format(&buf, "%%\n");

	/* Define text fields with the units used for export.  These values are set in the Subsurface Preferences
	 * and the text fields created here are included in the data fields below.
	 */
	put_format(&buf, "\n%% These fields contain the units used in other fields below. They may be\n");
	put_format(&buf, "%% referenced as needed in TeX templates.\n");
	put_format(&buf, "%% \n");
	put_format(&buf, "%% By default, Subsurface exports units of volume as \"ℓ\" and \"cuft\", which do\n");
	put_format(&buf, "%% not render well in TeX/LaTeX.  The code below substitutes \"L\" and \"ft$^{3}$\",\n");
	put_format(&buf, "%% respectively.  If you wish to display the original values, you may edit this\n");
	put_format(&buf, "%% list and all calls to those units will be updated in your document.\n");

	put_format(&buf, "\\def\\%sdepthunit{\\%sunit%s}", ssrf, ssrf, units->length == units::METERS ? "meter" : "ft");
	put_format(&buf, "\\def\\%sweightunit{\\%sunit%s}",ssrf, ssrf, units->weight == units::KG ? "kg" : "lb");
	put_format(&buf, "\\def\\%spressureunit{\\%sunit%s}", ssrf, ssrf, units->pressure == units::BAR ? "bar" : "psi");
	put_format(&buf, "\\def\\%stemperatureunit{\\%sunit%s}", ssrf, ssrf, units->temperature == units::CELSIUS ? "centigrade" : "fahrenheit");
	put_format(&buf, "\\def\\%svolumeunit{\\%sunit%s}", ssrf, ssrf, units->volume == units::LITER ? "liter" : "cuft");
	put_format(&buf, "\\def\\%sverticalspeedunit{\\%sunit%s}", ssrf, ssrf, units->length == units::METERS ? "meterpermin" : "ftpermin");

	put_format(&buf, "\n%%%%%%%%%% Begin Dive Data: %%%%%%%%%%\n");

	for_each_dive (i, dive) {
		if (selected_only && !dive->selected)
			continue;

		saveProfile(dive, texdir.filePath(QString("profile%1.png").arg(dive->number)));
		struct tm tm;
		utc_mkdate(dive->when, &tm);

		dive_site *site = dive->dive_site;
		QRegExp ct("countrytag: (\\w+)");
		QString country;
		if (site && ct.indexIn(site->notes) >= 0)
			country = ct.cap(1);
		else
			country = "";

		pressure_t delta_p = {.mbar = 0};

		QString star = "*";
		QString viz = star.repeated(dive->visibility);
		QString rating = star.repeated(dive->rating);

		int i;
		int qty_cyl;
		int qty_weight;
		double total_weight;

		if (need_pagebreak) {
			if (plain)
				put_format(&buf, "\\vfill\\eject\n");

			else
				put_format(&buf, "\\newpage\n");
		}
		need_pagebreak = true;
		put_format(&buf, "\n%% Time, Date, and location:\n");
		put_format(&buf, "\\def\\%sdate{%04u-%02u-%02u}\n", ssrf,
		      tm.tm_year, tm.tm_mon+1, tm.tm_mday);
		put_format(&buf, "\\def\\%shour{%02u}\n", ssrf, tm.tm_hour);
		put_format(&buf, "\\def\\%sminute{%02u}\n", ssrf, tm.tm_min);
		put_format(&buf, "\\def\\%snumber{%d}\n", ssrf, dive->number);
		put_format(&buf, "\\def\\%splace{%s}\n", ssrf, site ? site->name : "");
		put_format(&buf, "\\def\\%sspot{}\n", ssrf);
		put_format(&buf, "\\def\\%ssitename{%s}\n", ssrf, site ? site->name : "");
		site ? put_format(&buf, "\\def\\%sgpslat{%f}\n", ssrf, site->location.lat.udeg / 1000000.0) : put_format(&buf, "\\def\\%sgpslat{}\n", ssrf);
		site ? put_format(&buf, "\\def\\%sgpslon{%f}\n", ssrf, site->location.lon.udeg / 1000000.0) : put_format(&buf, "\\def\\gpslon{}\n");
		put_format(&buf, "\\def\\%scomputer{%s}\n", ssrf, dive->dc.model);
		put_format(&buf, "\\def\\%scountry{%s}\n", ssrf, qPrintable(country));
		put_format(&buf, "\\def\\%stime{%u:%02u}\n", ssrf, FRACTION(dive->duration.seconds, 60));

		put_format(&buf, "\n%% Dive Profile Details:\n");
		dive->maxtemp.mkelvin ? put_format(&buf, "\\def\\%smaxtemp{%.1f\\%stemperatureunit}\n", ssrf, get_temp_units(dive->maxtemp.mkelvin, &unit), ssrf) : put_format(&buf, "\\def\\%smaxtemp{}\n", ssrf);
		dive->mintemp.mkelvin ? put_format(&buf, "\\def\\%smintemp{%.1f\\%stemperatureunit}\n", ssrf, get_temp_units(dive->mintemp.mkelvin, &unit), ssrf) : put_format(&buf, "\\def\\%ssrfmintemp{}\n", ssrf);
		dive->watertemp.mkelvin ? put_format(&buf, "\\def\\%swatertemp{%.1f\\%stemperatureunit}\n", ssrf, get_temp_units(dive->watertemp.mkelvin, &unit), ssrf) : put_format(&buf, "\\def\\%swatertemp{}\n", ssrf);
		dive->airtemp.mkelvin ? put_format(&buf, "\\def\\%sairtemp{%.1f\\%stemperatureunit}\n", ssrf, get_temp_units(dive->airtemp.mkelvin, &unit), ssrf) : put_format(&buf, "\\def\\%sairtemp{}\n", ssrf);
		dive->maxdepth.mm ? put_format(&buf, "\\def\\%smaximumdepth{%.1f\\%sdepthunit}\n", ssrf, get_depth_units(dive->maxdepth.mm, NULL, &unit), ssrf) : put_format(&buf, "\\def\\%smaximumdepth{}\n", ssrf);
		dive->meandepth.mm ? put_format(&buf, "\\def\\%smeandepth{%.1f\\%sdepthunit}\n", ssrf, get_depth_units(dive->meandepth.mm, NULL, &unit), ssrf) : put_format(&buf, "\\def\\%smeandepth{}\n", ssrf);

		struct tag_entry *tag = dive->tag_list;
		QString tags;
		if (tag) {
			tags = tag->tag->name;
			while ((tag = tag->next))
				tags += QString(", ") + QString(tag->tag->name);
		}
		put_format(&buf, "\\def\\%stype{%s}\n", ssrf, qPrintable(tags));
		put_format(&buf, "\\def\\%sviz{%s}\n", ssrf, qPrintable(viz));
		put_format(&buf, "\\def\\%srating{%s}\n", ssrf, qPrintable(rating));
		put_format(&buf, "\\def\\%splot{\\includegraphics[width=9cm,height=4cm]{profile%d}}\n", ssrf, dive->number);
		put_format(&buf, "\\def\\%sprofilename{profile%d}\n", ssrf, dive->number);
		put_format(&buf, "\\def\\%scomment{%s}\n", ssrf, dive->notes ? dive->notes : "");
		put_format(&buf, "\\def\\%sbuddy{%s}\n", ssrf, dive->buddy ? dive->buddy : "");
		put_format(&buf, "\\def\\%sdivemaster{%s}\n", ssrf, dive->divemaster ? dive->divemaster : "");
		put_format(&buf, "\\def\\%ssuit{%s}\n", ssrf, dive->suit ? dive->suit : "");

		// Print cylinder data
		put_format(&buf, "\n%% Gas use information:\n");
		qty_cyl = 0;
		for (i = 0; i < MAX_CYLINDERS; i++){

			if (is_cylinder_used(dive, i) || (prefs.display_unused_tanks && dive->cylinder[i].type.description)){
				put_format(&buf, "\\def\\%scyl%cdescription{%s}\n", ssrf, 'a' + i, dive->cylinder[i].type.description);
				put_format(&buf, "\\def\\%scyl%cgasname{%s}\n", ssrf, 'a' + i, gasname(dive->cylinder[i].gasmix));
				put_format(&buf, "\\def\\%scyl%cmixO2{%.1f\\%%}\n", ssrf, 'a' + i, get_o2(dive->cylinder[i].gasmix)/10.0);
				put_format(&buf, "\\def\\%scyl%cmixHe{%.1f\\%%}\n", ssrf, 'a' + i, get_he(dive->cylinder[i].gasmix)/10.0);
				put_format(&buf, "\\def\\%scyl%cmixN2{%.1f\\%%}\n", ssrf, 'a' + i, (100.0 - (get_o2(dive->cylinder[i].gasmix)/10.0) - (get_he(dive->cylinder[i].gasmix)/10.0)));
				delta_p.mbar += dive->cylinder[i].start.mbar - dive->cylinder[i].end.mbar;
				put_format(&buf, "\\def\\%scyl%cstartpress{%.1f\\%spressureunit}\n", ssrf, 'a' + i, get_pressure_units(dive->cylinder[i].start.mbar, &unit)/1.0, ssrf);
				put_format(&buf, "\\def\\%scyl%cendpress{%.1f\\%spressureunit}\n", ssrf, 'a' + i, get_pressure_units(dive->cylinder[i].end.mbar, &unit)/1.0, ssrf);
				qty_cyl += 1;
			} else {
				put_format(&buf, "\\def\\%scyl%cdescription{}\n", ssrf, 'a' + i);
				put_format(&buf, "\\def\\%scyl%cgasname{}\n", ssrf, 'a' + i);
				put_format(&buf, "\\def\\%scyl%cmixO2{}\n", ssrf, 'a' + i);
				put_format(&buf, "\\def\\%scyl%cmixHe{}\n", ssrf, 'a' + i);
				put_format(&buf, "\\def\\%scyl%cmixN2{}\n", ssrf, 'a' + i);
				delta_p.mbar += dive->cylinder[i].start.mbar - dive->cylinder[i].end.mbar;
				put_format(&buf, "\\def\\%scyl%cstartpress{}\n", ssrf, 'a' + i);
				put_format(&buf, "\\def\\%scyl%cendpress{}\n", ssrf, 'a' + i);
				qty_cyl += 1;
			}
		}
		put_format(&buf, "\\def\\%sqtycyl{%d}\n", ssrf, qty_cyl);
		put_format(&buf, "\\def\\%sgasuse{%.1f\\%spressureunit}\n", ssrf, get_pressure_units(delta_p.mbar, &unit)/1.0, ssrf);
		put_format(&buf, "\\def\\%ssac{%.2f\\%svolumeunit/min}\n", ssrf, get_volume_units(dive->sac, NULL, &unit), ssrf);

		//Code block prints all weights listed in dive.
		put_format(&buf, "\n%% Weighting information:\n");
		qty_weight = 0;
		total_weight = 0;
		for (i = 0; i < dive->weightsystems.nr; i++) {
			weightsystem_t w = dive->weightsystems.weightsystems[i];
			put_format(&buf, "\\def\\%sweight%ctype{%s}\n", ssrf, 'a' + i, w.description);
			put_format(&buf, "\\def\\%sweight%camt{%.3f\\%sweightunit}\n", ssrf, 'a' + i, get_weight_units(w.weight.grams, NULL, &unit), ssrf);
			qty_weight += 1;
			total_weight += get_weight_units(w.weight.grams, NULL, &unit);
		}
		put_format(&buf, "\\def\\%sqtyweights{%d}\n", ssrf, qty_weight);
		put_format(&buf, "\\def\\%stotalweight{%.2f\\%sweightunit}\n", ssrf, total_weight, ssrf);
		unit = "";

		// Legacy fields
		put_format(&buf, "\\def\\%sspot{}\n", ssrf);
		put_format(&buf, "\\def\\%sentrance{}\n", ssrf);
		put_format(&buf, "\\def\\%splace{%s}\n", ssrf, site ? site->name : "");
		dive->maxdepth.mm ? put_format(&buf, "\\def\\%sdepth{%.1f\\%sdepthunit}\n", ssrf, get_depth_units(dive->maxdepth.mm, NULL, &unit), ssrf) : put_format(&buf, "\\def\\%sdepth{}\n", ssrf);

		put_format(&buf, "\\%spage\n", ssrf);

	}

	if (plain)
		put_format(&buf, "\\bye\n");
	else
		put_format(&buf, "\\end{document}\n");

	f = subsurface_fopen(filename, "w+");
	if (!f) {
		report_error(qPrintable(tr("Can't open file %s")), filename);
	} else {
		flush_buffer(&buf, f); /*check for writing errors? */
		fclose(f);
	}
	free_buffer(&buf);
}
