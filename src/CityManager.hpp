#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QPair>

struct City {
    QString name;
    double latitude {0.0};
    double longitude {0.0};

    City() : latitude(0), longitude(0) {}
    City (const QString &name, double lat, double lon)
        : name(name), latitude(lat), longitude(lon) {}

    bool isValid() const {
        return !name.isEmpty() && latitude != 0 && longitude != 0;
    }
};

class CityManager : public QObject
{
    Q_OBJECT

public:
    explicit CityManager (QObject * parent = nullptr);

    // Управление городами
    bool loadCities (const QString &filename = "cities.conf");
    bool saveCities (const QString &filename = "cities.conf");

    void addCity (const QString &name, double latitude, double longitude);
    bool removeCity (const QString &name);
    bool updateCity (const QString &oldName, const QString &newName,
                    double latitude, double longitude);

    // Получение данных
    QStringList cityNames () const;
    City getCity (const QString &name) const;
    QList<City> getAllCities () const;
    bool cityExists (const QString &name) const;
    int cityCount () const { return _cities.size(); }

    // Стандартные города по умолчанию
    void loadDefaults();

    Q_SIGNAL void citiesChanged ();
    Q_SIGNAL void cityAdded (const QString &name);
    Q_SIGNAL void cityRemoved (const QString &name);
    Q_SIGNAL void cityUpdated (const QString &oldName, const QString &newName);

private:
    QMap<QString, City> _cities;
    QString             _configPath;

    QString defaultConfigPath () const;
};
