#include "../include/inventory_manager.h"
#include "../include/communication.h"

namespace robot
{

// Item type
ItemType::ItemType(){
    // Default constructor for Cereal
}
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
    this->reserved_count = 0;
}
void Slot::add_items(int quantity){
    this->count += quantity;
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
void Slot::remove_items(int quantity){
    if (this->count >= quantity){
        this->count -= quantity;
    }
    else{
        std::cout << "Not enough items in slot!" << std::endl;
    }
}
void Slot::reserve(int quantity){
    if (this->count - this->reserved_count >= quantity){
        this->reserved_count += quantity;
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
int Inventory::get_slot_count(){
    return slots.size();
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
void Inventory::reserve(unsigned int slot, const ItemType* item, unsigned int count){
    const ItemType* slot_type = slots[slot].get_type();

    if (item == slot_type){
        slots[slot].reserve(count);
    }
    else{
        std::cout << "This slot isn't of that item type!" << std::endl;
    }
}

// Manager
Manager::Manager(Inventory* inventory, Server* server){
    // Set up inventory
    this->inventory = inventory;
    this->server = server;
}
void Manager::dispense_item(unsigned int slot, float quantity){
    std::cout << "Dispensing item!" << std::endl;
}
void Manager::handle_input(char* input, int len){
    if (len > 0){

        if (input[0] == 'c'){
            // Command
            std::cout << "Command received" << std::endl;
            handle_command(input, len);
        }
        else if (input[0] == 'o'){
            // Order
            std::cout << "Order received" << std::endl;
            handle_order(input, len);
        }
        else if (input[0] == 's'){
            // Status
            std::cout << "Status update" << std::endl;
            handle_status(input, len);
        }

        process_queue();
    }
}
void Manager::handle_command(char* input, int len){
    std::string command(input);
    if (command == "status"){
        std::cout << "Current Status: Great!" << std::endl;
    }
}
void Manager::handle_order(char* input, int len){
    // Read in new order
    std::stringstream ss;
    std::vector<ItemType> items;
    std::vector<int> quantities;

    //std::cout << len << std::endl;

    for (int i = 1; i < len; i++){
        ss << input[i];
    }

    {
        cereal::BinaryInputArchive iarchive(ss); // Create an input archive

        iarchive(items, quantities); // Read the data from the archive
    }

    // Double check order validity
    int count_slots = this->inventory->get_slot_count();
    int count_items = items.size();
    int inventory_map [count_items];

    // Make sure items match inventory
    for (int i=0; i<count_items; i++){
        for (int j=0; j<count_slots; j++){
            if (items[i].get_name() == this->inventory->get_item(j)->get_name()){
                inventory_map[i] = j;
            }
        }

    // Make sure we have enough
    }

    // If order is valid, reserve it
    for (int i=0; i<count_items; i++){

        this->inventory->reserve(inventory_map[i], &items[i], quantities[i]);
    }

    // Add order to queue
    this->queue.emplace_back(items, quantities);

    std::cout << "New order placed!" << std::endl;
}
void Manager::handle_status(char* input, int len){
    //foo
}
void Manager::process_queue(){
    std::cout << "Processing queue..." << std::endl;
    // First, check current status
    // If robot is occupied, do nothing
    // If robot is ready to go and queue has orders, start processing them
    if (this->status == 0 && this->queue.size() > 0){
        // Pop first order off queue
        Order curr_order = this->queue.front();
        this->queue.pop_front();

        for (int i=0; i<3; i++){
            std::cout << curr_order.get_item(i).get_name() << std::endl;
            std::cout << curr_order.get_count(i) << std::endl;
        }

        // Dispense items

        // Subtract inventory

        // Unreserve quantities
    }
}
void Manager::run(){

    // Create callback function that can be passed as argument
    std::function<void(char*, int)> callback_func(std::bind(&Manager::handle_input, this, std::placeholders::_1, std::placeholders::_2));

    // Run server and process callbacks
    server->serve(callback_func);
}
void Manager::shutdown(){
    this->server->shutdown();
}

}
