#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include "nounoursengine.h"
#include "ui/mainwindow.h"
#include "mpvhandler.h"
#include "ui/keydialog.h"

#include <QFileDialog>
#include <QMessageBox>

PreferencesDialog::PreferencesDialog(NounoursEngine *nounours, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog),
    nounours(nounours),
    screenshotDir("")
{
    ui->setupUi(this);

    ui->infoWidget->sortByColumn(0, Qt::AscendingOrder);
    sortLock = new SortLock(ui->infoWidget);

    PopulateLangs();

    QString ontop = nounours->window->getOnTop();
    if(ontop == "never")
        ui->neverRadioButton->setChecked(true);
    else if(ontop == "playing")
        ui->playingRadioButton->setChecked(true);
    else if(ontop == "always")
        ui->alwaysRadioButton->setChecked(true);
    ui->resumeCheckBox->setChecked(nounours->window->getResume());
    ui->groupBox_2->setChecked(nounours->sysTrayIcon->isVisible());
    ui->hidePopupCheckBox->setChecked(nounours->window->getHidePopup());
    ui->gestureCheckBox->setChecked(nounours->window->getGestures());
    ui->leftClickPlayPauseCheckBox->setChecked(nounours->window->getLeftClickPlayPause());
    ui->langComboBox->setCurrentText(nounours->window->getLang());
    int autofit = nounours->window->getAutoFit();
    ui->autoFitCheckBox->setChecked((bool)autofit);
    ui->comboBox->setCurrentText(QString::number(autofit)+"%");
    ui->gestureCheckBox->setChecked(nounours->window->getGestures());
    int maxRecent= nounours->window->getMaxRecent();
    ui->recentCheckBox->setChecked(maxRecent > 0);
    ui->recentSpinBox->setValue(maxRecent);
    ui->resumeCheckBox->setChecked(nounours->window->getResume());
    ui->formatComboBox->setCurrentText(nounours->mpv->getScreenshotFormat());
    screenshotDir = QDir::toNativeSeparators(nounours->mpv->getScreenshotDir());
    ui->templateLineEdit->setText(nounours->mpv->getScreenshotTemplate());
    ui->msgLvlComboBox->setCurrentText(nounours->mpv->getMsgLevel());

    // add shortcuts
    saved = nounours->input;
    PopulateShortcuts();

    connect(ui->autoFitCheckBox, &QCheckBox::clicked,
            [=](bool b)
            {
                ui->comboBox->setEnabled(b);
            });

    connect(ui->changeButton, &QPushButton::clicked,
            [=]
            {
                QString dir = QFileDialog::getExistingDirectory(this, tr("Choose screenshot directory"), screenshotDir);
                if(dir != QString())
                    screenshotDir = dir;
            });

    connect(ui->addKeyButton, &QPushButton::clicked,
            [=]
            {
                SelectKey(true);
            });

    connect(ui->editKeyButton, &QPushButton::clicked,
            [=]
            {
                int i = ui->infoWidget->currentRow();
                if(i == -1)
                    return;

                SelectKey(false,
                    {ui->infoWidget->item(i, 0)->text(),
                    {ui->infoWidget->item(i, 1)->text(),
                     ui->infoWidget->item(i, 2)->text()}});
            });

    connect(ui->resetKeyButton, &QPushButton::clicked,
            [=]
            {
                if(QMessageBox::question(this, tr("Reset All Key Bindings?"), tr("Are you sure you want to reset all shortcut keys to its original bindings?")) == QMessageBox::Yes)
                {
                    nounours->input = nounours->default_input;
                    while(numberOfShortcuts > 0)
                        RemoveRow(numberOfShortcuts-1);
                    PopulateShortcuts();
                }
            });

    connect(ui->removeKeyButton, &QPushButton::clicked,
            [=]
            {
                int row = ui->infoWidget->currentRow();
                if(row == -1)
                    return;

                nounours->input[ui->infoWidget->item(row, 0)->text()] = {QString(), QString()};
                RemoveRow(row);
            });

    connect(ui->infoWidget, &QTableWidget::currentCellChanged,
            [=](int r,int,int,int)
            {
                ui->editKeyButton->setEnabled(r != -1);
                ui->removeKeyButton->setEnabled(r != -1);
            });

    connect(ui->infoWidget, &QTableWidget::doubleClicked,
            [=](const QModelIndex &index)
            {
                int i = index.row();
                SelectKey(false,
                    {ui->infoWidget->item(i, 0)->text(),
                    {ui->infoWidget->item(i, 1)->text(),
                     ui->infoWidget->item(i, 2)->text()}});
            });

    connect(ui->recentCheckBox, SIGNAL(toggled(bool)),
            ui->recentSpinBox, SLOT(setEnabled(bool)));

    connect(ui->okButton, SIGNAL(clicked()),
            this, SLOT(accept()));

    connect(ui->cancelButton, SIGNAL(clicked()),
            this, SLOT(reject()));
}

