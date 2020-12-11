

#include <iostream>
#include <string>
#include <fstream>

// include headers that implement a archive in simple text format

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include "db_raw.h"



using namespace std;
class gps_position
{
private:
    friend class boost::serialization::access;
    // When the class Archive corresponds to an output archive, the
    // & operator is defined similar to <<.  Likewise, when the class Archive
    // is a type of input archive the & operator is defined similar to >>.
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & degrees;
        ar & minutes;
        ar & seconds;
        ar & test;
    }
public:
    int degrees;
    int minutes;
    float seconds;
    string test;
    gps_position(){};
    gps_position(int d, int m, float s) :
        degrees(d), minutes(m), seconds(s),test(260,65)
    {
    }
};

int main() {


    const gps_position g(35, 59, 24.567f);
    db_raw db1;
    db1.open_db("test.bin");

    {
        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        // write class instance to archive
        oa << g;
        db1.add_row(0,ss); // добавить запись db_raw_row
    }
    // ... some time later restore the class instance to its orginal state
    {
        db_raw_view view(&db1,db1.go_bottom());
        db_raw_header h;
        ssize_t ret_size=0;
        ret_size=view.get_prev(&h);
        if( ret_size > 0)
        {
            char buf[h.get_buf_size()];
            ret_size=view.get_buf(buf,h); // сохранить обьект в буфер
            if(ret_size >0)
            {
                std::istringstream i_ss(buf);
                boost::archive::text_iarchive ia(i_ss);
                // read class state from archive
                gps_position newg;
                ia >> newg;
                cout << newg.degrees << "-" << newg.minutes << "-" <<  newg.seconds << "-" <<  newg.test << endl;

            }
        }
    }

    return 0;
}

