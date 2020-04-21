/**
 * This file contains implementations for methods in the VirtualAddress class.
 *
 * You'll need to add code here to make the corresponding tests pass.
 */

#include "virtual_address/virtual_address.h"

using namespace std;


VirtualAddress VirtualAddress::from_string(int process_id, string address) {

    size_t address_binary = bitset<32>(address).to_ulong();
    size_t page_ = (address_binary & PAGE_BITMASK) >> OFFSET_BITS;
    size_t offset_ = address_binary & OFFSET_BITMASK;

    return VirtualAddress(process_id, page_, offset_);
}

string VirtualAddress::to_string() const {
    return bitset<16>((page << OFFSET_BITS) | offset).to_string();
}

ostream& operator <<(ostream& out, const VirtualAddress& address) {
    out << "PID " << address.process_id << " @ " << address.to_string()
        << " [page: " << address.page << "; offset: " << address.offset << "]";
    return out;
}
