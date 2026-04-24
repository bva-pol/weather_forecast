#include "CityManager.hpp"

#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

CityManager::CityManager (QObject *parent)
    : QObject(parent)
{
    _configPath = defaultConfigPath();
}

QString CityManager::defaultConfigPath () const
{
    // Используем директорию конфигурации пользователя
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    // Создаем директорию, если её нет
    QDir().mkpath(configDir);

    return configDir + "/cities.conf";
}

void CityManager::loadDefaults ()
{
    // Добавляем города по умолчанию
    _cities.clear();
    _cities["Москва"] = City("Москва", 55.7558, 37.6176);
    _cities["Екатеринбург"] = City("Екатеринбург", 56.8389, 60.6057);
    _cities["Тюмень"] = City("Тюмень", 57.1522, 65.5425);
}

bool CityManager::loadCities(const QString &filename)
{
    QString path = filename.isEmpty() ? _configPath : filename;

    QFile file(path);
    if (!file.exists()) {
        qDebug() << "Config file not found, loading defaults:" << path;
        loadDefaults();
        saveCities(path); // Сохраняем дефолтные города
        return true;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open config file for reading:" << path;
        loadDefaults();
        return false;
    }

    _cities.clear();
    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();

        // Пропускаем комментарии и пустые строки
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        // Формат: Название|широта|долгота
        QStringList parts = line.split('|');
        if (parts.size() == 3) {
            QString name = parts[0].trimmed();
            bool latOk, lonOk;
            double lat = parts[1].trimmed().toDouble(&latOk);
            double lon = parts[2].trimmed().toDouble(&lonOk);

            if (latOk && lonOk && !name.isEmpty()) {
                _cities[name] = City(name, lat, lon);
            } else {
                qWarning() << "Invalid city entry:" << line;
            }
        }
    }

    file.close();

    // Если файл пустой или нет валидных городов, загружаем defaults
    if (_cities.isEmpty()) {
        qDebug() << "No valid cities in config, loading defaults";
        loadDefaults();
    }

    emit citiesChanged();
    return true;
}

bool CityManager::saveCities (const QString &filename)
{
    QString path = filename.isEmpty() ? _configPath : filename;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "Cannot open config file for writing:" << path;
        return false;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");

    // Заголовок файла
    stream << "# Конфигурационный файл городов для Метеостанции\n";
    stream << "# Формат: Название|широта|долгота\n";
    stream << "# Пример: Москва|55.7558|37.6176\n\n";

    // Записываем города
    for (const City &city : _cities) {
        stream << city.name << "|"
               << QString::number(city.latitude, 'f', 4) << "|"
               << QString::number(city.longitude, 'f', 4) << "\n";
    }

    file.close();
    qDebug() << "Cities saved to:" << path;
    return true;
}

void CityManager::addCity (const QString &name, double latitude, double longitude)
{
    if (!cityExists(name)) {
        _cities[name] = City(name, latitude, longitude);
        emit cityAdded(name);
        emit citiesChanged();
    }
}

bool CityManager::removeCity (const QString &name)
{
    if (_cities.contains(name)) {
        _cities.remove(name);
        emit cityRemoved(name);
        emit citiesChanged();
        return true;
    }
    return false;
}

bool CityManager::updateCity (const QString &oldName, const QString &newName,
                             double latitude, double longitude)
{
    if (_cities.contains(oldName)) {
        // Если имя изменилось и новое имя уже существует
        if (oldName != newName && _cities.contains(newName)) {
            qWarning() << "City with name" << newName << "already exists";
            return false;
        }

        // Удаляем старую запись
        _cities.remove(oldName);

        // Добавляем новую
        _cities[newName] = City(newName, latitude, longitude);

        emit cityUpdated(oldName, newName);
        emit citiesChanged();
        return true;
    }
    return false;
}

QStringList CityManager::cityNames () const
{
    return _cities.keys();
}

City CityManager::getCity (const QString &name) const
{
    return _cities.value(name, City());
}

QList<City> CityManager::getAllCities () const
{
    return _cities.values();
}

bool CityManager::cityExists (const QString &name) const
{
    return _cities.contains(name);
}
