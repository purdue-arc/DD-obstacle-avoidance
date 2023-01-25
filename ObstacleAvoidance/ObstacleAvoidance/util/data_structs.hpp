#pragma once

namespace strcts {
	template <typename T>
	class linked_arraylist {
		static const unsigned int linked_array_length = 32;
		struct linked_array {
			T items[linked_array_length];
			linked_array* next;
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