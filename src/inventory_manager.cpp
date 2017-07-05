#include "../include/inventory_manager.h"
#include "../include/communication.h"

#define MESSAGE_SIZE 8
#define UI_SOCKET 5000
#define DP_SOCKET 5001

namespace robot
{

// Create server
robot::Server server("localhost", UI_SOCKET);

// Item type
ItemType::ItemType(const std::string& name){
    this->name = name;
}
const std::string& ItemType::get_name() const{
    return this->name;
}

// Slot
Slot::Slot(){
    this->type = NULL;
    this->count = 0;
}
void Slot::change_type(const ItemType* new_type){
    if (this->type == NULL){ // If slot has not been ininitialized
        std::cout << "Assigning item type " + new_type->get_name() << std::endl;
        this->type = new_type;
    }
    else if (this->count == 0){
        std::cout << "Changing item type from " + this->type->get_name() + " to " + new_type->get_name() << std::endl;
        this->type = new_type;
    }
    else{
        std::cout << "Cannot change type when there are items remaining!" << std::endl;
    }
}
const ItemType* Slot::get_type() const{
    return this->type;
}
int Slot::get_count() const{
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
Inventory::Inventory(int count_slots){
    slots.resize(count_slots);
}
void Inventory::add_item(unsigned int slot, const ItemType* item, unsigned int count){
    const ItemType* slot_type = slots[slot].get_type();

    if (item == slot_type){
        slots[slot].add_items(count);
    }
    else{
        std::cout << "This slot isn't of that item type!" << std::endl;
    }
}
void Inventory::change_slot_type(unsigned int slot, const ItemType* new_type){
    slots[slot].change_type(new_type);
}
unsigned int Inventory::get_count(unsigned int slot) const{
    return slots[slot].get_count();
}
const ItemType* Inventory::get_item(unsigned int slot) const{
    return slots[slot].get_type();
}
void Inventory::remove_item(unsigned int slot, const ItemType* item, unsigned int count){
    const ItemType* slot_type = slots[slot].get_type();

    if (item == slot_type){
        slots[slot].remove_items(count);
    }
    else{
        std::cout << "This slot isn't of that item type!" << std::endl;
    }
}

// Manager
Manager::Manager(Inventory* inventory){
    // Set up inventory
    this->inventory = inventory;
}
void Manager::dispense_item(unsigned int slot, float quantity){
    std::cout << "Dispensing item!" << std::endl;
}
void Manager::handle_input(char* input){
    std::string str_input(input);
    std::string content = str_input.substr(1, std::string::npos);

    // Strip out newlines
    size_t newline = content.find('\n');
    if (newline > 0){
        content.pop_back();
    }

    if (input[0] == 'c'){
        // Command
        std::cout << "Command received" << std::endl;
        handle_command(content);
    }
    else if (input[0] == 'o'){
        // Order
        handle_order(content);
    }
    else if (input[0] == 's'){
        // Status
        handle_status(content);
    }
}
void Manager::handle_command(std::string command){
    if (command == "status"){
        std::cout << "Current Status: Great!" << std::endl;
    }
}
void Manager::handle_order(std::string order){
    //foo
}
void Manager::handle_status(std::string status){
    //foo
}
void Manager::run(){

    ItemType apple("apple");

    Order order(apple, 2);

    std::cout << order.get_serial() << std::endl;

    // Create callback function that can be passed as argument
    std::function<void(char*)> callback_func(std::bind(&Manager::handle_input, this, std::placeholders::_1));

    // Run server and process callbacks
    server.serve(callback_func);
}
void Manager::shutdown(){
    server.shutdown();
}

}
