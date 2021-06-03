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
    if(db.isOpen()){
        // Will close any open queries before removing the database connection.
        db.close();
    }
}

QSqlError sqliteDB::InitialDBSetup(){
    if(!QSqlDatabase::drivers().contains("QSQLITE", Qt::CaseInsensitive)){
        qDebug() << "SQLite database driver is not available.";
        return db.lastError();
    }

    db = QSqlDatabase::addDatabase("QSQLITE", DBConnectionName);
    db.setDatabaseName(DBPath);

    QSqlError err;

    if(!db.open()){
        err = db.lastError();
        db.close();
        qDebug() << "Databse connection failed to open. Closing the database connection now.";
        return err;
    }

    // Create resource directories

    if(!imagesDir.exists()){
            imagesDir.mkpath(imagesDir.absolutePath());
    }

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
    QSqlQuery q(QSqlDatabase::database(DBConnectionName));
    QStringList tables = db.tables();

    if(!tables.contains("ImageSets", Qt::CaseInsensitive)){
        qDebug() << "Create Image Sets Table.";
        if(!q.exec(CREATE_TABLE_IMAGE_SETS)){
            return q.lastError();
        }
    }

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

    return QSqlError();
}

QSqlError sqliteDB::ConfirmDBConnection(){
    if(!(db.isOpen() && db.contains(DBConnectionName))){
        return InitialDBSetup();
    }

    else
        return QSqlError();
}

int sqliteDB::InsertImageSet(QString name){
    if(ConfirmDBConnection().type() != QSqlError::NoError){
        qDebug() << "Failed to insert image set: " << name;
        return -1;
    }

    qDebug() << "Loading image set: " << name;

    QSqlQuery q(QSqlDatabase::database(DBConnectionName));

    q.prepare(INSERT_IMAGE_SET);

    QString createdOn = QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss");
    QString updatedOn = createdOn;

    q.addBindValue(name);
    q.addBindValue(createdOn);
    q.addBindValue(updatedOn);

    if(!q.exec()){
        qDebug() << "InsertImageSet) " << q.lastError().text();
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

int sqliteDB::InsertImage(QString imagePath, QString name, int height, int width, int imageSetID){
    if(ConfirmDBConnection().type() != QSqlError::NoError){
        qDebug() << "Failed to insert image: " << name;
        return -1;
    }

    qDebug() << "Loading image: " << name;

    QSqlQuery q(QSqlDatabase::database(DBConnectionName));

    q.prepare(INSERT_IMAGE);

    QString createdOn = QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss");

    if(!QFile::exists(imagePath)){
        qDebug() << "InsertImage) Image path is incorrect";
        return -1;
    }

    q.addBindValue(name);
    q.addBindValue(createdOn);
    q.addBindValue(imagePath);
    q.addBindValue(height);
    q.addBindValue(width);
    q.addBindValue(imageSetID);

    if(!q.exec()){
        qDebug() << "InsertImage) " << q.lastError().text();
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

int sqliteDB::InsertModel(QString srcModelPath, QString name, QString type, QString platform, QString searchspace, QString inferencelevel){
    if(ConfirmDBConnection().type() != QSqlError::NoError){
        qDebug() << "Failed to insert model: " << name;
        return -1;
    }

    qDebug() << "Loading model: " << name;

    QSqlQuery q(QSqlDatabase::database(DBConnectionName));

    q.prepare(INSERT_MODEL);

    QString createdOn = QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss");

    QUuid uID = QUuid::createUuid();
    QString suffix = "." + srcModelPath.split(".").last();
    QString newModelPath = modelsDir.absolutePath() + "/" + uID.toString() + suffix;

    if(!QFile::exists(srcModelPath) || !QFile::exists(newModelPath)){
        qDebug() << "InsertImage) Image paths are incorrect";
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

int sqliteDB::InsertEvaluationSet(int imagesetID, int modelID){
    if(ConfirmDBConnection().type() != QSqlError::NoError){
        qDebug() << "Failed to make evaluation set";
        return -1;
    }

    qDebug() << "Making evaluation set.";

    QSqlQuery q(QSqlDatabase::database(DBConnectionName));

    q.prepare(INSERT_EVALUATION_SET);

    QString createdOn = QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss");
    QUuid uID = QUuid::createUuid();
    QString evaluationJsonPath = evaluationsDir.absolutePath() + "/" + uID.toString() + ".json";

    q.addBindValue(createdOn);
    q.addBindValue(imagesetID);
    q.addBindValue(modelID);
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

int sqliteDB::InsertResultItem(QString resultImagePath, int imageID, int evaluationSetID){
    if(ConfirmDBConnection().type() != QSqlError::NoError){
        qDebug() << "Failed to make result item";
        return -1;
    }

    QSqlQuery q(QSqlDatabase::database(DBConnectionName));

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

QString sqliteDB::getEvalutionJsonPath(int evaluationSetId){
    if(ConfirmDBConnection().type() != QSqlError::NoError){
        qDebug() << "getEvaluationJsonPath) DB Connection Failed.";
        return "";
    }

    QSqlQuery q(QSqlDatabase::database(DBConnectionName));

    q.prepare("SELECT EvaluationJsonPath FROM EvaluationSets WHERE Id=?");
    q.addBindValue(evaluationSetId);

    while(q.next()){
        QString evaluationJsonPath = q.value(0).toString();
        return evaluationJsonPath;
    }

    return "";
}

int sqliteDB::getEvaluationSetID(int imageSetId, int modelId){
    if(ConfirmDBConnection().type() != QSqlError::NoError){
        qDebug() << "evaluationSetExists) DB Connection Failed.";
        return -1;
    }

    QSqlQuery q(QSqlDatabase::database(DBConnectionName));

    q.prepare("SELECT Id FROM EvaluationSets WHERE ImageSetId=? AND ModelId=?");
    q.addBindValue(imageSetId);
    q.addBindValue(modelId);

    while(q.next()){
        int id = q.value(0).toInt();
        return id;
    }

    return -1;
}
