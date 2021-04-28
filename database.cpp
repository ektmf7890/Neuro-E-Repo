#include "database.h"

neuroeDB::neuroeDB(){}

neuroeDB::~neuroeDB(){}

sqlite3* neuroeDB::getDB() {
    return this->db;
}

char buffer[80];

char* getTime() {
    time_t rawtime;
    struct tm * timeinfo;

    time (&rawtime);
    timeinfo = localtime (&rawtime);

    buffer[0] = '\0';
    strftime (buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);

    return buffer;
}

bool neuroeDB::Open(const char* dbFileName){
    int rc = sqlite3_open(dbFileName, &db);

    if(rc != SQLITE_OK) {
        sqlite3_close(db);
        return false;
    }
    return true;
}

bool neuroeDB::Close() {
    return (sqlite3_close(db) == SQLITE_OK);
}

bool neuroeDB::Begin(){
    sqlite3_stmt * stmt;
    bool result = true;

    sqlite3_prepare_v2(this->db, QUERY_DB_BEGIN, -1, &stmt, NULL);

    if(sqlite3_step(stmt) != SQLITE_DONE){
        fwprintf(stderr, L"line %d: %s\n", __LINE__, sqlite3_errmsg16(this->db));
        result = false;
    }

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return result;
}

bool neuroeDB::Commit() {
    sqlite3_stmt * stmt;
    bool result = true;

    sqlite3_prepare_v2(this->db, QUERY_DB_COMMIT, -1, &stmt, NULL);
    if(sqlite3_step(stmt) != SQLITE_DONE){
        fwprintf(stderr, L"line %d: %s\n", __LINE__, sqlite3_errmsg16(this->db));
        result = false;
    }

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return result;
}

bool neuroeDB::RollBack() {
    sqlite3_stmt * stmt;
    bool result = true;

    sqlite3_prepare_v2(this->db, QUERY_DB_ROLLBACK, -1, &stmt, NULL);
    if(sqlite3_step(stmt) != SQLITE_DONE){
        fwprintf(stderr, L"line %d: %s\n", __LINE__, sqlite3_errmsg16(this->db));
        result = false;
    }

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return result;
}

bool neuroeDB::CreateUserGroupTable(){
    sqlite3_stmt * stmt;
    bool result = true;

    sqlite3_prepare_v2(this->db, QUERY_USER_GROUPS_TABLE_CREATE, -1, &stmt, NULL);

    this->Begin();
    if(sqlite3_step(stmt) != SQLITE_DONE){
        fwprintf(stderr, L"line %d: %s\n", __LINE__, sqlite3_errmsg16(this->db));
        result = false;
    }
    this->Commit();

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return result;
}

bool neuroeDB::CreateImageSetTable(){
    sqlite3_stmt * stmt;
    bool result = true;

    sqlite3_prepare_v2(this->db, QUERY_IMAGE_SETS_TABLE_CREATE, -1, &stmt, NULL);

    this->Begin();
    if(sqlite3_step(stmt) != SQLITE_DONE){
        fwprintf(stderr, L"line %d: %s\n", __LINE__, sqlite3_errmsg16(this->db));
        result = false;
    }
    this->Commit();

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return result;
}

bool neuroeDB::CreateModelTable(){
    sqlite3_stmt* stmt;
    bool result = true;

    sqlite3_prepare_v2(this->db, QUERY_MODELS_TABLE_CREATE, -1, &stmt, NULL);

    this->Begin();
    if(sqlite3_step(stmt) != SQLITE_DONE){
        fwprintf(stderr, L"line %d: %s\n", __LINE__, sqlite3_errmsg16(this->db));
        result = false;
    }
    this->Commit();

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return result;
}

bool neuroeDB::CreateEvaluationSetTable() {
    sqlite3_stmt* stmt;
    bool result = true;

    sqlite3_prepare_v2(this->db, QUERY_EVALUATION_SETS_TABLE_CREATE, -1, &stmt, NULL);

    this->Begin();
    if(sqlite3_step(stmt) != SQLITE_DONE){
        fwprintf(stderr, L"line %d: %s\n", __LINE__, sqlite3_errmsg16(this->db));
        result = false;
    }
    this->Commit();

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return result;
}

bool neuroeDB::CreateImagesTable() {
    sqlite3_stmt* stmt;
    bool result = true;

    sqlite3_prepare_v2(this->db, QUERY_IMAGES_TABLE_CREATE, -1, &stmt, NULL);

    this->Begin();
    if(sqlite3_step(stmt) != SQLITE_DONE){
        fwprintf(stderr, L"line %d: %s\n", __LINE__, sqlite3_errmsg16(this->db));
        result = false;
    }
    this->Commit();

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return result;
}

bool neuroeDB::CreateResultImagesTable(){
    sqlite3_stmt* stmt;
    bool result = true;

    sqlite3_prepare_v2(this->db, QUERY_RESULT_IMAGES_TABLE_CREATE, -1, &stmt, NULL);

    this->Begin();
    if(sqlite3_step(stmt) != SQLITE_DONE){
        fwprintf(stderr, L"line %d: %s\n", __LINE__, sqlite3_errmsg16(this->db));
        result = false;
    }
    this->Commit();

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return result;
}

