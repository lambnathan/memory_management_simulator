/**
 * This file contains implementations for the methods defined in the Simulation
 * class.
 *
 * You'll probably spend a lot of your time here.
 */

#include "simulation/simulation.h"
#include "flag_parser/flag_parser.h"
#include <stdexcept>

Simulation::Simulation(FlagOptions& flags)
{
    this->flags = flags;
    this->frames.reserve(this->NUM_FRAMES);
    
}

void Simulation::run() {
    //initialize free frames
    for(int i = 0; i < 512; i++){
        free_frames.push_back(i);
    }
    //access memory
    for (auto virtual_address : this->virtual_addresses) {
        this->perform_memory_access(virtual_address);
        this->time++; //increment time
    }

    this->print_summary();
}

char Simulation::perform_memory_access(const VirtualAddress& virtual_address) {
    if (this->flags.verbose) {
        std::cout << virtual_address << std::endl;
    }
    Process* proc = processes[virtual_address.process_id]; //get the corresponding process
    proc->memory_accesses++;
    //check if the process page will cause segault
    if(!proc->is_valid_page(virtual_address.page)){
        //page segfault
        std::cout << "SEGFAULT - INVALID PAGE" << std::endl;
        exit(0);
    }

    //check if the referenced page is already in memory
    if(proc->page_table.rows[virtual_address.page].present){
        //is in memory
        if(this->flags.verbose){
            std::cout << "\t-> IN MEMORY" << std::endl;
        }
        proc->page_table.rows[virtual_address.page].last_accessed_at = time;
        //do nothing else?
    }
    else{
        handle_page_fault(proc, virtual_address.page); //not in memory, handle page fault
    }
    
    //create physical address and print out physical address
    PhysicalAddress p(proc->page_table.rows[virtual_address.page].frame, virtual_address.offset);
    if(this->flags.verbose){
        std::cout << "\t-> physical address " << p << std::endl;
        //check for offset segfault
        if(!proc->pages[virtual_address.page]->is_valid_offset(virtual_address.offset)){
            std::cout << "SEGFAULT - INVALID OFFSET" << std::endl;
            exit(0);
        }
        std::cout << "\t-> RSS: " << proc->get_rss() << std::endl << std::endl;
    }
    return 0;
}

//when page is not in main memory
void Simulation::handle_page_fault(Process* process, size_t page) {
    page_faults++;
    process->page_faults++;
    if(this->flags.verbose){
        std::cout << "\t-> PAGE FAULT" << std::endl;
    }
    //give that page the first unused frame (if it exists)
    if(process->get_rss() < this->flags.max_frames){
        //give it first free frame
        int frame_index = free_frames.front();
        free_frames.remove(frame_index); //remove; is no longer free
        Frame f;
        f.set_page(process, page);
        frames.push_back(f); 
        process->page_table.rows[page].frame = frame_index;
        process->page_table.rows[page].present = true;
        process->page_table.rows[page].loaded_at = time;
        process->page_table.rows[page].last_accessed_at = time;
        return;
    }
    //process has used all of its allowed frames
    //put the page in memory, handle differently based on FIFO/LRU
    if(this->flags.strategy == ReplacementStrategy::FIFO){
        //pages removed in round robin style
        //page that has been in memory the longest is replaced

        int page_index = process->page_table.get_oldest_page();
        process->page_table.rows[page_index].present = false;
        int frame_index = process->page_table.rows[page_index].frame;
        Frame f;
        f.set_page(process, page);
        frames.push_back(f);
        process->page_table.rows[page].frame = frame_index;
        process->page_table.rows[page].present = true;
        process->page_table.rows[page].loaded_at = time;
        process->page_table.rows[page].last_accessed_at = time;
    }
    else{ //LRU strategy
        //replace the page that has not been referenced for the longest time
        int page_index = process->page_table.get_least_recently_used_page();
        process->page_table.rows[page_index].present = false;
        int frame_index = process->page_table.rows[page_index].frame;
        Frame f;
        f.set_page(process, page);
        frames.push_back(f);
        process->page_table.rows[page].frame = frame_index;
        process->page_table.rows[page].present = true;
        process->page_table.rows[page].loaded_at = time;
        process->page_table.rows[page].last_accessed_at = time;
    }

}

