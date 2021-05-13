#ifndef MODEL_SELECT_H
#define MODEL_SELECT_H

#include <QApplication>
#include <QWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>
#include <QString>
#include <QUuid>
#include <QDir>
#include <QDirIterator>
#include <QHeaderView>
#include <QDebug>
#include <QPushButton>

class model_select : public QWidget
{
    Q_OBJECT

public:
    model_select(QWidget *parent = 0, const char *name = 0 );
private:

public slots:
};

#endif // MODEL_SELECT_H
