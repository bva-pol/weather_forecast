#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>

struct CurrentWeather {
    double temperature {0.0};
    int humidity {0};
    double pressure {0.0};
    double windSpeed {0.0};
    int windDirection {0};
    QString windDirectionText;
};

struct ForecastData {
    QDateTime timestamp;
    double temperature {0.0};
};

class WeatherAPI : public QObject
{
    Q_OBJECT
public:
    explicit WeatherAPI (QObject * parent = nullptr);
    void fetchWeather (double lat, double lon);

    Q_SIGNAL void currentWeatherReady (const CurrentWeather & weather);
    Q_SIGNAL void forecastReady (const QVector<ForecastData> & temperatures);
    Q_SIGNAL void errorOccurred (const QString & errorString);

private:
    QNetworkAccessManager * _manager {nullptr};
    QString                 _apiUrl;

    Q_SLOT void handleReply (QNetworkReply * reply);
    QString windDirectionToString (int degrees);
};
