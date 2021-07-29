#ifndef SQLITEDB_H
#define SQLITEDB_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QLatin1String>
#include <QSqlError>
#include <shared_include.h>

//static QDir imagesDir = QDir("resources/images");
static QDir modelsDir = QDir("resources/models");
static QDir evaluationsDir = QDir("resources/evaluations");
static QDir resultImagesDir = QDir("resources/resultImages");

//const auto CREATE_TABLE_IMAGE_SETS = QLatin1String(
//            R"(
//            CREATE TABLE IF NOT EXISTS ImageSets(
//                Id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
//                Name TEXT NOT NULL,
//                CreatedOn TEXT NOT NULL,
//                UpdatedOn TEXT NOT NULL,
//                NumOfImages integer DEFAULT 0)
//            )");

const auto CREATE_TABLE_IMAGES = QLatin1String(
            R"(
            CREATE TABLE IF NOT EXISTS Images(
                Id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
                Name TEXT NOT NULL,
                InferencedAt TEXT NOT NULL,
                ImagePath TEXT NOT NULL,
                Height INTEGER NOT NULL,
                Width INTEGER NOT NULL)
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
                StartedOn TEXT NOT NULL,
                ModelId INTEGER NOT NULL,
                EnsembleModelId INTEGER NOT NULL,
                EvaluationJsonPath TEXT NOT NULL,
                FOREIGN KEY(ModelId) REFERENCES Models(Id) ON DELETE CASCADE,
                FOREIGN KEY(EnsembleModelId) REFERENCES Models(Id) ON DELETE CASCADE)
            )");

const auto CREATE_TABLE_RESULT_ITEMS = QLatin1String(
            R"(
            CREATE TABLE IF NOT EXISTS ResultItems(
                Id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
                CreatedOn TEXT NOT NULL,
                ResultImagePath TEXT NOT NULL,
                ImageId INTEGER NOT NULL,
                EvaluationSetId INTEGER NOT NULL,
                FOREIGN KEY(ImageId) REFERENCES Images(Id) ON DELETE CASCADE,
                FOREIGN KEY(EvaluationSetId) REFERENCES EvaluationSets(Id) ON DELETE CASCADE)
            )");

const auto CREATE_TABLE_SAVE_PATH = QLatin1String(
            R"(
            CREATE TABLE IF NOT EXISTS SavePath(RootPath TEXT NOT NULL)
            )");

//const auto INSERT_IMAGE_SET = QLatin1String(
//            R"(
//            INSERT OR REPLACE INTO ImageSets(Name, CreatedOn, UpdatedOn) VALUES(?, ?, ?)
//            )");

const auto INSERT_IMAGE = QLatin1String(
            R"(
            INSERT OR REPLACE INTO Images(Name, InferencedAt, ImagePath, Height, Width) VALUES(?, ?, ?, ?, ?)
            )");

const auto INSERT_MODEL = QLatin1String(
            R"(
            INSERT OR REPLACE INTO Models(Name, CreatedOn, ModelPath, Type, Platform, SearchSpace, InferenceLevel) VALUES(?, ?, ?, ?, ?, ?, ?)
            )");

const auto INSERT_EVALUATION_SET = QLatin1String(
            R"(
            INSERT OR REPLACE INTO EvaluationSets(StartedOn, ModelId, EnsembleModelId, EvaluationJsonPath) VALUES(?, ?, ?, ?)
            )");

const auto INSERT_RESULT_ITEM = QLatin1String(
            R"(
            INSERT OR REPLACE INTO ResultItems(CreatedOn, ImageId, EvaluationSetId, ResultImagePath) VALUES(?, ?, ?, ?)
            )");


class sqliteDB
{
public:
    sqliteDB();
    ~sqliteDB();

public:
    QString DBPath = QString("%1%2").arg(qApp->applicationDirPath()).arg("/neuore.db");;

    QSqlError InitialDBSetup();

    QSqlError ConfirmDBConnection(QString connection_name);

//    int InsertImageSet(QString name);
    bool ReplaceSavePath(QString path);
    int InsertImage(QString imagePath, QString name, int height, int width, QString db_connection = "save_thread");
    int InsertModel(QString srcModelPath, QString name, QString type, QString platform, QString searchspace, QString inferencelevel);
    int InsertEvaluationSet(QString StartedOn, int modelID, int ensembleModelId);
    int InsertResultItem(QString resultImagePath, int imageID, int evaluationSetID, QString db_connectino = "save_thread");

    bool DeleteModel(int modelId);

    QString getSavePath(QString db_connection="main_thread");

    QString getEvaluationJsonPath(int evaluationSetId);
    QString getEvaluationSetStartedOn(int evaluationSetId, QString db_connection="main_thread");
    int getEvaluationSetID(QString StartedOn, int modelId, int ensmebleModelId);

    QString getModelPath(int modelId);
    QString getModelName(int modelId);
    QString getModelType(int modelId);

    QString getImagePath(int imageId, QString db_connection="main_thread");
    QString getImageName(int imageId, QString db_connection="main_thread");

    QString getResultImagePath(int resultItemId, QString db_connection="main_thread");
    int getImageId(int resultItemId, QString db_connection="main_thread");
};

#endif // SQLITEDB_H
