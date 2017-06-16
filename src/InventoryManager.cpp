#include <iostream>
#include "../include/InventoryManager.h"

namespace robot
{

// Item type
ItemType::ItemType(std::string name){
	this->name = name;
}
std::string ItemType::get_name(){
	return this->name;
}

// Slot
Slot::Slot(ItemType* type){
	this->type = type;
	this->count = 0;
}
void Slot::change_type(ItemType * new_type){
    if (this->count == 0){
        std::cout << "Changing item type from " + this->type->get_name() + " to " + new_type->get_name() << std::endl;
        this->type = new_type;
    }
    else{
        std::cout << "Cannot change type when there are items remaining!" << std::endl;
    }
}
ItemType * Slot::get_type(){
	return this->type;
}
int Slot::get_count(){
    return this->count;
}
void Slot::add_items(int quantity){
    this->count += quantity;
}
void Slot::remove_items(int quantity){
    if (this->count >= quantity){
        this->count -= quantity;
    }
    else{
        std::cout << "Not enough items in slot!" << std::endl;
    }
}

// Inventory
Inventory::Inventory(int count_slots, ItemType * item_types[]){
	//slots[count_slots] = new Slot[count_slots];
	for(int i=0; i<count_slots; i++){
		slots[i] = new Slot(item_types[i]);
	}
}
void Inventory::add_item(unsigned char slot, ItemType * item, unsigned char count){
    ItemType * slot_type = slots[slot]->get_type();
    
    if (item == slot_type){
        slots[slot]->add_items(count);
    }
    else{
        std::cout << "This slot isn't of that item type!" << std::endl;
    }
}
void Inventory::change_slot_type(unsigned char slot, ItemType * new_type){
    slots[slot]->change_type(new_type);
}
unsigned char Inventory::get_count(unsigned char slot){
    return slots[slot]->get_count();
}
ItemType * Inventory::get_item(unsigned char slot){
	return slots[slot]->get_type();
}
void Inventory::remove_item(unsigned char slot, ItemType * item, unsigned char count){
    ItemType * slot_type = slots[slot]->get_type();
    
    if (item == slot_type){
        slots[slot]->remove_items(count);
    }
    else{
        std::cout << "This slot isn't of that item type!" << std::endl;
    }
}

}
