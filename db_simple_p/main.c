#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include "db_raw.h"


// Тестовый пример - приложение Питон может открыть только одну БД  одновременно + тема многозадачности  нераскрыта
struct db_raw* db= NULL;
struct db_raw_view* view = NULL;

 
// Открыть базу данных
static PyObject* db_open(PyObject * self, PyObject * args)
{
	
	char* filename;
	int ret=0;

	if (!PyArg_ParseTuple(args, "s", &filename)) {
		return NULL;
	  }
	if( db != NULL) return NULL;

	db= malloc(sizeof (struct db_raw));
    if(db == NULL) return NULL;
    
	ret=db_raw_open_db(db,filename);	
	if( ret  < 1) return Py_BuildValue("i", ret);
	
	printf("open db %s\n",filename) ;
	    
    view = malloc(sizeof (struct db_raw_view));
	if( view == NULL) { 		
		db_raw_close(db);
		free(db);
		db=NULL;
		return NULL;
	}
	db_raw_view_init(view,db,db_raw_go_top(db));
	
	return Py_BuildValue("i", ret);
}

// Закрываем базу данных
static PyObject* db_close(PyObject * self, PyObject * args)
{
	if( db != NULL)
	{
		free(view); view=NULL;
		db_raw_close(db);
		free(db);
		db=NULL;
	}
	return Py_BuildValue("i", 0);;
}


// Перемещаем указатель базы данных на начало файла 
static PyObject* db_init_view(PyObject * self, PyObject * args)
{
	int ret=0;
	if( db != NULL)
	{
		db_raw_view_init(view,db,db_raw_go_top(db));
	}
	return Py_BuildValue("i", 0);
}

// считываем  строку (тест  = всегда строка)
static PyObject* db_get_next(PyObject * self, PyObject * args)
{
	if( db != NULL)
	{		
		struct db_raw_header h={0};
        
         int ret_size=db_raw_view_get_next(view,&h);

        if(ret_size >0)
        {
              //  printf("%iu - %lu\n",h.size,h.id);
               // читать содержимое
               {
                    uint32_t s_tmp=db_raw_header_get_buf_size(&h);
                    if( s_tmp == 0)
                    {
                        return Py_BuildValue("iis",1,0, "NULL"); 
                    }
                    else
                    {
                        char *tmp= calloc(sizeof(char),s_tmp+1 );
                        ret_size=db_raw_view_get_buf(view,tmp,s_tmp); // сохранить обьект в буфер

                        if(ret_size >0)
                        {
                            tmp[s_tmp]=0x0;
                            PyObject* ret_str= Py_BuildValue("iis",1,h.id, tmp); 
                            free(tmp);
                            return ret_str;
                        }
                        else
							free(tmp);
							
							
							
                    }
                }               
                
		}
		
	}
	return Py_BuildValue("iis", 0,0,"");
}


static PyObject* db_append(PyObject * self, PyObject * args)
{
	int id;
	char* test_str;
	if (!PyArg_ParseTuple(args, "is",&id, &test_str)) {
		return NULL;
	  }
	if( db != NULL)
	{
		db_raw_add_row(db, id, strlen(test_str), test_str);
	}
	return Py_BuildValue("i", 0);
}



// Список функций модуля
static PyMethodDef methods[] = {
    {"db_open", db_open, METH_VARARGS, "db_open"}, 
    {"db_close", db_close, METH_VARARGS, "db_close"}, 
    {"db_init_view", db_init_view, METH_VARARGS, "db_init_view"}, 
    {"db_get_next", db_get_next, METH_VARARGS, "db_get_next"}, 
    {"db_append", db_append, METH_VARARGS, "db_append"}, 
    {NULL, NULL, 0, NULL}
};

// Описание модуля
static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT, "_db_simple_p", "DB module", -1, methods
};

// Инициализация модуля
PyMODINIT_FUNC
PyInit__db_simple_p(void) {
    PyObject *mod = PyModule_Create(&module);
    return mod;
}
