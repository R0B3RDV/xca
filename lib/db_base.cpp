#include "db_base.h"


db_base::db_base(DbEnv *dbe, string DBfile, string DB, QListBox *l) 
{
	listView = l;
	dbenv = dbe;
	data = new Db(dbe, 0);
	cerr << "DB:" << DBfile <<"\n";//
	int x;
	if ( x = data->open(DBfile.data(), DB.data(), DB_BTREE, DB_CREATE, 0600)) 
		data->err(x,"DB open");
	updateView();
}


db_base::~db_base()
{
	data->close(0);
}



bool db_base::updateView()
{
	listView->clear();
	Dbc *cursor;
	if (int x = data->cursor(NULL, &cursor, 0))
		data->err(x,"DB new Cursor");
	Dbt *key = new Dbt();
	Dbt *data = new Dbt();
	string desc;
	while (!cursor->get(key, data, DB_NEXT)) {
		desc = (char *)key->get_data();
		listView->insertItem(desc.data());
	}
}


bool db_base::insertPKI(pki_base *pki) 
{
	string desc = pki->getDescription();
	string orig = desc;
	int size=0;
	unsigned char *p;
	p = pki->toData(&size);
	int cnt=0;
	int x = DB_KEYEXIST;
	while (x == DB_KEYEXIST) {
	   Dbt k((void *)desc.data(), desc.length() + 1);
	   Dbt d((void *)p, size);
           cerr << "Size: " << d.get_size() << "\n";
	
	   if (x = data->put(NULL, &k, &d, DB_NOOVERWRITE)) {
		data->err(x,"DB Error put");
		string z ;
		z = (++cnt);
	   	desc = orig + "_" + z ;
	   }
	}
	if (x != DB_KEYEXIST && x != 0) {
	   data->err(x,"DB Error put TINE");
	   //return false;
	}
	OPENSSL_free(p);
	updateView();
	pki->setDescription(desc);
	return true;
}


bool db_base::deletePKI(pki_base *pki) 
{
	string desc = pki->getDescription();
	Dbt k((void *)desc.data(), desc.length() + 1);
	int x = data->del(NULL, &k, 0);
	updateView();
	if (x){
	   data->err(x,"DB Error del");
	   return false;
	}
	return true;
}

bool db_base::updatePKI(pki_base *pki, string desc) 
{
	if (deletePKI(pki)){
	   pki->setDescription(desc);
	   return insertPKI(pki);
	}
	return true;
}


pki_base *db_base::getSelectedPKI(string desc)
{
	if (desc == "" ) return NULL;
	unsigned char *p;
	pki_base *targetPki = NULL;
	Dbt k((void *)desc.data(), desc.length() + 1);
	Dbt d((void *)p, 0);
	int x = data->get(NULL, &k, &d, 0);
	p = (unsigned char *)d.get_data();
	int size = d.get_size();
	if (x) data->err(x,"DB Error get");
	else {
		targetPki = new pki_base();
		targetPki->fromData(p, size);
		targetPki->setDescription(desc);
	}
	return targetPki;
}

pki_base *db_base::getSelectedPKI()
{
	string desc = listView->currentText().latin1();
	return getSelectedPKI(desc);
}
	

pki_base *db_base::findPKI(pki_base *refpki)
{
	unsigned char *p;
	Dbc *cursor;
	if (int x = data->cursor(NULL, &cursor, 0))
		data->err(x,"DB new Cursor");
	Dbt *k = new Dbt();
	Dbt *d = new Dbt();
	string desc;
	pki_base *pki;
	bool found = false;
	while (!cursor->get(k, d, DB_NEXT)) {
		desc = (char *)k->get_data();
		p = (unsigned char *)d->get_data();
		int size = d->get_size();
		pki = new pki_base();
		pki->fromData(p, size);
		pki->setDescription(desc);
		if (refpki->compare(pki)) {
			delete (k);
			delete (d);
			return pki;
		}
		delete(pki);
		
	}
	delete (k);
	delete (d);
	return NULL;
}