bool neuroeDB::DropTable(const char* table_name) {
    sqlite3_stmt* stmt;
    bool result = true;

    sqlite3_prepare_v2(this->db, QUERY_TABLE_DROP, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, table_name, -1, SQLITE_STATIC);

    if(sqlite3_step(stmt) != SQLITE_DONE){
        fwprintf(stderr, L"line %d: %s\n", __LINE__, sqlite3_errmsg16(this->db));
        result = false;
    }

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return result;
}

bool neuroeDB::IsTableExist(const char* table_name) {
    sqlite3_stmt* stmt;
    bool result = false;

    sqlite3_prepare_v2(this->db, QUERY_TABLE_EXIST, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, table_name, -1, SQLITE_STATIC);

    if(sqlite3_step(stmt) == SQLITE_ROW){
        result = true;
    }

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return result;
}

bool neuroeDB::InsertUserGroup(const char *name) {
    char* createdOn = getTime();

    sqlite3_stmt * stmt;
    bool result = true;

    sqlite3_prepare_v2(this->db, QUERY_USER_GROUP_INSERT, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, createdOn, -1, SQLITE_STATIC);

    // begin
    this->Begin();
    if(sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "result code(bind): %s, %s\n", createdOn, name);
        fwprintf(stderr, L"line %d: %s\n", __LINE__, sqlite3_errmsg16(this->db));
        result = false;
    }
    // commit
    this->Commit();

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return result;
}

bool neuroeDB::InsertImageSet(const char* name, int userGroupId){
    const char* createdOn = getTime();

    sqlite3_stmt * stmt;
    bool result = true;

    sqlite3_prepare_v2(this->db, QUERY_IMAGE_SET_INSERT, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, createdOn, -1, SQLITE_STATIC);;
    sqlite3_bind_int(stmt, 3, userGroupId);

    this->Begin();
    if(sqlite3_step(stmt) != SQLITE_DONE) {
        fwprintf(stderr, L"line %d: %s\n", __LINE__, sqlite3_errmsg16(this->db));
        result = false;
    }
    this->Commit();

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return result;
}

bool neuroeDB::InsertModel(const char* name, const char* type, const char* platform, const char* interpoly, int numOfClasses, const char* modelPath){
    const char* createdOn = getTime();
    sqlite3_stmt * stmt;
    bool result = true;

    sqlite3_prepare_v2(this->db, QUERY_MODEL_INSERT, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, createdOn, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, type, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, platform, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, interpoly, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, numOfClasses);
    sqlite3_bind_text(stmt, 7, modelPath, -1, SQLITE_STATIC);

    this->Begin();
    if(sqlite3_step(stmt) != SQLITE_DONE) {
        fwprintf(stderr, L"line %d: %s\n", __LINE__, sqlite3_errmsg16(this->db));
        result = false;
    }
    this->Commit();

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return result;
}

bool neuroeDB::InsertEvaluationSet(int imageSetId, int modelId, const char* resultCSVPath){
    const char* createdOn = getTime();
    sqlite3_stmt* stmt;
    bool result = true;

    sqlite3_prepare_v2(this->db, QUERY_EVALUATION_SET_INSERT, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, imageSetId);
    sqlite3_bind_int(stmt, 2, modelId);
    sqlite3_bind_text(stmt, 3, resultCSVPath, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, createdOn, -1, SQLITE_STATIC);

    this->Begin();
    if(sqlite3_step(stmt) != SQLITE_DONE) {
        fwprintf(stderr, L"line %d: %s\n", __LINE__, sqlite3_errmsg16(this->db));
        result = false;
    }
    this->Commit();

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return result;
}

bool neuroeDB::InsertImage(int imageSetId, const char* name, const char* imagePath){
    const char* createdOn = getTime();

    sqlite3_stmt* stmt;
    bool result = true;

    sqlite3_prepare_v2(this->db, QUERY_IMAGE_INSERT, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, createdOn, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, imagePath, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, imageSetId);

    this->Begin();
    int rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        printf("Result code: %d\n", rc);
        fwprintf(stderr, L"line %d: %s\n", __LINE__, sqlite3_errmsg16(this->db));
    }
    if(rc == SQLITE_DONE){
        cout << "Insert done for " << name << endl;
    }
    this->Commit();

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return result;
}

bool neuroeDB::InsertResultImage(int evaluationSetId, int imageId, const char* resultImagePath, double inferTime){
    const char* createdOn = getTime();
    sqlite3_stmt* stmt;
    bool result = true;

    sqlite3_prepare_v2(this->db, QUERY_RESULT_IMAGE_INSERT, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, evaluationSetId);
    sqlite3_bind_int(stmt, 2, imageId);
    sqlite3_bind_text(stmt, 3, resultImagePath, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 4, inferTime);
    sqlite3_bind_text(stmt, 5, createdOn, -1, SQLITE_STATIC);

    this->Begin();
    if(sqlite3_step(stmt) != SQLITE_DONE) {
        fwprintf(stderr, L"line %d: %s\n", __LINE__, sqlite3_errmsg16(this->db));
        result = false;
    }
    this->Commit();

    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);

    return result;
}
