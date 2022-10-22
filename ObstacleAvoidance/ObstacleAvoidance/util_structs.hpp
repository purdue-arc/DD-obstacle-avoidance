
namespace util {
	struct char_chain {
		unsigned char* chars;
		unsigned long size;
		char_chain* next;
		char_chain* end;
		~char_chain() {
			delete[] chars;
			chars = 0;
			delete next;
			next = 0;
		}
	};

	inline void append(char_chain* a, char_chain* b) {
		a->end->next = b;
		a->end = b->end;
	}
}