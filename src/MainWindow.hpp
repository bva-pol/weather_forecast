#pragma once

#include "ui_MainWindow.h"

#include <QMainWindow>
#include <QVector>

QT_CHARTS_BEGIN_NAMESPACE
    class QLineSeries;
    class QDateTimeAxis;
    class QValueAxis;
    class QScatterSeries;
    class QAreaSeries;
QT_CHARTS_END_NAMESPACE

class WeatherAPI;
class CurrentWeather;
class CityManager;
class ForecastData;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow (QWidget *parent = nullptr);
    ~MainWindow () = default;

private:
    Ui::MainWindow              ui;
    WeatherAPI                * _api {nullptr};
    CityManager               * _cityManager {nullptr};
    QtCharts::QChart          * _chart {nullptr};
    QtCharts::QLineSeries     * _series {nullptr};
    QtCharts::QDateTimeAxis   * _axisXDate {nullptr};
    QtCharts::QDateTimeAxis   * _axisXTime {nullptr};
    QtCharts::QValueAxis      * _axisY {nullptr};
    QMap<QString, QPair<double, double>> _cityCoords;
    QVector<QtCharts::QAreaSeries*> _dayZones;  // Зоны для разных дней

    void createDayZones (const QVector<ForecastData> &data);
    void clearDayZones ();
    void initChart ();
    void setupCityComboBox ();
    void updateAxisXFormat ();
    QString getOptimalDateFormat (int chartWidth) const;
    QString getOptimalTimeFormat (int chartWidth) const;
    QVector<QDateTime> getDateMarkers (const QVector<ForecastData> & forecast) const;

    Q_SLOT void onCityChanged (int index);
    Q_SLOT void onRefreshClicked ();
    Q_SLOT void updateCurrentWeather (const CurrentWeather & weather);
    Q_SLOT void updateForecastChart (const QVector<ForecastData> & forecast);
    Q_SLOT void updateCityComboBox ();
    Q_SLOT void onEditCities ();
    Q_SLOT void onChartViewResized ();
};
