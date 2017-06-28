#include "../include/InventoryManager.h"

// Items that will be available for selection
robot::ItemType apple("apple");
robot::ItemType cracker("cracker");
robot::ItemType granola("granola bar");
robot::ItemType gummy("gummy bears");

// Set up initial inventory with empty slots
const int inventory_size = 3;
robot::Inventory base(inventory_size);

// Set up the manager
robot::Manager manager(&base);

void check_slot_quant(unsigned char slot){
    int quant = base.get_count(slot);
    std::string message = "Slot "+ std::to_string(slot) +" now has this many:";
    std::cout << message << std::endl;
    std::cout << quant << std::endl;
}

int main()
{
    std::cout << "Starting Inventory Manager" << std::endl;

    // Set up slots with assigned items
    base.change_slot_type(0, &apple);
    base.change_slot_type(1, &cracker);
    base.change_slot_type(2, &granola);

    // Run forever
    manager.run();
}
