#pragma once

#include "CityManager.hpp"
#include "ui_CityEditor.h"

#include <QDialog>

class CityEditor : public QDialog
{
    Q_OBJECT

public:
    explicit CityEditor (CityManager * manager, QWidget * parent = nullptr);
    ~CityEditor ();

private:
    Ui::CityEditor  ui;
    CityManager   * m_cityManager {nullptr};

    void loadCitiesToTable ();
    void clearEditForm ();
    bool validateInput ();
    Q_SLOT void onTableSelectionChanged ();
    Q_SLOT void onAddCity ();
    Q_SLOT void onUpdateCity ();
    Q_SLOT void onDeleteCity ();
    Q_SLOT void onRestoreDefaults ();
    Q_SLOT void onSave ();
    Q_SLOT void onCancel ();
};
