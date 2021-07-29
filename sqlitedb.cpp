#include "sqlitedb.h"

sqliteDB::sqliteDB()
{
    QStringList drivers = QSqlDatabase::drivers();
    if(drivers.isEmpty()){
        qDebug() << "There are no available database drivers. NO database connection will be created.";
    }
    else{
        qDebug() << "Available database drivers:" << drivers;
    }
}

sqliteDB::~sqliteDB(){
}

QSqlError sqliteDB::InitialDBSetup(){
    QSqlDatabase main_thread_db = QSqlDatabase::addDatabase("QSQLITE", "main_thread");
    main_thread_db.setDatabaseName(DBPath);

    QSqlError err;

    main_thread_db.open();

    // Create resource directories

//    if(!imagesDir.exists()){
//            imagesDir.mkpath(imagesDir.absolutePath());
//    }

    if(!modelsDir.exists()){
            modelsDir.mkpath(modelsDir.absolutePath());
    }

    if(!evaluationsDir.exists()){
            evaluationsDir.mkpath(evaluationsDir.absolutePath());
    }

    if(!resultImagesDir.exists()){
            resultImagesDir.mkpath(resultImagesDir.absolutePath());
    }


    // Create Tables If not created already
    QSqlQuery q(QSqlDatabase::database("main_thread"));
    QStringList tables = main_thread_db.tables();

//    if(!tables.contains("ImageSets", Qt::CaseInsensitive)){
//        qDebug() << "Create Image Sets Table.";
//        if(!q.exec(CREATE_TABLE_IMAGE_SETS)){
//            return q.lastError();
//        }
//    }

    if(!tables.contains("Images", Qt::CaseInsensitive)){
        qDebug() << "Create Images Table.";
        if(!q.exec(CREATE_TABLE_IMAGES)){
            return q.lastError();
        }
    }

    if(!tables.contains("Models", Qt::CaseInsensitive)){
        qDebug() << "Create Models Table.";
        if(!q.exec(CREATE_TABLE_MODELS)){
            return q.lastError();
        }
    }

    if(!tables.contains("EvaluationSets", Qt::CaseInsensitive)){
        qDebug() << "Create Evaluation Sets Table.";
        if(!q.exec(CREATE_TABLE_EVALUATION_SETS)){
            return q.lastError();
        }
    }

    if(!tables.contains("ResultItems", Qt::CaseInsensitive)){
        qDebug() << "Create Result Items Table.";
        if(!q.exec(CREATE_TABLE_RESULT_ITEMS)){
            return q.lastError();
        }
    }

    if(!tables.contains("SavePath", Qt::CaseInsensitive)){
        qDebug() << "Create User Defined Save Path Table.";
        if(!q.exec(CREATE_TABLE_SAVE_PATH)){
            return q.lastError();
        }
    }

    return QSqlError();
}

QSqlError sqliteDB::ConfirmDBConnection(QString connection_name){
    QSqlDatabase db = QSqlDatabase::database(connection_name);
    if(!db.isOpen()){
        return InitialDBSetup();
    }

    else
        return QSqlError();
}


int sqliteDB::InsertImage(QString imagePath, QString name, int height, int width, QString db_connection){
    if(ConfirmDBConnection(db_connection).type() != QSqlError::NoError){
        qDebug() << "Failed to insert image: " << name;
        return -1;
    }

    qDebug() << "Loading image: " << name;

    QSqlQuery q(QSqlDatabase::database(db_connection));

    q.prepare(INSERT_IMAGE);

    QString inferencedAt = QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss.zzz");

    if(!QFile::exists(imagePath)){
        qDebug() << "InsertImage) Image path is incorrect";
        return -1;
    }

    q.addBindValue(name);
    q.addBindValue(inferencedAt);
    q.addBindValue(imagePath);
    q.addBindValue(height);
    q.addBindValue(width);

    if(!q.exec()){
        qDebug() << "InsertImage) " << q.lastError().text();
        return -1;
    }

    QVariant id_var = q.lastInsertId();

    if(id_var.canConvert(QMetaType::Int)){
        return id_var.toInt();
    }
    else{
        return -1;
    }
}

bool sqliteDB::ReplaceSavePath(QString path){
    QString db_connection = "main_thread";
    if(ConfirmDBConnection(db_connection).type() != QSqlError::NoError){
        qDebug() << "Failed to insert save path";
        return false;
    }

    QSqlQuery q(QSqlDatabase::database(db_connection));

    q.prepare("DELETE FROM SavePath");
    if(!q.exec()){
        qDebug() << "ReplaceSavePath) " << q.lastError().text();
        return false;
    }

    q.prepare("INSERT INTO SavePath(RootPath) VALUES(?)");
    q.addBindValue(path);
    if(!q.exec()){
        qDebug() << "ReplaceSavePath) " << q.lastError().text();
        return false;
    }

    return true;
}

QString sqliteDB::getSavePath(QString db_connection){
    if(ConfirmDBConnection(db_connection).type() != QSqlError::NoError){
        return "";
    }

    QSqlQuery q(QSqlDatabase::database(db_connection));

    q.prepare("SELECT RootPath FROM SavePath");
    if(!q.exec()){
        return "";
    }

    if(q.next()){
        return q.value(0).toString();
    }

    return "";
}

