#include "../include/communication.h"
#include "../include/inventory_manager.h"

#define MESSAGE_SIZE 8
#define UI_SOCKET 5000
#define DP_SOCKET 5001

// Items that will be available for selection
robot::ItemType apple("apple");
robot::ItemType cracker("cracker");
robot::ItemType granola("granola bar");
robot::ItemType gummy("gummy bears");

// Create server
robot::Server server("localhost", UI_SOCKET);

// Set up initial inventory with empty slots
const int inventory_size = 3;
robot::Inventory base(inventory_size);

// Set up the manager
robot::Manager manager(&base, &server);

void check_slot_quant(unsigned char slot){
    int quant = base.get_count_available(slot);
    std::string message = "Slot "+ std::to_string(slot) +" now has this many:";
    std::cout << message << std::endl;
    std::cout << quant << std::endl;
}

void shutdown(int signum){
    manager.shutdown();
    std::cout << "\nStopping Inventory Manager" << std::endl;
    exit(0);
}

int main()
{
    std::cout << "Starting Inventory Manager" << std::endl;

    // Set up slots with assigned items
    base.change_slot_type(0, &apple);
    base.change_slot_type(1, &cracker);
    base.change_slot_type(2, &granola);

    for (int i=0; i<3; i++){
        base.add(i, 5);
    }

    // Kill manager gracefully on ctrl+c
    std::signal(SIGINT, shutdown);

    // Run forever
    manager.run();
}
