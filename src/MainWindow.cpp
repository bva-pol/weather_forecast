#include "MainWindow.hpp"
#include "WeatherApi.hpp"
#include "CityEditor.hpp"

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QAreaSeries>
#include <QDateTime>
#include <QMessageBox>
#include <QTimer>
#include <QToolTip>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    // Инициализация менеджера городов
    _cityManager = new CityManager(this);
    _cityManager->loadCities();

    // Инициализация API
    _api = new WeatherAPI(this);
    connect(_api, &WeatherAPI::currentWeatherReady, this, &MainWindow::updateCurrentWeather);
    connect(_api, &WeatherAPI::forecastReady, this, &MainWindow::updateForecastChart);
    connect(_api, &WeatherAPI::errorOccurred, [this](const QString &error) {
        QMessageBox::warning(this, "Ошибка API",
                             QString("Не удалось получить данные погоды:\n%1").arg(error));
    });
    setMouseTracking(true);
    ui.chartView->setMouseTracking(true);
    // Настройка графика
    initChart();

    // Настройка выпадающего списка городов
    setupCityComboBox();

    // Сигналы
    connect(ui.comboBoxCity, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onCityChanged);
    connect(ui.btnRefresh, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);
    connect(ui.btnEditCities, &QPushButton::clicked, this, &MainWindow::onEditCities);

    // Обновление комбобокса при изменениях в менеджере городов
    connect(_cityManager, &CityManager::citiesChanged, this, &MainWindow::updateCityComboBox);

    // Загрузка данных для первого города
    if (ui.comboBoxCity->count() > 0) {
        onCityChanged(0);
    }
}

void MainWindow::setupCityComboBox ()
{
    ui.comboBoxCity->clear();
    ui.comboBoxCity->addItems(_cityManager->cityNames());
}

void MainWindow::updateCityComboBox ()
{
    QString currentCity = ui.comboBoxCity->currentText();
    ui.comboBoxCity->clear();
    ui.comboBoxCity->addItems(_cityManager->cityNames());

    // Восстанавливаем выбор, если город еще существует
    int index = ui.comboBoxCity->findText(currentCity);
    if (index >= 0) {
        ui.comboBoxCity->setCurrentIndex(index);
    }
}

void MainWindow::initChart()
{
    _chart = new QtCharts::QChart();
    _chart->setTitle("Почасовой прогноз температуры на 4 дня");
    _chart->legend()->hide();

    // Основная линия
    _series = new QtCharts::QLineSeries();
    _series->setName("Температура");
    _series->setPen(QPen(QColor("#3498db"), 2));
    _chart->addSeries(_series);

    // Настройка осей
    // Верхняя ось - даты
    _axisXDate = new QtCharts::QDateTimeAxis();
    _axisXDate->setTitleText("Дата");
    _axisXDate->setFormat("dd MMMM");  // Формат: "23 Апреля"
    _axisXDate->setLabelsAngle(0);
    _axisXDate->setGridLineVisible(false);
    _axisXDate->setLineVisible(true);
    _axisXDate->setLinePen(QPen(QColor("#34495e"), 2));
    _axisXDate->setLabelsColor(QColor("#2c3e50"));
    _axisXDate->setTitleBrush(QBrush(QColor("#2c3e50")));
    _chart->addAxis(_axisXDate, Qt::AlignTop);
    _series->attachAxis(_axisXDate);

    // Нижняя ось - время
    _axisXTime = new QtCharts::QDateTimeAxis();
    _axisXTime->setTitleText("Время");
    _axisXTime->setFormat("hh:mm");  // Формат: "14:00"
    _axisXTime->setLabelsAngle(-45);
    _axisXTime->setGridLineVisible(true);
    _axisXTime->setGridLineColor(QColor("#ecf0f1"));
    _axisXTime->setLineVisible(true);
    _axisXTime->setLinePen(QPen(QColor("#34495e"), 2));
    _axisXTime->setLabelsColor(QColor("#7f8c8d"));
    _axisXTime->setTitleBrush(QBrush(QColor("#7f8c8d")));
    _chart->addAxis(_axisXTime, Qt::AlignBottom);
    _series->attachAxis(_axisXTime);

    _axisY = new QtCharts::QValueAxis();
    _axisY->setTitleText(QString("Температура (°C)"));
    _axisY->setLabelFormat(QString("%.1f"));
    _chart->addAxis(_axisY, Qt::AlignLeft);
    _series->attachAxis(_axisY);

    // Настройка отображения
    _chart->setBackgroundBrush(QBrush(Qt::white));
    _chart->setPlotAreaBackgroundBrush(QBrush(QColor("#f8f9fa")));
    _chart->setPlotAreaBackgroundVisible(true);

    // Настройка шрифтов
    QFont titleFont;
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    _chart->setTitleFont(titleFont);

    ui.chartView->setChart(_chart);
    ui.chartView->setRenderHint(QPainter::Antialiasing);
}

