/*
Copyright (C) 2005-2007 Remon Sijrier 

This file is part of Traverso

Traverso is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.

*/

#include "ProjectManagerDialog.h"

#include "libtraversocore.h"
#include <QStringList>
#include <QInputDialog>
#include <QHeaderView>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <dialogs/project/NewSongDialog.h>
#include <Interface.h>

// Always put me below _all_ includes, this is needed
// in case we run with memory leak detection enabled!
#include "Debugger.h"

ProjectManagerDialog::ProjectManagerDialog( QWidget * parent )
	: QDialog(parent)
{
	setupUi(this);

	treeSongWidget->setColumnCount(3);
	treeSongWidget->header()->resizeSection(0, 160);
	treeSongWidget->header()->resizeSection(1, 55);
	treeSongWidget->header()->resizeSection(2, 70);
	QStringList stringList;
	stringList << "Sheet Name" << "Tracks" << "Length";
	treeSongWidget->setHeaderLabels(stringList);
	
	set_project(pm().get_project());
	
	undoButton->setIcon(QIcon(find_pixmap(":/undo-16")));
	redoButton->setIcon(QIcon(find_pixmap(":/redo-16")));
	
	buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

	connect(treeSongWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(songitem_clicked(QTreeWidgetItem*,int)));
	connect(&pm(), SIGNAL(projectLoaded(Project*)), this, SLOT(set_project(Project*)));
}

ProjectManagerDialog::~ProjectManagerDialog()
{}

void ProjectManagerDialog::set_project(Project* project)
{
	m_project = project;
	
	if (m_project) {
		connect(m_project, SIGNAL(songAdded(Song*)), this, SLOT(update_song_list()));
		connect(m_project, SIGNAL(songRemoved(Song*)), this, SLOT(update_song_list()));
		connect(m_project->get_history_stack(), SIGNAL(redoTextChanged ( const QString &)),
			this, SLOT(redo_text_changed(const QString&)));
		connect(m_project->get_history_stack(), SIGNAL(undoTextChanged ( const QString &)),
			this, SLOT(undo_text_changed(const QString&)));
		setWindowTitle("Manage Project - " + m_project->get_title());
		descriptionTextEdit->setText(m_project->get_description());
		lineEditTitle->setText(m_project->get_title());
		lineEditId->setText(m_project->get_discid());
		lineEditUPC->setText(m_project->get_upc_ean());
		lineEditPerformer->setText(m_project->get_performer());
		lineEditArranger->setText(m_project->get_arranger());
		lineEditSongwriter->setText(m_project->get_songwriter());
		lineEditMessage->setText(m_project->get_message());
		comboBoxGenre->setCurrentIndex(m_project->get_genre());
		redoButton->setText(m_project->get_history_stack()->redoText());
		undoButton->setText(m_project->get_history_stack()->undoText());
	} else {
		setWindowTitle("Manage Project - No Project loaded!");
		treeSongWidget->clear();
		descriptionTextEdit->clear();
		lineEditTitle->clear();
		lineEditId->clear();
		lineEditUPC->clear();
		lineEditPerformer->clear();
		lineEditArranger->clear();
		lineEditSongwriter->clear();
		lineEditMessage->clear();
		comboBoxGenre->setCurrentIndex(0);
	}
	
	update_song_list();
}


void ProjectManagerDialog::update_song_list( )
{
	if ( ! m_project) {
		printf("ProjectManagerDialog:: no project ?\n");
		return;
	}
	
	treeSongWidget->clear();
	foreach(Song* song, m_project->get_songs()) {

		QString songNr = QString::number(m_project->get_song_index(song->get_id()));
		QString songName = "Sheet " + songNr + " - " + song->get_title();
		QString numberOfTracks = QString::number(song->get_numtracks());
		QString songLength = frame_to_ms_2(song->get_last_frame(), song->get_rate());
		QString songStatus = song->is_changed()?"UnSaved":"Saved";
		QString songSpaceAllocated = "Unknown";

		QTreeWidgetItem* item = new QTreeWidgetItem(treeSongWidget);
		item->setTextAlignment(1, Qt::AlignHCenter);
		item->setTextAlignment(2, Qt::AlignHCenter);
		item->setText(0, songName);
		item->setText(1, numberOfTracks);
		item->setText(2, songLength);
		
		item->setData(0, Qt::UserRole, song->get_id());
	}
}

