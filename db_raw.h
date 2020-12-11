#ifndef DB_RAW_H
#define DB_RAW_H

#include <fcntl.h>	/* open() and O_XXX flags */
#include <sys/stat.h>	/* S_IXXX flags */
#include <sys/types.h>	/* mode_t */
#include <string>
#include <sstream>

#ifdef linux
#include <unistd.h>	/* close() */

#else
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#include <io.h>

#endif


#ifdef linux
#else
#endif
class db_raw_bottom // окончание блока
{
public:
    uint32_t size=0;
};

class db_raw_header// заголовок блока
{
public:
    uint32_t size=0; // размер всего блока с учетом db_raw_bottom и db_raw_header
    uint64_t id=0; // идентификатор блока заполняется самостоятельно

    uint32_t get_buf_size() // расчитать размер только содержимого
    {
        if(size ==0) return  0;
        return size - (sizeof(db_raw_header)+sizeof (db_raw_bottom));
    }
};


class db_raw_row // блок данных
{
public:
    db_raw_header hdr;
    db_raw_bottom bottom;

    void set(uint64_t _id,uint32_t _size_buf)
    {
        hdr.id=_id;
        hdr.size= _size_buf+sizeof(db_raw_header)+sizeof(db_raw_bottom);
        bottom.size = hdr.size;
    }   
};

class db_raw // управление файлом БД
{    
    friend class db_raw_view;
public:
    db_raw()
    {

    }
    bool open_db(std::string _filename)
    {
        filename = _filename;

        #ifdef linux
            fd = open(filename.c_str(), O_RDWR|O_CREAT, S_IRUSR | S_IWUSR);
        #else
            _sopen_s(&fd, filename.c_str(), O_RDWR | O_CREAT | _O_BINARY, _SH_DENYWR, _S_IREAD | _S_IWRITE);
        #endif


        if(fd >0)
        {
           return true;
        }
        return false;
    }


    void flush()
    {
        #ifdef linux
                fdatasync(fd);
        #else
                _commit(fd);
        #endif
    }
    off_t go_top()
    {
        return seek_db(0,SEEK_SET); // чтение базы с начала файла
    }
    off_t go_bottom()
    {
        return seek_db(0,SEEK_END); // чтение базы с конца файла
    }
    off_t get_pos()
    {
        return seek_db(0,SEEK_CUR); // позиция в файле
    }
    bool add_row(uint64_t _id,std::stringstream& ss )
    {
       ss.seekg(0, std::ios::end );
       add_row(_id,ss.tellg() ,static_cast<void*>(const_cast<char*>( ss.str().c_str() )));
    }
    bool add_row(uint64_t _id,uint32_t _size_buf,void* buf)
    {
        db_raw_row row;
        go_bottom();
        row.set(_id,_size_buf);
        return wr_row(&row,buf);
    }

    uint64_t lenght()
    {
        #ifdef linux


        struct stat64 _fileStatbuff;
        if ((fstat64(fd, &_fileStatbuff) != 0) || (!S_ISREG(_fileStatbuff.st_mode))) {
            return -1;
        }
        #else


        struct _stat64 _fileStatbuff;
        if ((_fstat64(fd, &_fileStatbuff) != 0) ) {
            return -1;
        }

        #endif

        return _fileStatbuff.st_size;
    }

    ~db_raw()
    {
        if( fd > 0 ){
           #ifdef linux
            close(fd);
           #else
            _close(fd);
           #endif
        }

    }

private:

    off_t  seek_db(off_t off,int whence=SEEK_CUR)
    {
        #ifdef linux
            return lseek(fd,off,whence);
        #else
            return _lseek(fd,off,whence);
        #endif
    }

    bool wr_row(db_raw_row* row,void *buf)
    {
        if(wr(&row->hdr,sizeof(db_raw_header)) & wr(buf,row->hdr.get_buf_size()) & wr(&row->bottom,sizeof (db_raw_bottom)) )
        {
            return true;
        }
        return false;
    }

