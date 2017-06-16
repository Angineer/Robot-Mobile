#include <iostream>

namespace robot
{

class ItemType
{
	private:
		std::string name;
	public:	
		ItemType(std::string name);
		std::string get_name();
};

class Slot
{
	private:
		ItemType * type;
		int count;
	public:
		Slot(ItemType * type);
		
		void change_type(ItemType * new_type);
		ItemType * get_type();
		int get_count();
		void add_items(int quantity);
		void remove_items(int quantity);
};

class Inventory
{
	private:
		Slot * slots[];

	public:
		Inventory(int count_slots, ItemType * item_types[]);
		void add_item(unsigned char slot, ItemType * item, unsigned char count);
		void change_slot_type(unsigned char slot, ItemType * new_type);
		unsigned char get_count(unsigned char slot);
		ItemType * get_item(unsigned char slot);
		void remove_item(unsigned char slot, ItemType * item, unsigned char count);
};

class Manager
{
    private:
        //foo
        //Socket communication object
    public:
        void dispense_item(unsigned char slot, float quantity);
         
};

}
