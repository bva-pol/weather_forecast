#include "CityEditor.hpp"

#include <QMessageBox>

CityEditor::CityEditor (CityManager *manager, QWidget *parent)
    : QDialog(parent)
    , m_cityManager(manager)
{
    ui.setupUi(this);

    // Настройка таблицы
    ui.tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui.tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    // Загрузка городов в таблицу
    loadCitiesToTable();

    // Подключение сигналов
    connect(ui.tableWidget, &QTableWidget::itemSelectionChanged,
            this, &CityEditor::onTableSelectionChanged);
    connect(ui.btnAdd, &QPushButton::clicked, this, &CityEditor::onAddCity);
    connect(ui.btnUpdate, &QPushButton::clicked, this, &CityEditor::onUpdateCity);
    connect(ui.btnDelete, &QPushButton::clicked, this, &CityEditor::onDeleteCity);
    connect(ui.btnRestore, &QPushButton::clicked, this, &CityEditor::onRestoreDefaults);
    connect(ui.btnSave, &QPushButton::clicked, this, &CityEditor::onSave);
    connect(ui.btnCancel, &QPushButton::clicked, this, &CityEditor::onCancel);
}

CityEditor::~CityEditor ()
{

}

void CityEditor::loadCitiesToTable ()
{
    ui.tableWidget->setRowCount(0);

    QList<City> cities = m_cityManager->getAllCities();
    for (const City &city : cities) {
        int row = ui.tableWidget->rowCount();
        ui.tableWidget->insertRow(row);

        ui.tableWidget->setItem(row, 0, new QTableWidgetItem(city.name));
        ui.tableWidget->setItem(row, 1,
                                 new QTableWidgetItem(QString::number(city.latitude, 'f', 4)));
        ui.tableWidget->setItem(row, 2,
                                 new QTableWidgetItem(QString::number(city.longitude, 'f', 4)));
    }
}

void CityEditor::clearEditForm ()
{
    ui.lineEditName->clear();
    ui.doubleSpinBoxLat->setValue(0.0);
    ui.doubleSpinBoxLon->setValue(0.0);
    ui.tableWidget->clearSelection();
}

bool CityEditor::validateInput ()
{
    QString name = ui.lineEditName->text().trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Название города не может быть пустым!");
        return false;
    }

    double lat = ui.doubleSpinBoxLat->value();
    double lon = ui.doubleSpinBoxLon->value();

    if ((lat < -90.0) || (lat > 90.0))
    {
        QMessageBox::warning(this, "Ошибка",
                             "Широта должна быть в диапазоне от -90 до 90 градусов!");
        return false;
    }

    if ((lon < -180) || (lon > 180))
    {
        QMessageBox::warning(this, "Ошибка",
                             "Долгота должна быть в диапазоне от -180 до 180 градусов!");
        return false;
    }

    return true;
}

void CityEditor::onTableSelectionChanged ()
{
    int currentRow = ui.tableWidget->currentRow();
    if (currentRow >= 0) {
        ui.lineEditName->setText(ui.tableWidget->item(currentRow, 0)->text());
        ui.doubleSpinBoxLat->setValue(
            ui.tableWidget->item(currentRow, 1)->text().toDouble());
        ui.doubleSpinBoxLon->setValue(
            ui.tableWidget->item(currentRow, 2)->text().toDouble());
    }
}

void CityEditor::onAddCity ()
{
    if (!validateInput()) return;

    QString name = ui.lineEditName->text().trimmed();
    double lat = ui.doubleSpinBoxLat->value();
    double lon = ui.doubleSpinBoxLon->value();

    if (m_cityManager->cityExists(name)) {
        QMessageBox::warning(this, "Ошибка",
                             QString("Город '%1' уже существует в списке!").arg(name));
        return;
    }

    m_cityManager->addCity(name, lat, lon);
    loadCitiesToTable();
    clearEditForm();

    QMessageBox::information(this, "Успех",
                             QString("Город '%1' успешно добавлен!").arg(name));
}

void CityEditor::onUpdateCity ()
{
    if (!validateInput()) return;

    int currentRow = ui.tableWidget->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите город для обновления!");
        return;
    }

    QString oldName = ui.tableWidget->item(currentRow, 0)->text();
    QString newName = ui.lineEditName->text().trimmed();
    double lat = ui.doubleSpinBoxLat->value();
    double lon = ui.doubleSpinBoxLon->value();

    // Проверяем, что новое имя не конфликтует с существующими
    if (oldName != newName && m_cityManager->cityExists(newName)) {
        QMessageBox::warning(this, "Ошибка",
                             QString("Город с именем '%1' уже существует!").arg(newName));
        return;
    }

    if (m_cityManager->updateCity(oldName, newName, lat, lon)) {
        loadCitiesToTable();
        clearEditForm();
        QMessageBox::information(this, "Успех", "Город успешно обновлен!");
    }
}

void CityEditor::onDeleteCity ()
{
    int currentRow = ui.tableWidget->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите город для удаления!");
        return;
    }

    QString name = ui.tableWidget->item(currentRow, 0)->text();

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Подтверждение",
                                                              QString("Вы действительно хотите удалить город '%1'?").arg(name),
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_cityManager->removeCity(name);
        loadCitiesToTable();
        clearEditForm();
    }
}

void CityEditor::onRestoreDefaults ()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Подтверждение",
                                                              "Вы действительно хотите восстановить стандартный список городов?\n"
                                                              "Все ваши изменения будут потеряны!",
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_cityManager->loadDefaults();
        loadCitiesToTable();
        clearEditForm();
    }
}

void CityEditor::onSave ()
{
    if (m_cityManager->saveCities()) {
        QMessageBox::information(this, "Успех",
                                 "Конфигурация успешно сохранена в файл cities.conf");
        accept();
    } else {
        QMessageBox::critical(this, "Ошибка",
                              "Не удалось сохранить конфигурацию!");
    }
}

void CityEditor::onCancel ()
{
    // Перезагружаем предыдущую конфигурацию
    m_cityManager->loadCities();
    reject();
}
