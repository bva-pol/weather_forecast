#include "WeatherApi.hpp"

#include <QUrlQuery>
#include <QDateTime>

const QString UrlAddress {"https://api.open-meteo.com/v1/forecast"};
const QString Latitude {"?latitude=%1"};
const QString Longitude {"&longitude=%2"};
const QString CurrentWeatherParams {"&current=temperature_2m"
                                   ",relative_humidity_2m"
                                   ",surface_pressure"
                                   ",wind_speed_10m"
                                   ",wind_direction_10m"};
const QString ForecastParams {"&hourly=temperature_2m"
                             "&timezone=auto"
                             "&forecast_days=4"};

WeatherAPI::WeatherAPI (QObject * parent)
    : QObject (parent)
{
    _manager = new QNetworkAccessManager(this);
    connect(_manager, &QNetworkAccessManager::finished, this, &WeatherAPI::handleReply);
}

void WeatherAPI::fetchWeather (double lat, double lon)
{
    // Формируем URL для Open-Meteo с параметрами: температура, влажность, давление, скорость и направление ветра на 10м
    QString urlTemplate {UrlAddress+Latitude+Longitude+CurrentWeatherParams+ForecastParams};
    _apiUrl = urlTemplate.arg(lat).arg(lon);
    QNetworkRequest request(_apiUrl);
    _manager->get(request);
}

void WeatherAPI::handleReply (QNetworkReply * reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();

    // --- Парсинг текущей погоды ---
    QJsonObject currentObj = root["current"].toObject();
    CurrentWeather current;
    current.temperature = currentObj["temperature_2m"].toDouble();
    current.humidity = currentObj["relative_humidity_2m"].toInt();
    current.pressure = currentObj["surface_pressure"].toDouble();
    current.windSpeed = currentObj["wind_speed_10m"].toDouble();
    current.windDirection = currentObj["wind_direction_10m"].toInt();
    current.windDirectionText = windDirectionToString(current.windDirection);

    emit currentWeatherReady(current);

    // --- Парсинг почасового прогноза ---
    QJsonObject hourlyObj = root["hourly"].toObject();
    QJsonArray timeArray = hourlyObj["time"].toArray();
    QJsonArray tempArray = hourlyObj["temperature_2m"].toArray();

    QVector<ForecastData> forecast;

    for (int i = 0; i < timeArray.size() && i < tempArray.size(); ++i) {
        ForecastData data;
        data.timestamp = QDateTime::fromString(timeArray[i].toString(), Qt::ISODate);
        data.temperature = tempArray[i].toDouble();
        forecast.append(data);
    }

    emit forecastReady(forecast);
    reply->deleteLater();
}

QString WeatherAPI::windDirectionToString (int degrees)
{
    // Нормализуем градусы
    degrees = degrees % 360;
    if (degrees < 0) degrees += 360;

    // Определяем направление по 8 румбам
    if (degrees >= 337.5 || degrees < 22.5)   return QString("Северный");   // Север
    if (degrees >= 22.5  && degrees < 67.5)   return QString("Северо-восточный");  // Северо-восток
    if (degrees >= 67.5  && degrees < 112.5)  return QString("Восточный");   // Восток
    if (degrees >= 112.5 && degrees < 157.5)  return QString("Юго-восточный");  // Юго-восток
    if (degrees >= 157.5 && degrees < 202.5)  return QString("Южный");   // Юг
    if (degrees >= 202.5 && degrees < 247.5)  return QString("Юго-западный");  // Юго-запад
    if (degrees >= 247.5 && degrees < 292.5)  return QString("Западный");   // Запад
    if (degrees >= 292.5 && degrees < 337.5)  return QString("Северо-западный");  // Северо-запад

    return QString("?");
}
