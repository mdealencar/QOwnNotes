#include <QDebug>
#include <QKeyEvent>
#include <entities/notefolder.h>
#include <entities/note.h>
#include <QTreeWidgetItem>
#include <QGraphicsPixmapItem>
#include <QtWidgets/QMessageBox>
#include <utils/gui.h>
#include "orphanedimagesdialog.h"
#include "ui_orphanedimagesdialog.h"

OrphanedImagesDialog::OrphanedImagesDialog(QWidget *parent) :
        MasterDialog(parent),
    ui(new Ui::OrphanedImagesDialog) {
    ui->setupUi(this);
    ui->fileTreeWidget->installEventFilter(this);

    QDir mediaDir(NoteFolder::currentMediaPath());

    if (!mediaDir.exists()) {
        ui->progressBar->setValue(ui->progressBar->maximum());
        return;
    }

    QStringList orphanedFiles = mediaDir.entryList(
            QStringList("*"), QDir::Files, QDir::Time);
    orphanedFiles.removeDuplicates();

    QList<Note> noteList = Note::fetchAll();
    int noteListCount = noteList.count();

    ui->progressBar->setMaximum(noteListCount);
    ui->progressBar->show();

    Q_FOREACH(Note note, noteList) {
            QString text = note.getNoteText();

            // match image links like ![media-qV920](file://media/608766373.gif)
            QRegularExpression re(
                    "!\\[.*?\\]\\(file:\\/\\/media/(.+?)\\)");
            QRegularExpressionMatchIterator i = re.globalMatch(text);

            // remove all found images from the orphaned files list
            while (i.hasNext()) {
                QRegularExpressionMatch match = i.next();
                QString fileName = match.captured(1);
                orphanedFiles.removeAll(fileName);
            }

            ui->progressBar->setValue(ui->progressBar->value() + 1);
        }

    ui->progressBar->hide();

    Q_FOREACH(QString fileName, orphanedFiles) {
            QTreeWidgetItem *item = new QTreeWidgetItem();
            item->setText(0, fileName);
            item->setData(0, Qt::UserRole, fileName);

            QString filePath = getFilePath(item);
            QFileInfo info(filePath);
            item->setToolTip(0, tr("Last modified at %1").arg(
                    info.lastModified().toString()));

            ui->fileTreeWidget->addTopLevelItem(item);
        }
}

OrphanedImagesDialog::~OrphanedImagesDialog() {
    delete ui;
}

/**
 * Shows the currently selected image
 * 
 * @param current
 * @param previous
 */
void OrphanedImagesDialog::on_fileTreeWidget_currentItemChanged(
        QTreeWidgetItem *current, QTreeWidgetItem *previous) {
    Q_UNUSED(previous);

    QGraphicsScene *scene = new QGraphicsScene();
    QString filePath = getFilePath(current);

    if (!filePath.isEmpty()) {
        scene->addPixmap(QPixmap(filePath));
    }

    ui->graphicsView->setScene(scene);
}

/**
 * Gets the file path of a tree widget item
 *
 * @param item
 * @return
 */
QString OrphanedImagesDialog::getFilePath(QTreeWidgetItem *item) {
    if (item == Q_NULLPTR) {
        return "";
    }

    QString fileName = NoteFolder::currentMediaPath() + QDir::separator() +
                       item->data(0, Qt::UserRole).toString();
    return fileName;
}

/**
 * Deletes selected images
 */
void OrphanedImagesDialog::on_deleteButton_clicked() {
    int selectedItemsCount = ui->fileTreeWidget->selectedItems().count();

    if (selectedItemsCount == 0) {
        return;
    }

    if (Utils::Gui::question(
            this,
            tr("Delete selected files"),
            tr("Delete <strong>%n</strong> selected files(s)?",
               "", selectedItemsCount),
            "delete-files") != QMessageBox::Yes) {
        return;
    }

    // delete all selected files
    Q_FOREACH(QTreeWidgetItem *item, ui->fileTreeWidget->selectedItems()) {
            QString filePath = getFilePath(item);
            bool removed = QFile::remove(filePath);

            if (removed) {
                delete item;
            }
    }
}

/**
 * Event filters
 *
 * @param obj
 * @param event
 * @return
 */
bool OrphanedImagesDialog::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        if (obj == ui->fileTreeWidget) {
            // delete the currently selected images
            if ((keyEvent->key() == Qt::Key_Delete) ||
                       (keyEvent->key() == Qt::Key_Backspace)) {
                on_deleteButton_clicked();
                return true;
            }
            return false;
        }
    }

    return MasterDialog::eventFilter(obj, event);
}
