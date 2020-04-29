/**
 * This file contains implementations for methods in the PageTable class.
 *
 * You'll need to add code here to make the corresponding tests pass.
 */

#include "page_table/page_table.h"
#include <limits.h>

using namespace std;


size_t PageTable::get_present_page_count() const {
    int present = 0;
    for(int i = 0; i < rows.size(); i++){
        if(rows[i].present){
            present++;
        }
    }
    return present;
}


size_t PageTable::get_oldest_page() const {
    int index_of_oldest = 0;
    int time = INT_MAX;
    for(int i = 0; i < rows.size(); i++){
        if(rows[i].loaded_at < time && rows[i].present){
            index_of_oldest = i;
            time = rows[i].loaded_at;
        }
    }
    return index_of_oldest;
}


size_t PageTable::get_least_recently_used_page() const {
    int index_of_last_used = 0;
    int time = INT_MAX;
    for(int i = 0; i < rows.size(); i++){
        if(rows[i].last_accessed_at < time && rows[i].present){
            index_of_last_used = i;
            time = rows[i].last_accessed_at;
        }
    }
    return index_of_last_used;
}
