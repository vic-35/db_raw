#ifndef DB_RAW_H
#define DB_RAW_H

#include <fcntl.h>	/* open() and O_XXX flags */
#include <sys/stat.h>	/* S_IXXX flags */
#include <sys/types.h>	/* mode_t */

#include <stdint.h>
#include <unistd.h>	/* close() */
#include <linux/limits.h>
#include <string.h>


struct db_raw_bottom // окончание блока
{
    uint32_t size;
};

struct db_raw_header// заголовок блока
{
    uint32_t size; // размер всего блока с учетом db_raw_bottom и db_raw_header
    uint64_t id; // идентификатор блока заполняется самостоятельно
};

uint32_t db_raw_header_get_buf_size(struct db_raw_header* p_db_raw_header) // расчитать размер только содержимого
    {
        if(p_db_raw_header->size ==0) return  0;
        return p_db_raw_header->size - (sizeof(struct db_raw_header)+sizeof (struct db_raw_bottom));
    }

struct db_raw_row // блок данных
{
    struct db_raw_header hdr;
    struct db_raw_bottom bottom;
};

void db_raw_row_set(struct db_raw_row* p_db_raw_row, uint64_t _id,uint32_t _size_buf)
{
    p_db_raw_row->hdr.id=_id;
    p_db_raw_row->hdr.size= _size_buf+sizeof(struct db_raw_header)+sizeof(struct db_raw_bottom);
    p_db_raw_row->bottom.size = p_db_raw_row->hdr.size;
}

struct db_raw // управление файлом БД
{
    char filename[PATH_MAX];
    int fd;
};

int db_raw_open_db(struct db_raw* p_db, char* _filename)
    {
        strncpy(p_db->filename,_filename,PATH_MAX);

        p_db->fd = open(p_db->filename, O_RDWR|O_CREAT, S_IRUSR | S_IWUSR);

        if(p_db->fd >0)
        {
           return 1;
        }
        return 0;
    }


void db_raw_flush(struct db_raw* p_db)
    {
        fdatasync(p_db->fd);
    }

off_t db_raw_seek_db(struct db_raw* p_db, off_t off,int whence )
    {
            return lseek(p_db->fd,off,whence);
    }

off_t db_raw_go_top(struct db_raw* p_db)
    {
        return db_raw_seek_db(p_db,0,SEEK_SET); // чтение базы с начала файла
    }
off_t db_raw_go_bottom(struct db_raw* p_db)
    {
        return db_raw_seek_db(p_db,0,SEEK_END); // чтение базы с конца файла
    }
off_t db_raw_get_pos(struct db_raw* p_db)
    {
        return db_raw_seek_db(p_db,0,SEEK_CUR); // позиция в файле
    }

//записать буфер
int db_raw_wr(struct db_raw* p_db,void* buf,ssize_t size)
{
    if(write(p_db->fd,buf,size) != size) return 0;

   return 1;
}

int db_raw_wr_row(struct db_raw* p_db,struct db_raw_row* row,void *buf)
{
//    if( db_raw_wr(p_db,&row->hdr,sizeof(struct db_raw_header)) & db_raw_wr(p_db,buf,db_raw_header_get_buf_size(&row->hdr)) & db_raw_wr(p_db, &row->bottom,sizeof (struct db_raw_bottom)) )

    if( db_raw_wr(p_db,&row->hdr,sizeof(struct db_raw_header)) & db_raw_wr(p_db,buf,db_raw_header_get_buf_size(&row->hdr)) & db_raw_wr(p_db, &row->bottom,sizeof (struct db_raw_bottom)) )
    {
        return 1;
    }
    return 0;
}


// считать буфер
ssize_t db_raw_rd(struct db_raw* p_db,void* buf,ssize_t size)
{
    return(read(p_db->fd,buf,size));
}

int db_raw_add_row(struct db_raw* p_db,uint64_t _id,uint32_t _size_buf,void* buf)
    {
        struct db_raw_row row={0};
        db_raw_go_bottom(p_db);
        db_raw_row_set(&row,_id,_size_buf);
        return db_raw_wr_row(p_db, &row, buf);
    }

uint64_t db_raw_lenght(struct db_raw* p_db)
    {
        struct stat _fileStatbuff;
        if ((fstat(p_db->fd, &_fileStatbuff) != 0) || (!S_ISREG(_fileStatbuff.st_mode))) {
            return -1;
        }
        return _fileStatbuff.st_size;
    }

void db_raw_close(struct db_raw* p_db)
    {
        if( p_db->fd > 0 ){
            close(p_db->fd);
        }
    }

struct db_raw_view // класс просмотра данных , на один класс db_raw может быть несколько db_raw_view , синхронизация потоков отсутствует!
{
   struct db_raw* db;
   off_t position; // текущая позиция в файле необходима для одновременного сканирования  одной базы разными db_raw_view
   uint32_t current_size; // размер текущего блока db_raw_row он может быть считан целиком on_end_row=true или только заголовок
   int on_end_row;

};

void db_raw_view_init(struct db_raw_view* p_view,  struct db_raw* p_db, off_t _position) // инициализация в произвольной точке
   {
       p_view->db=p_db;
       p_view->position=_position;
       p_view->current_size=0;
       p_view->on_end_row=1;
   }

ssize_t db_raw_view_get_next(struct db_raw_view* p_view, struct db_raw_header* hdr) // получить следующий блок данных db_raw_header
   {
      if(p_view->position != db_raw_get_pos(p_view->db))
      {
           db_raw_seek_db(p_view->db, p_view->position,SEEK_SET); // это значит что было чтение файла и надо вернуться на место
      }

      if(p_view->on_end_row == 0 && p_view->current_size != 0)
      {
          db_raw_seek_db( p_view->db, p_view->current_size - sizeof(struct db_raw_header), SEEK_CUR);
          p_view->on_end_row=1;
      }

      ssize_t ret= db_raw_rd(p_view->db,hdr,sizeof (struct db_raw_header));
      p_view->position = db_raw_get_pos(p_view->db);

      if( ret >0)
      {
          p_view->on_end_row=0;
          p_view->current_size=hdr->size;
      }
      else{
          p_view->on_end_row=1;
          p_view->current_size=0;
      }
      return ret;
   }

   ssize_t db_raw_view_get_buf(struct db_raw_view* p_view,void* buf,uint32_t buf_size) // получить буфер данных
   {
       if(p_view->position != db_raw_get_pos(p_view->db))
       {
           db_raw_seek_db(p_view->db, p_view->position,SEEK_SET); // это значит что было чтение файла
       }

       if( buf_size > 0 && p_view->on_end_row == 0 && p_view->current_size != 0)
       {
           ssize_t ret=db_raw_rd(p_view->db,buf,buf_size);

           db_raw_seek_db(p_view->db,sizeof(struct db_raw_bottom),SEEK_CUR); //сдвинулись на конец блока и запомнили позицию
           p_view->position = db_raw_get_pos(p_view->db);
           p_view->on_end_row=1;

           if(ret < 1){
               p_view->current_size=0; // ошибка при чтении
           }
           return  ret;
       }
       return 0;
   }
   ssize_t db_raw_view_get_buf_h(struct db_raw_view* p_view,void* buf,struct db_raw_header* h) // получить буфер данных
   {
       return db_raw_view_get_buf(p_view,buf,db_raw_header_get_buf_size(h));
   }




#endif // DB_RAW_H