void ProjectManagerDialog::songitem_clicked( QTreeWidgetItem* item, int)
{
	if (!item) {
		return;
	}

	Song* song;

	qint64 id = item->data(0, Qt::UserRole).toLongLong();
	song = m_project->get_song(id);

	Q_ASSERT(song);
		
	selectedSongName->setText(song->get_title());
	
	m_project->set_current_song(song->get_id());
}

void ProjectManagerDialog::on_renameSongButton_clicked( )
{
	if( ! m_project) {
		return;
	}
	
	QTreeWidgetItem* item = treeSongWidget->currentItem();
	
	if ( ! item) {
		return;
	}
	
	qint64 id = item->data(0, Qt::UserRole).toLongLong();
	Song* song = m_project->get_song(id);
	
	Q_ASSERT(song);
	
	QString newtitle = selectedSongName->text();
	
	if (newtitle.isEmpty()) {
		info().information(tr("No new Sheet name was supplied!"));
		return;
	}
	
	song->set_title(newtitle);

	update_song_list();
}

void ProjectManagerDialog::on_deleteSongButton_clicked( )
{
	QTreeWidgetItem* item = treeSongWidget->currentItem();
	
	if ( ! item ) {
		return;
	}
	
	qint64 id = item->data(0, Qt::UserRole).toLongLong();
	
	Command::process_command(m_project->remove_song(m_project->get_song(id)));
}

void ProjectManagerDialog::on_createSongButton_clicked( )
{
	Interface::instance()->show_newsong_dialog();
}

void ProjectManagerDialog::redo_text_changed(const QString & text)
{
	redoButton->setText(text);
}

void ProjectManagerDialog::undo_text_changed(const QString & text)
{
	undoButton->setText(text);
}

void ProjectManagerDialog::on_undoButton_clicked()
{
	if (! m_project ) {
		return;
	}
	
	m_project->get_history_stack()->undo();
}

void ProjectManagerDialog::on_redoButton_clicked()
{
	if (! m_project ) {
		return;
	}
	
	m_project->get_history_stack()->redo();
}

void ProjectManagerDialog::on_songsExportButton_clicked()
{
	Interface::instance()->show_export_widget();
}

void ProjectManagerDialog::on_exportTemplateButton_clicked()
{
	bool ok;
	QString text = QInputDialog::getText(this, tr("Save Template"),
					     tr("Enter Template name"),
					     QLineEdit::Normal, "", &ok);
	if (! ok || text.isEmpty()) {
		return;
	}
	
	QString fileName = QDir::homePath() + "/.traverso/ProjectTemplates/";
	
	QDir dir;
	if (! dir.exists(fileName)) {
		if (! dir.mkdir(fileName)) {
			info().critical( tr("Unable to create directory %1!").arg(fileName));
			return;
		}
	}
	
	fileName.append(text + ".tpt");
	
	QDomDocument doc(text);
	
	if (QFile::exists(fileName)) {
		QMessageBox::StandardButton button = QMessageBox::question(this,
				tr("Traverso - Information"),
				tr("Template with name %1 allready exists!\n Do you want to overwrite it?").arg(fileName),
				QMessageBox::Yes | QMessageBox::No,
				QMessageBox::No);
				
		if (button == QMessageBox::No) {
			return;
		}
	}
	
	QFile file(fileName);

	if (file.open( QIODevice::WriteOnly ) ) {
		m_project->get_state(doc, true);
		QTextStream stream(&file);
		doc.save(stream, 4);
		file.close();
		info().information(tr("Saved Project Template: %1").arg(text));
	} else {
		info().critical( tr("Couldn't open file %1 for writing!").arg(fileName));
	}
	
}

void ProjectManagerDialog::accept()
{
	if ( ! m_project ) {
		hide();
		return;
	}
	
	m_project->set_description(descriptionTextEdit->toPlainText());
	m_project->set_title(lineEditTitle->text());
	m_project->set_discid(lineEditId->text());
	m_project->set_upc_ean(lineEditUPC->text());
	m_project->set_performer(lineEditPerformer->text());
	m_project->set_arranger(lineEditArranger->text());
	m_project->set_songwriter(lineEditSongwriter->text());
	m_project->set_message(lineEditMessage->text());
	m_project->set_genre(comboBoxGenre->currentIndex());
	
	hide();
}

void ProjectManagerDialog::reject()
{
	if ( ! m_project ) {
		hide();
		return;
	}
	
	descriptionTextEdit->setText(m_project->get_description());
	lineEditTitle->setText(m_project->get_title());
	lineEditId->setText(m_project->get_discid());
	lineEditUPC->setText(m_project->get_upc_ean());
	lineEditPerformer->setText(m_project->get_performer());
	lineEditArranger->setText(m_project->get_arranger());
	lineEditSongwriter->setText(m_project->get_songwriter());
	lineEditMessage->setText(m_project->get_message());
	comboBoxGenre->setCurrentIndex(m_project->get_genre());

	hide();
}


