#ifndef DATABASE_H
#define DATABASE_H

#include <iostream>
#include "sqlite3.h"
#include <ctime>

using namespace std;

const static char* QUERY_USER_GROUPS_TABLE_CREATE = "CREATE TABLE IF NOT EXISTS UserGroups (Id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, Name TEXT NOT NULL, CreatedOn TEXT NOT NULL)";

const static char* QUERY_IMAGE_SETS_TABLE_CREATE = "CREATE TABLE IF NOT EXISTS ImageSets ("\
                "Id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "\
                "Name TEXT NOT NULL, "\
                "CreatedOn TEXT NOT NULL, "\
                "NumOfImages INTEGER DEFAULT 0, "\
                "UserGroupId INTEGER NOT NULL,"\
                "FOREIGN KEY(UserGroupId) REFERENCES UserGroups(Id) ON DELETE CASCADE );";

const static char* QUERY_IMAGES_TABLE_CREATE = "CREATE TABLE IF NOT EXISTS Images ("\
                "Id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "\
                "Name TEXT NOT NULL, "\
                "CreatedOn TEXT NOT NULL, "\
                "ImagePath TEXT NOT NULL, "\
                "ImageSetId INTEGER NOT NULL,"\
                "FOREIGN KEY(ImageSetId) REFERENCES ImageSets(Id) ON DELETE CASCADE )";

const static char* QUERY_MODELS_TABLE_CREATE = "CREATE TABLE IF NOT EXISTS Models ("\
                "Id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "\
                "Name TEXT NOT NULL, "\
                "CreatedOn TEXT NOT NULL, "\
                "Type TEXT NOT NULL, "\
                "Platform TEXT NOT NULL, "\
                "Interpolation Type TEXT NOT NULL, "\
                "NumClasses INTEGER NOT NULL, "\
                "ModelPath TEXT NOT NULL )";

const static char* QUERY_EVALUATION_SETS_TABLE_CREATE = "CREATE TABLE IF NOT EXISTS EvaluationSets ("\
                "Id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "\
                "ImageSetId INTEGER NOT NULL, "\
                "ModelId INTEGER NOT NULL, "\
                "ResultCSV TEXT NOT NULL, "\
                "CreatedOn TEXT NOT NULL, "\
                "FOREIGN KEY(ImageSetId) REFERENCES ImageSets(Id) ON DELETE CASCADE, "\
                "FOREIGN KEY(ModelId) REFERENCES Models(Id) ON DELETE CASCADE )";

const static char* QUERY_RESULT_IMAGES_TABLE_CREATE = "CREATE TABLE IF NOT EXISTS ResultImages ("\
                "Id INTEGER NOT NULL, "\
                "EvaluationSetId INTEGER NOT NULL, "\
                "ImageId INTEGER NOT NULL, "\
                "ResultImagePath TEXT NOT NULL, "\
                "InferTime REAL NOT NULL, "\
                "CreatedOn TEXT NOT NULL, "\
                "FOREIGN KEY(EvaluationSetId) REFERENCES EvaluationSets(Id) ON DELETE CASCADE, "\
                "FOREIGN KEY(ImageId) REFERENCES Images(Id) ON DELETE CASCADE, "\
                "PRIMARY KEY(Id, EvaluationSetId, ImageId) )";

const static char* QUERY_TABLE_EXIST = "SELECT name FROM sqlite_master WHERE type='table' AND name='?'";

const static char* QUERY_TABLE_DROP = "DROP TABLE IF EXISTS ?";

const static char* QUERY_DB_CLEAN_UP = "VACUUM";

const static char* QUERY_DB_BEGIN = "BEGIN";

const static char* QUERY_DB_COMMIT = "COMMIT";

const static char* QUERY_DB_ROLLBACK = "ROLLBACK";

const static char* QUERY_USER_GROUP_INSERT = "INSERT OR REPLACE INTO UserGroups(Name, CreatedOn) VALUES(?, ?)";

const static char* QUERY_IMAGE_SET_INSERT = "INSERT OR REPLACE INTO ImageSets(Name, CreatedOn, UserGroupId) VALUES(?, ?, ?)";

const static char* QUERY_MODEL_INSERT = "INSERT OR REPLACE INTO Models VALUES(?, ?, ?, ?, ?, ?, ?)";

const static char* QUERY_EVALUATION_SET_INSERT = "INSERT OR REPLACE INTO EvaluationSets VALUES(?, ?, ?)";

const static char* QUERY_IMAGE_INSERT = "INSERT OR REPLACE INTO Images(Name, CreatedOn, ImagePath, ImageSetId) VALUES(?, ?, ?, ?)";

const static char* QUERY_RESULT_IMAGE_INSERT = "INSERT OR REPLACE INTO ResultImages VALUES(?, ?, ?, ?, ?)";

class neuroeDB
{
private:
    sqlite3* db;

public:
    neuroeDB();
    ~neuroeDB();

    sqlite3* getDB();

    bool Open(const char* dbFileName);
    bool Close();
    bool Begin();
    bool Commit();
    bool RollBack();

    bool CreateUserGroupTable();
    bool CreateImageSetTable();
    bool CreateModelTable();
    bool CreateEvaluationSetTable();
    bool CreateImagesTable();
    bool CreateResultImagesTable();

    bool DropTable(const char* table_name);
    bool IsTableExist(const char* table_name);

    bool InsertUserGroup(const char* name);
    bool InsertImageSet(const char* name, int userGroupId);
    bool InsertModel(const char* name, const char* type, const char* platform, const char* interpoly, int numOfClasses, const char* modelPath);
    bool InsertEvaluationSet(int imageSetId, int modelId, const char* resultCSVPath);
    bool InsertImage(int imageSetId, const char* name, const char* imagePath);
    bool InsertResultImage(int evaluationSetId, int imageId, const char* resultImagePath, double inferTime);

    bool DeleteUserGroup();
    bool DeleteImageSet();
    bool DeleteModel();
    bool DeleteEvaluationSet();
    bool DeleteImage();
    bool DeleteResultImage();
};

#endif

