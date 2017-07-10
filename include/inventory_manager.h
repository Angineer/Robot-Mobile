#ifndef INVENTORY_H
#define INVENTORY_H

#include <functional>
#include <iostream>
#include <netinet/in.h>
#include <queue>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace robot
{

// Forward declarations
class Order;
class Server;

class ItemType
{
    private:
        std::string name;
    public:
        ItemType();
        ItemType(const std::string& name);
        const std::string& get_name() const;
        template <class Archive>
            void serialize(Archive & archive){
                archive( name );
            }
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
        Inventory* inventory;
        Server* server;
        std::queue<Order> queue;
        int status;

        void dispense_item(unsigned int slot, float quantity);
        void get_robot_state();
        void handle_input(char* input, int len);
        void handle_command(char* input, int len);
        void handle_order(char* input, int len);
        void handle_status(char* input, int len);
        void process_order();
    public:
        Manager(Inventory* inventory, Server* server);
        void run();
        void shutdown();
};

}

#endif