void MainWindow::updateAxisXFormat ()
{
    int chartWidth = ui.chartView->width();

    // Обновляем формат даты
    QString dateFormat = getOptimalDateFormat(chartWidth);
    _axisXDate->setFormat(dateFormat);

    // Обновляем формат времени
    QString timeFormat = getOptimalTimeFormat(chartWidth);
    _axisXTime->setFormat(timeFormat);

    // Настройка количества делений для оси времени
    if (chartWidth > 1000) {
        _axisXTime->setTickCount(25);  // Много делений
    } else if (chartWidth > 800) {
        _axisXTime->setTickCount(17);
    } else if (chartWidth > 600) {
        _axisXTime->setTickCount(13);
    } else if (chartWidth > 400) {
        _axisXTime->setTickCount(9);
    } else {
        _axisXTime->setTickCount(5);   // Минимум делений
    }

    // Для оси дат всегда 4 деления (по количеству дней)
    _axisXDate->setTickCount(4);
}

QVector<QDateTime> MainWindow::getDateMarkers (const QVector<ForecastData> & forecast) const
{
    QVector<QDateTime> markers;
    QDate lastDate;

    for (const auto & item : forecast) {
        if (item.timestamp.date() != lastDate) {
            lastDate = item.timestamp.date();
            // Используем полдень как маркер для даты
            QDateTime noon(item.timestamp.date(), QTime(12, 0));
            markers.append(noon);
        }
    }

    return markers;
}

QString MainWindow::getOptimalDateFormat (int chartWidth) const
{
    if (chartWidth > 600) {
        return "dd MMMM yyyy";  // "23 Апреля 2024"
    } else if (chartWidth > 400) {
        return "dd MMMM";        // "23 Апреля"
    } else {
        return "dd.MM";          // "23.04"
    }
}

QString MainWindow::getOptimalTimeFormat (int chartWidth) const
{
    if (chartWidth > 800) {
        return "hh:mm";          // "14:30"
    } else if (chartWidth > 500) {
        return "hh:00";          // "14:00"
    } else {
        return "hh";             // "14"
    }
}

void MainWindow::onCityChanged(int index)
{
    if (index >= 0 && index < ui.comboBoxCity->count()) {
        QString cityName = ui.comboBoxCity->currentText();
        ui.labelCityName->setText(cityName);
        onRefreshClicked();
    }
}

void MainWindow::onRefreshClicked()
{
    QString cityName = ui.comboBoxCity->currentText();
    City city = _cityManager->getCity(cityName);

    if (city.isValid()) {
        ui.btnRefresh->setEnabled(false);
        ui.btnRefresh->setText("⏳ Загрузка...");
        _api->fetchWeather(city.latitude, city.longitude);

        // Включаем кнопку обратно после получения данных
        QTimer::singleShot(2000, [this]() {
            ui.btnRefresh->setEnabled(true);
            ui.btnRefresh->setText("🔄 Обновить");
        });
    } else {
        QMessageBox::warning(this, "Ошибка",
                             QString("Город '%1' не найден в конфигурации!").arg(cityName));
    }
}

void MainWindow::onEditCities()
{
    CityEditor editor(_cityManager, this);
    if (editor.exec() == QDialog::Accepted) {
        // Сохраняем конфигурацию
        _cityManager->saveCities();
    } else {
        // Отмена - перезагружаем старую конфигурацию
        _cityManager->loadCities();
    }
}

void MainWindow::onChartViewResized()
{
    updateAxisXFormat();
}

