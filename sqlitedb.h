#ifndef SQLITEDB_H
#define SQLITEDB_H

#include <shared_include.h>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QLatin1String>
#include <QDir>
#include <QChar>
#include <QMessageBox>
#include <QDateTime>
#include <QUuid>
#include <QFile>

const auto CREATE_TABLE_IMAGE_SETS = QLatin1String(
            R"(
            CREATE TABLE IF NOT EXISTS ImageSets(
                Id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
                Name TEXT NOT NULL,
                CreatedOn TEXT NOT NULL,
                UpdatedOn TEXT NOT NULL,
                NumOfImages integer DEFAULT 0)
            )");

const auto CREATE_TABLE_IMAGES = QLatin1String(R"(
            CREATE TABLE IF NOT EXITS Images(
                Id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
                Name TEXT NOT NULL,
                CreatedOn TEXT NOT NULL,
                ImagePath TEXT NOT NULL,
                Height INTEGER NOT NULL,
                Width INTEGER NOT NULL,
                ImageSetId INTEGER NOT NULL,
                FOREIGN KEY(ImageSetId) REFERENCES ImageSets(Id) ON DELETE CASCADE)
            )");

const auto CREATE_TABLE_MODELS = QLatin1String(
            R"(
            CREATE TABLE IF NOT EXISTS Models(
                Id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
                Name TEXT NOT NULL,
                CreatedOn TEXT NOT NULL,
                ModelPath TEXT NOT NULL,
                Type TEXT NOT NULL,
                Platform TEXT NOT NULL,
                SearchSpace TEXT NOT NULL,
                InferenceLevel TEXT NOT NULL)
            )");

const auto CREATE_TABLE_EVALUATION_SETS = QLatin1String(
            R"(
            CREATE TABLE IF NOT EXISTS EvaluationSets(
                Id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
                CreatedOn TEXT NOT NULL,
                ImageSetId INTEGER NOT NULL,
                ModelId INTEGER NOT NULL,
                EvaluationJsonPath TEXT NOT NULL,
                FOREIGN KEY(ImageSetId) REFERENCES ImageSets(Id) ON DELETE CASCADE,
                FOREIGN KEY(ModelId) REFERENCES Models(Id) ON DELETE CASCADE)
            )");

const auto CREATE_TABLE_RESULT_ITEMS = QLatin1String(
            R"(
            CREATE TABLE IF NOT EXISTS ResultItems(
                Id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
                CreatedOn TEXT NOT NULL,
                InfTime INTEGER NOT NULL,
                ResultImagePath TEXT NOT NULL,
                ImageId INTEGER NOT NULL,
                EvaluationSetlId INTEGER NOT NULL,
                FOREIGN KEY(ImageSetId) REFERENCES ImageSets(Id) ON DELETE CASCADE,
                FOREIGN KEY(ModelId) REFERENCES Models(Id) ON DELETE CASCADE)
            )");

const auto INSERT_IMAGE_SET = QLatin1String(
            R"(
            INSERT OR REPLACE INTO ImageSets(Name, CreatedOn, UpdatedOn) VALUES(?, ?, ?)
            )");

const auto INSERT_IMAGE = QLatin1String(
            R"(
            INSERT OR REPLACE INTO Images(Name, CreatedOn, ImagePath, Height, Width, ImageSetId) VALUES(?, ?, ?, ?, ?, ?)
            )");

const auto INSERT_MODEL = QLatin1String(
            R"(
            INSERT OR REPLACE INTO Models(Name, CreatedOn, ModelPath, Type, Platform, SearchSpace, InferenceLevel) VALUES(?, ?, ?, ?, ?, ?, ?)
            )");

const auto INSERT_EVALUATION_SET = QLatin1String(
            R"(
            INSERT OR REPLACE INTO EvaluationSets(CreatedOn, ImageSetId, ModelId, EvaluationJsonPath) VALUES(?, ?, ?, ?)
            )");

const auto INSERT_RESULT_ITEM = QLatin1String(
            R"(
            INSERT OR REPLACE INTO ImageSets(CreatedOn, InfTime, ResultImagePath, ImageId, EvaluationSetId) VALUES(?, ?, ?, ?, ?)
            )");


class sqliteDB
{
public:
    sqliteDB();
    ~sqliteDB();

public:
    QSqlDatabase db;

    QString DBPath = QString("%1%2").arg(qApp->applicationDirPath()).arg("/phonebook.db");
    QString DBConnectionName = "neuroeDB";

    QSqlError InitialDBSetup();

    QSqlError ConfirmDBConnection();

    bool InsertImageSet(QString name);
    bool InsertImage(QString srcImagePath, QString name, int height, int width, int imageSetID);
    bool InsertModel(QString srcModelPath, QString name, QString type, QString platform, QString searchspace, QString inferencelevel);
    QString InsertEvaluationSet(int imagesetID, int modelID);
    QString InsertResultItem(int infTime, int imageID, int evaluationSetID, QString suffix);
};

#endif // SQLITEDB_H
