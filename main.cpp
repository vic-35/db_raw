


#include <iostream>
#include <string>

#include "db_raw.h"
using namespace std;


int main()
{

    db_raw db1;
    db1.open_db("test.bin");

    string x="read() function shall attempt to read nbyte bytes from the file associated with the open file descriptor"+std::to_string( db1.lenght());



    db_raw_row row;
    row.set_buf(db1.lenght(),x.length()+1,static_cast<void*>(const_cast<char*>( x.c_str() ))); // загрузка данных db_raw_row, для примера поле id заполняем размером файла
    // при загрузке строк не забывать про завершающий 0
    db1.add_row(&row); // добавить запись db_raw_row

    db_raw_row row1;
    row1.set_buf(db1.lenght(),0,nullptr); // загрузка данных db_raw_row, для примера поле id заполняем размером файла
    // нулевой буфер
    db1.add_row(&row1); // запись db_raw_row


    // чтение базы с начала файла
    db_raw_view view(&db1,db1.go_top());
    ssize_t ret_size=0;

    do{

        db_raw_header h;
        ret_size=view.get_next(&h);//  db1.rd(&h,sizeof (h));
        if(ret_size >0)
        {
           cout << h.size << "-" << h.id << endl;

           // читать содержимое           
           {
               uint32_t s_tmp=h.get_buf_size();
               if( s_tmp == 0)
               {
                   cout << "NULL" << endl;
                   db1.go_bottom();
               }
               else
               {
                   char *tmp= new char[s_tmp]();

                   ret_size=view.get_buf(tmp,s_tmp); // сохранить обьект в буфер

                   if(ret_size >0)
                   {
                       string tmp_s;
                       tmp_s.append(tmp);
                       cout << tmp_s << endl;
                   }
                   delete [] tmp;
               }
           }
        }
    }while(ret_size >0);


    cout << "--------------------------------" <<  endl;


// вывод  заголовков в обратном порядке
    view.init_view(&db1,db1.go_bottom());
    do{
        db_raw_header h;
        ret_size=view.get_prev(&h);

            if(ret_size >0)
            {
                cout << h.size << "-" << h.id << endl;
            }


        }while(ret_size >0);




    cout << "end." << endl;
    return 0;
}