int sqliteDB::InsertModel(QString srcModelPath, QString name, QString type, QString platform, QString searchspace, QString inferencelevel){
    QString db_connection = "main_thread";
    if(ConfirmDBConnection(db_connection).type() != QSqlError::NoError){
        qDebug() << "Failed to insert model: " << name;
        return -1;
    }

    QSqlQuery q(QSqlDatabase::database(db_connection));

    q.prepare(INSERT_MODEL);

    QString createdOn = QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss");

    QUuid uID = QUuid::createUuid();
    QString suffix = "." + srcModelPath.split(".").last();
    QString newModelPath = modelsDir.absolutePath() + "/" + uID.toString().remove(QChar('{')).remove(QChar('}')) + suffix;

    if(!QFile::exists(srcModelPath)){
        qDebug() << "Insert Model) Selected Model path does not exist.";
        return -1;
    }

    QFile::copy(srcModelPath, newModelPath);

    q.addBindValue(name);
    q.addBindValue(createdOn);
    q.addBindValue(newModelPath);
    q.addBindValue(type);
    q.addBindValue(platform);
    q.addBindValue(searchspace);
    q.addBindValue(inferencelevel);

    if(!q.exec()){
        qDebug() << "InsertModel) " << q.lastError().text();
        return -1;
    }

    QVariant id = q.lastInsertId();
    if(id.canConvert(QMetaType::Int)){
        return id.toInt();
    }
    else{
        return -1;
    }
}

int sqliteDB::InsertEvaluationSet(QString StartedOn, int modelID, int ensembleModelId){
    QString db_connection = "main_thread";
    if(ConfirmDBConnection(db_connection).type() != QSqlError::NoError){
        qDebug() << "Failed to make evaluation set";
        return -1;
    }

    qDebug() << "Making evaluation set.";

    QSqlQuery q(QSqlDatabase::database(db_connection));

    q.prepare(INSERT_EVALUATION_SET);

    QUuid uID = QUuid::createUuid();
    QString evaluationJsonPath = evaluationsDir.absolutePath() + "/" + uID.toString().remove(QChar('{')).remove(QChar('}')) + ".json";

    q.addBindValue(StartedOn);
    q.addBindValue(modelID);
    q.addBindValue(ensembleModelId);
    q.addBindValue(evaluationJsonPath);

    if(!q.exec()){
        qDebug() << "InsertEvaluationSet) " << q.lastError().text();
        return -1;
    }

    QVariant id = q.lastInsertId();
    if(id.canConvert(QMetaType::Int)){
        return id.toInt();
    }
    else{
        return -1;
    }
}

int sqliteDB::InsertResultItem(QString resultImagePath, int imageID, int evaluationSetID, QString db_connection){
    if(ConfirmDBConnection(db_connection).type() != QSqlError::NoError){
        qDebug() << "Failed to make result item";
        return -1;
    }

    QSqlQuery q(QSqlDatabase::database(db_connection));

    q.prepare(INSERT_RESULT_ITEM);

    QString createdOn = QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss");

    q.addBindValue(createdOn);
    q.addBindValue(imageID); // which image of the image set
    q.addBindValue(evaluationSetID); // image set & model information
    q.addBindValue(resultImagePath);

    if(!q.exec()){
        qDebug() << "InsertResultItem) " << q.lastError().text();
        return -1;
    }

    QVariant id = q.lastInsertId();
    if(id.canConvert(QMetaType::Int)){
        return id.toInt();
    }
    else{
        return -1;
    }
}

QString sqliteDB::getEvaluationJsonPath(int evaluationSetId){
    QString db_connection = "main_thread";
    if(ConfirmDBConnection(db_connection).type() != QSqlError::NoError){
        qDebug() << "getEvaluationJsonPath) DB Connection Failed.";
        return "";
    }

    QSqlQuery q(QSqlDatabase::database(db_connection));

    q.prepare("SELECT EvaluationJsonPath FROM EvaluationSets WHERE Id=?");
    q.addBindValue(evaluationSetId);

    if(!q.exec()){
        qDebug() << "getEvaluationJsonPath) " << q.lastError().text();
        return "";
    }

    while(q.next()){
        QString evaluationJsonPath = q.value(0).toString();
        return evaluationJsonPath;
    }

    return "";
}

QString sqliteDB::getEvaluationSetStartedOn(int evaluationSetId, QString db_connection){
    if(ConfirmDBConnection(db_connection).type() != QSqlError::NoError){
        qDebug() << "evaluationSetExists) DB Connection Failed.";
        return "";
    }

    QSqlQuery q(QSqlDatabase::database(db_connection));

    q.prepare("SELECT StartedOn FROM EvaluationSets WHERE Id=?");
    q.addBindValue(evaluationSetId);

    if(!q.exec()){
        qDebug() << "getEavluationSetStartedOn) " << q.lastError().text();
    }

    while(q.next()){
        return q.value(0).toString();
    }

    return "";
}

