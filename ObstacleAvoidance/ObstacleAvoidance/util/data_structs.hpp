#pragma once

namespace strcts {
	template <typename T, unsigned int linked_array_length = 32>
	class linked_arraylist {
		struct linked_array {
			T items[linked_array_length];
			linked_array* next;
			linked_array() = default;
			linked_array(const linked_array& array) {
				for (int i = 0; i < linked_array_length; i++)
					items[i] = array.items[i];
				if (array.next) next = new linked_array(*array.next);
				else next = 0;
			}
		};

		linked_array *first_array, *current_array, *last_array;
		unsigned int len, last_array_len, current_array_pos;
	public:
		void reset() {
			current_array = first_array;
			current_array_pos = 0;
		}
		linked_arraylist() {
			first_array = new linked_array();
			last_array = first_array;
			len = 0;
			last_array_len = 0;
			reset();
		}
		linked_arraylist& operator =(const linked_arraylist& list) {
			len = list.len;
			last_array_len = list.last_array_len;
			first_array = new linked_array(*list.first_array);
			last_array = first_array;
			while (last_array->next) last_array = last_array->next;
			reset();
			return *this;
		}
		linked_arraylist(const linked_arraylist& list) {
			*this = list;
		}
		inline void add(T item) {
			if (last_array_len == linked_array_length) {
				last_array = (last_array->next = new linked_array());
				last_array_len = 0;
			}
			last_array->items[last_array_len++] = item;
			len++;
		}
		inline T next() {
			if (current_array_pos == linked_array_length) {
				current_array = current_array->next;
				current_array_pos = 0;
			}
			return current_array->items[current_array_pos++];
		}
		unsigned int get_length() {
			return len;
		}
		~linked_arraylist() {
			linked_array *array1 = first_array, *array2;
			while (array1) {
				array2 = array1->next;
				delete array1;
				array1 = array2;
			}
		}
	};
}