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
int Slot::get_count_available() const{
    return this->count - this->reserved_count;
}
int Slot::get_count_total() const{
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
void Inventory::add(unsigned int slot, unsigned int count){
    slots[slot].add_items(count);
}
void Inventory::change_slot_type(unsigned int slot, const ItemType* new_type){
    slots[slot].change_type(new_type);
}
unsigned int Inventory::get_count_available(unsigned int slot) const{
    return slots[slot].get_count_available();
}
const ItemType* Inventory::get_type(unsigned int slot) const{
    return slots[slot].get_type();
}
int Inventory::get_num_slots(){
    return slots.size();
}
void Inventory::remove(unsigned int slot, unsigned int count){
    slots[slot].remove_items(count);
}
void Inventory::reserve(unsigned int slot, unsigned int count){
    slots[slot].reserve(count);
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
    command = command.substr(1, std::string::npos);

    std::cout << command << std::endl;

    if (command == "status"){
        std::cout << "Current Status: " + std::to_string(this->status) << std::endl;
    }
}
void Manager::handle_order(char* input, int len){
    std::cout << "Processing order..." << std::endl;

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
    bool valid_order = true;
    int count_slots = this->inventory->get_num_slots();
    int count_items = items.size();
    int inventory_map [count_items];

    for (int i=0; i<count_items; i++){
        inventory_map[i] = -1;
    }

    // Make sure items match inventory
    for (int i=0; i<count_items; i++){
        for (int j=0; j<count_slots; j++){
            if (items[i].get_name() == this->inventory->get_type(j)->get_name()){
                inventory_map[i] = j;
            }
        }
        if (inventory_map[i] < 0){
            std::cout << "Order contains item not in inventory: " + items[i].get_name() + "!" << std::endl;
            valid_order = false;
            break;
        }

        // Make sure we have enough
        if (quantities[i] > inventory->get_count_available(inventory_map[i])){
            std::cout << "Insufficient quantity available: " + items[i].get_name() + "!" << std::endl;
            std::cout << "Asked for " + std::to_string(quantities[i]) + ", but inventory has " + std::to_string(inventory->get_count_available(inventory_map[i])) << std::endl;
            valid_order = false;
            break;
        }
    }

    // If order is valid, reserve it
    if (valid_order){
        for (int i=0; i<count_items; i++){
            this->inventory->reserve(inventory_map[i], quantities[i]);
        }

        // Add order to queue
        this->queue.emplace_back(items, quantities);

        std::cout << "New order placed!" << std::endl;
    }
}
void Manager::handle_status(char* input, int len){
    std::string new_status(input);
    new_status = new_status.substr(1, std::string::npos);

    this->status = stoi(new_status);
}
void Manager::process_queue(){
    std::cout << "Processing queue with size " + std::to_string(this->queue.size()) + "..." << std::endl;
    // First, check current status
    // If robot is occupied, do nothing
    // If robot is ready to go and queue has orders, start processing them
    if (this->status == 0 && this->queue.size() > 0){
        // Pop first order off queue
        Order curr_order = this->queue.front();
        this->queue.pop_front();

        int count_components = curr_order.get_num_components();
        int count_slots = this->inventory->get_num_slots();
        int slot;
        ItemType curr_item;
        int curr_count;

        for (int i=0; i<count_components; i++){
            curr_item = curr_order.get_item(i);
            curr_count = curr_order.get_count(i);
            slot = -1;

            for (int j=0; j<count_slots; j++){
                if (curr_item.get_name() == this->inventory->get_type(j)->get_name()){
                    slot = j;
                    break;
                }
            }

            // Dispense items
            dispense_item(slot, curr_count);

            // Subtract inventory
            this->inventory->remove(slot, curr_count);

            // Unreserve quantities
            this->inventory->reserve(slot, -curr_count);
        }
    }

    std::cout << "After processing queue, the inventory status is:" << std::endl;
    for (int i=0; i<this->inventory->get_num_slots(); i++){
        std::cout << this->inventory->get_type(i)->get_name() + " has " + std::to_string(this->inventory->get_count_available(i)) << std::endl;
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
