#include <iostream>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace robot
{

class ItemType
{
    private:
        std::string name;
    public:
        ItemType(const std::string& name);
        const std::string& get_name() const;
};

class Slot
{
    private:
        const ItemType* type;
        int count;
    public:
        Slot();
        void change_type(const ItemType* new_type);
        const ItemType* get_type() const;
        int get_count() const;
        void add_items(int quantity);
        void remove_items(int quantity);
};

class Inventory
{
    private:
        std::vector<Slot> slots;
    public:
        Inventory(int count_slots);
        void add_item(unsigned int slot, const ItemType* item, unsigned int count);
        void change_slot_type(unsigned int slot, const ItemType* new_type);
        unsigned int get_count(unsigned int slot) const;
        const ItemType* get_item(unsigned int slot) const;
        void remove_item(unsigned int slot, const ItemType* item, unsigned int count);
};

class Manager
{
    private:
        //Socket communication object
        struct sockaddr_in serv_addr, cli_addr;
        Inventory* inventory;

        void dispense_item(unsigned int slot, float quantity);
        void get_robot_state();
        void handle_user_input(std::string command);
        void handle_dispatch_input();
        void process_order();
    public:
        Manager(Inventory* inventory);
        void run();
};

}