void Simulation::print_summary() {
    if (!this->flags.csv) {
        boost::format process_fmt(
            "Process %3d:  "
            "ACCESSES: %-6lu "
            "FAULTS: %-6lu "
            "FAULT RATE: %-8.2f "
            "RSS: %-6lu\n");

        for (auto entry : this->processes) {
            std::cout << process_fmt
                % entry.first
                % entry.second->memory_accesses
                % entry.second->page_faults
                % entry.second->get_fault_percent()
                % entry.second->get_rss();
        }

        // Print statistics.
        boost::format summary_fmt(
            "\n%-25s %12lu\n"
            "%-25s %12lu\n"
            "%-25s %12lu\n");

        std::cout << summary_fmt
            % "Total memory accesses:"
            % this->virtual_addresses.size()
            % "Total page faults:"
            % this->page_faults
            % "Free frames remaining:"
            % this->free_frames.size();
    }

    if (this->flags.csv) {
        boost::format process_fmt(
            "%d,"
            "%lu,"
            "%lu,"
            "%.2f,"
            "%lu\n");

        for (auto entry : processes) {
            std::cout << process_fmt
                % entry.first
                % entry.second->memory_accesses
                % entry.second->page_faults
                % entry.second->get_fault_percent()
                % entry.second->get_rss();
        }

        // Print statistics.
        boost::format summary_fmt(
            "%lu,,,,\n"
            "%lu,,,,\n"
            "%lu,,,,\n");

        std::cout << summary_fmt
            % this->virtual_addresses.size()
            % this->page_faults
            % this->free_frames.size();
    }
}

int Simulation::read_processes(std::istream& simulation_file) {
    int num_processes;
    simulation_file >> num_processes;

    for (int i = 0; i < num_processes; ++i) {
        int pid;
        std::string process_image_path;

        simulation_file >> pid >> process_image_path;

        std::ifstream proc_img_file(process_image_path);

        if (!proc_img_file) {
            std::cerr << "Unable to read file for PID " << pid << ": " << process_image_path << std::endl;
            return 1;
        }
        this->processes[pid] = Process::read_from_input(proc_img_file);
    }
    return 0;
}

int Simulation::read_addresses(std::istream& simulation_file) {
    int pid;
    std::string virtual_address;

    try {
        while (simulation_file >> pid >> virtual_address) {
            this->virtual_addresses.push_back(VirtualAddress::from_string(pid, virtual_address));
        }
    } catch (const std::exception& except) {
        std::cerr << "Error reading virtual addresses." << std::endl;
        std::cerr << except.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Error reading virtual addresses." << std::endl;
        return 1;
    }
    return 0;
}

int Simulation::read_simulation_file() {
    std::ifstream simulation_file(this->flags.filename);
    // this->simulation_file.open(this->flags.filename);

    if (!simulation_file) {
        std::cerr << "Unable to open file: " << this->flags.filename << std::endl;
        return -1;
    }
    int error = 0;
    error = this->read_processes(simulation_file);

    if (error) {
        std::cerr << "Error reading processes. Exit: " << error << std::endl;
        return error;
    }

    error = this->read_addresses(simulation_file);

    if (error) {
        std::cerr << "Error reading addresses." << std::endl;
        return error;
    }

    if (this->flags.file_verbose) {
        for (auto entry: this->processes) {
            std::cout << "Process " << entry.first << ": Size: " << entry.second->size() << std::endl;
        }

        for (auto entry : this->virtual_addresses) {
            std::cout << entry << std::endl;
        }
    }

    return 0;
}