PreferencesDialog::~PreferencesDialog()
{
    if(result() == QDialog::Accepted)
    {
        nounours->window->setResume(ui->resumeCheckBox->isChecked());
        if(ui->neverRadioButton->isChecked())
            nounours->window->setOnTop("never");
        else if(ui->playingRadioButton->isChecked())
            nounours->window->setOnTop("playing");
        else if(ui->alwaysRadioButton->isChecked())
            nounours->window->setOnTop("always");
        nounours->sysTrayIcon->setVisible(ui->groupBox_2->isChecked());
        nounours->window->setHidePopup(ui->hidePopupCheckBox->isChecked());
        nounours->window->setGestures(ui->gestureCheckBox->isChecked());
        nounours->window->setLeftClickPlayPause(ui->leftClickPlayPauseCheckBox->isChecked());
        nounours->window->setLang(ui->langComboBox->currentText());
        if(ui->autoFitCheckBox->isChecked())
            nounours->window->setAutoFit(ui->comboBox->currentText().left(ui->comboBox->currentText().length()-1).toInt());
        else
            nounours->window->setAutoFit(0);
        nounours->window->setMaxRecent(ui->recentCheckBox->isChecked() ? ui->recentSpinBox->value() : 0);
        nounours->window->setGestures(ui->gestureCheckBox->isChecked());
        nounours->window->setResume(ui->resumeCheckBox->isChecked());
        nounours->mpv->ScreenshotFormat(ui->formatComboBox->currentText());
        nounours->mpv->ScreenshotDirectory(screenshotDir);
        nounours->mpv->ScreenshotTemplate(ui->templateLineEdit->text());
        nounours->mpv->MsgLevel(ui->msgLvlComboBox->currentText());
        nounours->window->MapShortcuts();
    }
    else
        nounours->input = saved;
    delete sortLock;
    delete ui;
}

void PreferencesDialog::showPreferences(NounoursEngine *nounours, QWidget *parent)
{
    PreferencesDialog dialog(nounours, parent);
    dialog.exec();
}

void PreferencesDialog::PopulateLangs()
{
    // open the language directory
    QDir root(NOUNOURS_PLAYER_LANG_PATH);
    // get files in the directory with .qm extension
    QFileInfoList flist;
    flist = root.entryInfoList({"*.qm"}, QDir::Files);
    // add the languages to the combo box
    ui->langComboBox->addItem("auto");
    for(auto &i : flist)
    {
        QString lang = i.fileName().mid(i.fileName().indexOf("_") + 1); // nounours-player_....
        lang.chop(3); // -  .qm
        ui->langComboBox->addItem(lang);
    }
}

void PreferencesDialog::PopulateShortcuts()
{
    sortLock->lock();
    numberOfShortcuts = 0;
    for(auto iter = nounours->input.begin(); iter != nounours->input.end(); ++iter)
    {
        QPair<QString, QString> p = iter.value();
        if(p.first == QString() || p.second == QString())
            continue;
        AddRow(iter.key(), p.first, p.second);
    }
    sortLock->unlock();
}

void PreferencesDialog::AddRow(QString first, QString second, QString third)
{
    bool locked = sortLock->tryLock();
    ui->infoWidget->insertRow(numberOfShortcuts);
    ui->infoWidget->setItem(numberOfShortcuts, 0, new QTableWidgetItem(first));
    ui->infoWidget->setItem(numberOfShortcuts, 1, new QTableWidgetItem(second));
    ui->infoWidget->setItem(numberOfShortcuts, 2, new QTableWidgetItem(third));
    ++numberOfShortcuts;
    if(locked)
        sortLock->unlock();
}

void PreferencesDialog::ModifyRow(int row, QString first, QString second, QString third)
{
    bool locked = sortLock->tryLock();
    ui->infoWidget->item(row, 0)->setText(first);
    ui->infoWidget->item(row, 1)->setText(second);
    ui->infoWidget->item(row, 2)->setText(third);
    if(locked)
        sortLock->unlock();
}

void PreferencesDialog::RemoveRow(int row)
{
    bool locked = sortLock->tryLock();
    ui->infoWidget->removeCellWidget(row, 0);
    ui->infoWidget->removeCellWidget(row, 1);
    ui->infoWidget->removeCellWidget(row, 2);
    ui->infoWidget->removeRow(row);
    --numberOfShortcuts;
    if(locked)
        sortLock->unlock();
}

void PreferencesDialog::SelectKey(bool add, QPair<QString, QPair<QString, QString>> init)
{
    sortLock->lock();
    KeyDialog dialog(this);
    int status = 0;
    while(status != 2)
    {
        QPair<QString, QPair<QString, QString>> result = dialog.SelectKey(add, init);
        if(result == QPair<QString, QPair<QString, QString>>()) // cancel
            break;
        for(int i = 0; i < numberOfShortcuts; ++i)
        {
            if(!add && i == ui->infoWidget->currentRow()) // don't compare selected row if we're changing
                continue;
            if(ui->infoWidget->item(i, 0)->text() == result.first)
            {
                if(QMessageBox::question(this,
                       tr("Existing keybinding"),
                       tr("%0 is already being used. Would you like to change its function?").arg(
                           result.first)) == QMessageBox::Yes)
                {
                    nounours->input[ui->infoWidget->item(i, 0)->text()] = {QString(), QString()};
                    RemoveRow(i);
                    status = 0;
                }
                else
                {
                    init = result;
                    status = 1;
                }
                break;
            }
        }
        if(status == 0)
        {
            if(add) // add
                AddRow(result.first, result.second.first, result.second.second);
            else // change
            {
                if(result.first != init.first)
                    nounours->input[init.first] = {QString(), QString()};
                ModifyRow(ui->infoWidget->currentRow(), result.first, result.second.first, result.second.second);
            }
            nounours->input[result.first] = result.second;
            status = 2;
        }
        else
            status = 0;
    }
    sortLock->unlock();
}

PreferencesDialog::SortLock::SortLock(QTableWidget *parent):parent(parent) {}

void PreferencesDialog::SortLock::lock()
{
    parent->setSortingEnabled(false);
    QMutex::lock();
}

void PreferencesDialog::SortLock::unlock()
{
    QMutex::unlock();
    parent->setSortingEnabled(true);
}