int sqliteDB::getEvaluationSetID(QString StartedOn, int modelId, int ensembleModelId){
    QString db_connection = "main_thread";
    if(ConfirmDBConnection(db_connection).type() != QSqlError::NoError){
        qDebug() << "evaluationSetExists) DB Connection Failed.";
        return -1;
    }

    QSqlQuery q(QSqlDatabase::database(db_connection));

    q.prepare("SELECT Id FROM EvaluationSets WHERE StartedOn=? AND ModelId=? AND EnsembleModelId=?");
    q.addBindValue(StartedOn);
    q.addBindValue(modelId);
    q.addBindValue(ensembleModelId);

    if(!q.exec()){
        qDebug() << "getEvaluationSetId" << q.lastError().text();
    }

    while(q.next()){
        int id = q.value(0).toInt();
        return id;
    }

    return -1;
}

QString sqliteDB::getModelPath(int modelId){
    QString db_connection = "main_thread";
    if(ConfirmDBConnection(db_connection).type() != QSqlError::NoError){
        qDebug() << "getModelPath) DB Connection Failed.";
        return "";
    }

    QSqlQuery q(QSqlDatabase::database(db_connection));

    q.prepare("SELECT ModelPath FROM Models WHERE Id=?");
    q.addBindValue(modelId);

    if(!q.exec()){
        qDebug() << "getModelPath) " << q.lastError().text();
        return "";
    }

    while(q.next()){
        return q.value(0).toString();
    }

    return "";
}

QString sqliteDB::getModelName(int modelId){
    QString db_connection = "main_thread";
    if(ConfirmDBConnection(db_connection).type() != QSqlError::NoError){
        qDebug() << "getModelName) DB Connection Failed.";
        return "";
    }

    QSqlQuery q(QSqlDatabase::database(db_connection));

    q.prepare("SELECT Name FROM Models WHERE Id=?");
    q.addBindValue(modelId);

    if(!q.exec()){
        qDebug() << "getModelName) " << q.lastError().text();
        return "";
    }

    while(q.next()){
        return q.value(0).toString();
    }

    return "";
}

QString sqliteDB::getModelType(int modelId){
    QString db_connection = "main_thread";
    if(ConfirmDBConnection(db_connection).type() != QSqlError::NoError){
        qDebug() << "getModelType) DB Connection Failed.";
        return "";
    }

    QSqlQuery q(QSqlDatabase::database(db_connection));

    q.prepare("SELECT Type FROM Models WHERE Id=?");
    q.addBindValue(modelId);

    if(!q.exec()){
        qDebug() << "getModelType) " << q.lastError().text();
        return "";
    }

    while(q.next()){
        return q.value(0).toString();
    }

    return "";
}

QString sqliteDB::getImagePath(int imageId, QString db_connection){
    if(ConfirmDBConnection(db_connection).type() != QSqlError::NoError){
        qDebug() << "getImagePath) DB Connection Failed.";
        return "";
    }

    QSqlQuery q(QSqlDatabase::database(db_connection));

    q.prepare("SELECT ImagePath FROM Images WHERE Id = ?");
    q.addBindValue(imageId);

    if(!q.exec()){
        qDebug() << "getImagePath) " << q.lastError().text();
    }

    while(q.next()){
        return q.value(0).toString();
    }

    return "";
}

QString sqliteDB::getImageName(int imageId, QString db_connection){
    if(ConfirmDBConnection(db_connection).type() != QSqlError::NoError){
        qDebug() << "getImageName) DB Connection Failed.";
        return "";
    }

    QSqlQuery q(QSqlDatabase::database(db_connection));

    q.prepare("SELECT Name FROM Images WHERE Id = ?");
    q.addBindValue(imageId);

    if(!q.exec()){
        qDebug() << "getImageName) " << q.lastError().text();
    }

    while(q.next()){
        return q.value(0).toString();
    }

    return "";
}

QString sqliteDB::getResultImagePath(int resultItemId, QString db_connection){
    if(ConfirmDBConnection(db_connection).type() != QSqlError::NoError){
        qDebug() << "getResultImagePath) DB Connection Failed.";
        return "";
    }

    QSqlQuery q(QSqlDatabase::database(db_connection));

    q.prepare("SELECT ResultImagePath FROM ResultItems WHERE Id = ?");
    q.addBindValue(resultItemId);

    if(!q.exec()){
        qDebug() << "getResultImagePath) " << q.lastError().text();
    }

    while(q.next()){
        return q.value(0).toString();
    }

    return "";
}

int sqliteDB::getImageId(int resultItemId, QString db_connection){
    if(ConfirmDBConnection(db_connection).type() != QSqlError::NoError){
        qDebug() << "getResultImagePath) DB Connection Failed.";
        return -1;
    }

    QSqlQuery q(QSqlDatabase::database(db_connection));

    q.prepare("SELECT ImageId FROM ResultItems WHERE Id = ?");
    q.addBindValue(resultItemId);

    if(!q.exec()){
        qDebug() << "getImageId) " << q.lastError().text();
    }

    while(q.next()){
        return q.value(0).toInt();
    }

    return -1;
}