void MainWindow::updateCurrentWeather(const CurrentWeather &weather)
{
    ui.labelTemperature->setText(QString::number(weather.temperature, 'f', 1) + " °C");
    ui.labelHumidity->setText(QString::number(weather.humidity) + " %");
    ui.labelPressure->setText(QString::number(weather.pressure, 'f', 0) + " гПа");
    ui.labelWind->setText(QString::number(weather.windSpeed, 'f', 1) + " м/с");

    if (!weather.windDirectionText.isEmpty()) {
        ui.labelWindDirection->setText(
            QString("%1 (%2°)").arg(weather.windDirectionText)
                .arg(weather.windDirection));
    }
}

void MainWindow::updateForecastChart(const QVector<ForecastData> & forecast)
{
    qDebug() << Q_FUNC_INFO << 1;
    if (forecast.isEmpty()) return;

    _series->clear();

    double minTemp = 100, maxTemp = -100;
    QDateTime minTime, maxTime;

    // Добавляем данные на график
    for (const auto & data : forecast) {
        qreal timestamp = data.timestamp.toMSecsSinceEpoch();
        _series->append(timestamp, data.temperature);

        if (data.temperature < minTemp) minTemp = data.temperature;
        if (data.temperature > maxTemp) maxTemp = data.temperature;

        if (!minTime.isValid() || data.timestamp < minTime)
            minTime = data.timestamp;
        if (!maxTime.isValid() || data.timestamp > maxTime)
            maxTime = data.timestamp;
    }

    // Настройка диапазонов для обеих осей X
    _axisXDate->setRange(minTime, maxTime);
    _axisXTime->setRange(minTime, maxTime);

    // Настройка оси Y
    double range = maxTemp - minTemp;
    double padding = qMax(range * 0.15, 2.0);
    _axisY->setRange(minTemp - padding, maxTemp + padding);

    // Обновляем формат осей
    updateAxisXFormat();

    // Настройка видимости сетки
    _axisXTime->setGridLineVisible(true);
    _axisXDate->setGridLineVisible(false);
    _axisY->setGridLineVisible(true);

    // Обновляем заголовок
    _chart->setTitle(QString("Прогноз температуры (%1 - %2)")
                        .arg(minTime.toString("dd.MM.yyyy"))
                        .arg(maxTime.toString("dd.MM.yyyy")));

    // Цветовое выделение зон по дням (опционально)
    createDayZones(forecast);
    qDebug() << Q_FUNC_INFO << 2;
}

void MainWindow::createDayZones (const QVector<ForecastData> & data)
{
    clearDayZones();

    if (data.isEmpty()) return;

    QDate currentDate;
    QDateTime zoneStart;
    QColor dayColors[] = {
        QColor("#ffffff"),      // Белый
        QColor("#777777"),      // Светло-серый
        QColor("#ffffff"),      // Белый
        QColor("#777777")       // Светло-серый
    };
    int colorIndex = 0;

    for (const auto & item : data) {
        if (item.timestamp.date() != currentDate) {
            // Закрываем предыдущую зону если есть
            if (zoneStart.isValid()) {
                // Создаем зону
                QtCharts::QLineSeries *upperLine = new QtCharts::QLineSeries();
                QtCharts::QLineSeries *lowerLine = new QtCharts::QLineSeries();

                upperLine->append(zoneStart.toMSecsSinceEpoch(), -100);
                upperLine->append(item.timestamp.toMSecsSinceEpoch(), -100);

                lowerLine->append(zoneStart.toMSecsSinceEpoch(), 100);
                lowerLine->append(item.timestamp.toMSecsSinceEpoch(), 100);

                QtCharts::QAreaSeries *area = new QtCharts::QAreaSeries(upperLine, lowerLine);
                area->setColor(dayColors[colorIndex % 4]);
                area->setBorderColor(Qt::transparent);
                area->setOpacity(0.3);

                _chart->addSeries(area);
                area->attachAxis(_axisXTime);
                area->attachAxis(_axisY);

                _dayZones.append(area);
            }

            // Начинаем новую зону
            currentDate = item.timestamp.date();
            zoneStart = item.timestamp;
            colorIndex++;
        }
    }
}

void MainWindow::clearDayZones()
{
    for (QtCharts::QAreaSeries *zone : _dayZones) {
        _chart->removeSeries(zone);
        delete zone;
    }
    _dayZones.clear();
}
