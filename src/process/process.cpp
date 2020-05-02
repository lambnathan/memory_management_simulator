/**
 * This file contains implementations for methods in the Process class.
 *
 * You'll need to add code here to make the corresponding tests pass.
 */

#include "process/process.h"

using namespace std;


Process* Process::read_from_input(std::istream& in) {
    size_t num_bytes = 0;

    Page* page;
    vector<Page*> pages;

    while ((page = Page::read_from_input(in)) != nullptr) {
        pages.push_back(page);
        num_bytes += page->size();
    }

    return new Process(num_bytes, pages);
}


size_t Process::size() const
{
    return this->num_bytes;
}


bool Process::is_valid_page(size_t index) const
{
    if(index >= 0 && index < page_table.rows.size()){
        return true;
    }
    return false;
}


size_t Process::get_rss() const
{
    return page_table.get_present_page_count();
}


double Process::get_fault_percent() const
{
    if(memory_accesses != 0){
        return ((double)page_faults / (double)memory_accesses) * 100.0;
    }
    return 0.0;
}