    //записать буфер
    bool wr(void* buf,ssize_t size)
    {
       #ifdef linux
        if(write(fd,buf,size) != size) return false;
       #else
        if(_write(fd,buf,size) != size) return false;
       #endif
       return true;
    }
    // считать буфер
    ssize_t rd(void* buf,ssize_t size)
    {
       #ifdef linux
            return(read(fd,buf,size));
       #else
            return(_read(fd,buf,size));
       #endif
    }

    std::string filename;
    int fd;
};

class db_raw_view // класс просмотра данных , на один класс db_raw может быть несколько db_raw_view , синхронизация потоков отсутствует!
{

public:
   db_raw_view(db_raw* _db,off_t _position): db(_db), position(_position),current_size(0),on_end_row(true)
   {

   }
   db_raw_view(): db(0), position(0),current_size(0),on_end_row(true)
   {

   }
   void init_view(db_raw* _db,off_t _position) // инициализация в произвольной точке
   {
       db=_db;
       position=_position;
       current_size=0;
       on_end_row=true;
   }

   uint32_t get_current_size() {
      return current_size;
   }
   uint32_t get_position() {
      return position;
   }

   ssize_t get_next(db_raw_header* hdr) // получить следующий блок данных db_raw_header
   {
      if(position != db->get_pos())
      {
           db->seek_db(position,SEEK_SET); // это значит что было чтение файла и надо вернуться на место
      }

      if(on_end_row == false && current_size != 0)
      {
          db->seek_db(current_size - sizeof(db_raw_header));
          on_end_row=true;
      }

      ssize_t ret= db->rd(hdr,sizeof (db_raw_header));
      position = db->get_pos();
      if( ret >0)
      {
          on_end_row=false;
          current_size=hdr->size;
      }
      else{
          on_end_row=true;
          current_size=0;
      }
      return ret;
   }

   ssize_t get_prev(db_raw_header* hdr)// получить предыдущий блок данных db_raw_header
   {
      if(position != db->get_pos())
      {
           db->seek_db(position,SEEK_SET); // это значит что было чтение файла и надо вернуться на место
      }

      off_t prev_pos = 0;
      if(on_end_row == false)
      {
          prev_pos= db->seek_db(-1* (static_cast <int>( sizeof(db_raw_bottom)+ sizeof(db_raw_header))) );

      }
      else
      {
          if( current_size == 0 ){
              prev_pos = db->seek_db(-1* static_cast <int>(sizeof(db_raw_bottom)));
          }
          else{
              prev_pos = db->seek_db(-1* (current_size + static_cast <int>(sizeof(db_raw_bottom))));
          }

      }

      on_end_row = true;
      current_size = 0;

      if( prev_pos < 0)
      {
          return 0; // вышли за начало файла
      }

      db_raw_bottom bottom;
      ssize_t ret = db->rd(&bottom,sizeof (db_raw_bottom));
      if( ret < 1) return 0; // ошибка чтение

      prev_pos = db->seek_db(-1* (static_cast <int>(bottom.size)));
      position = db->get_pos(); // установили новую позицию чтения

      if( prev_pos < 0)
      {
          return 0; // вышли за начало файла
      }

      return get_next(hdr);
   }
   ssize_t get_buf(void* buf,db_raw_header& h) // получить буфер данных
   {
       return get_buf(buf,h.get_buf_size());
   }
   ssize_t get_buf(void* buf,uint32_t buf_size) // получить буфер данных
   {
       if(position != db->get_pos())
       {
           db->seek_db(position,SEEK_SET); // это значит что было чтение файла
       }

       if( buf_size > 0 && on_end_row == false && current_size != 0)
       {
           ssize_t ret=db->rd(buf,buf_size);

           db->seek_db(sizeof(db_raw_bottom)); //сдвинулись на конец блока и запомнили позицию
           position = db->get_pos();
           on_end_row=true;

           if(ret < 1){
               current_size=0; // ошибка при чтении
           }

           return  ret;
       }
       return 0;
   }



private:
   db_raw* db;
   off_t position; // текущая позиция в файле необходима для одновременного сканирования  одной базы разными db_raw_view
   uint32_t current_size; // размер текущего блока db_raw_row он может быть считан целиком on_end_row=true или только заголовок
   bool on_end_row;

};

#endif // DB_RAW_H
