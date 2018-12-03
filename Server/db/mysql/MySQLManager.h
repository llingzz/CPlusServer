#pragma once

typedef struct _tagDB_DATA{
	string key;
	std::vector<string> values;
}DB_DATA, *LPDB_DATA;

class CMySQLManager : public CSingle<CMySQLManager>{
public:
	CMySQLManager();
	~CMySQLManager();

private:
	void _connect();
	void _disConnect();

public:
	_ConnectionPtr& GetConnection();
	_RecordsetPtr& GetRecordset();

private:
	_ConnectionPtr m_pConnection;
	_RecordsetPtr m_pRecordset;
};