/* ---------------------------------------------------------------------------------------------- */
/* Here is some stuff about CD-Text. It is difficult to find in the web, so let's archive it here */
/* ---------------------------------------------------------------------------------------------- */

/* @(#)cdtext.h 1.1 02/02/23 Copyright 1999-2002 J. Schilling */
/*
 *      Generic CD-Text support definitions
 *
 *      Copyright (c) 1999-2002 J. Schilling
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.

...

#define tc_title        textcodes[0x00]
#define tc_performer    textcodes[0x01]
#define tc_songwriter   textcodes[0x02]
#define tc_composer     textcodes[0x03]
#define tc_arranger     textcodes[0x04]
#define tc_message      textcodes[0x05]
#define tc_diskid       textcodes[0x06]
#define tc_genre        textcodes[0x07]
#define tc_toc          textcodes[0x08]
#define tc_toc2         textcodes[0x09]

#define tc_closed_info  textcodes[0x0d]
#define tc_isrc         textcodes[0x0e]

/*
 *      binaere Felder sind
 *      Disc ID                 (Wirklich ???)
 *      Genre ID
 *      TOC
 *      Second TOC
 *      Size information
 */

/*
 * Genre codes from Enhanced CD Specification page 21
 */

// #define GENRE_UNUSED            0       /* not used                          */
// #define GENRE_UNDEFINED         1       /* not defined                       */
// #define GENRE_ADULT_CONTEMP     2       /* Adult Contemporary                */
// #define GENRE_ALT_ROCK          3       /* Alternative Rock                  */
// #define GENRE_CHILDRENS         4       /* Childrens Music                   */
// #define GENRE_CLASSIC           5       /* Classical                         */
// #define GENRE_CHRIST_CONTEMP    6       /* Contemporary Christian            */
// #define GENRE_COUNTRY           7       /* Country                           */
// #define GENRE_DANCE             8       /* Dance                             */
// #define GENRE_EASY_LISTENING    9       /* Easy Listening                    */
// #define GENRE_EROTIC            10      /* Erotic                            */
// #define GENRE_FOLK              11      /* Folk                              */
// #define GENRE_GOSPEL            12      /* Gospel                            */
// #define GENRE_HIPHOP            13      /* Hip Hop                           */
// #define GENRE_JAZZ              14      /* Jazz                              */
// #define GENRE_LATIN             15      /* Latin                             */
// #define GENRE_MUSICAL           16      /* Musical                           */
// #define GENRE_NEWAGE            17      /* New Age                           */
// #define GENRE_OPERA             18      /* Opera                             */
// #define GENRE_OPERETTA          19      /* Operetta                          */
// #define GENRE_POP               20      /* Pop Music                         */
// #define GENRE_RAP               21      /* RAP                               */
// #define GENRE_REGGAE            22      /* Reggae                            */
// #define GENRE_ROCK              23      /* Rock Music                        */
// #define GENRE_RYTHMANDBLUES     24      /* Rhythm & Blues                    */
// #define GENRE_SOUNDEFFECTS      25      /* Sound Effects                     */
// #define GENRE_SPOKEN_WORD       26      /* Spoken Word                       */
// #define GENRE_WORLD_MUSIC       28      /* World Music                       */
// #define GENRE_RESERVED          29      /* Reserved is 29..32767             */
// #define GENRE_RIAA              32768   /* Registration by RIAA 32768..65535 */

/*
 * Character codings used in CD-Text data.
 * Korean and Mandarin Chinese to be defined in sept 1996
 */
// #define CC_8859_1       0x00            /* ISO 8859-1                   */
// #define CC_ASCII        0x01            /* ISO 646, ASCII (7 bit)       */
// #define CC_RESERVED_02  0x02            /* Reserved codes 0x02..0x7f    */
// #define CC_KANJI        0x80            /* Music Shift-JIS Kanji        */
// #define CC_KOREAN       0x81            /* Korean                       */
// #define CC_CHINESE      0x82            /* Mandarin Chinese             */
// #define CC_RESERVED_83  0x83            /* Reserved codes 0x83..0xFF    */


/*
 * Language codes (currently guessed)
 */
// #define LANG_ENGLISH    9

//eof

