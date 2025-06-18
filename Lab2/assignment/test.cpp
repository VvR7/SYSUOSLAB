#include <iostream>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <ctime>

#define RAND_WIDTH 26
#define ARRAY_LENGTH 50

extern "C" void student_function();
extern "C" void your_function();


int if_flag, a1, a2 = 0;
const char your_string_num[] = {68, 105, 37, 58, 95, 92, 101, 35, 23, 106, 107, 108, 91, 92, 101, 107, 106, 23, 88, 101, 91, 23, 75, 56, 106, 23, 88, 105, 92, 23, 107, 95, 92, 23, 89, 92, 106, 107, 24, 1, 0};
const char* your_string = your_string_num;
char *while_flag, *random_buffer;


// THE ONLY CODE YOU CAN CHANGE IN THIS FILE!!!
void student_setting() {
    a1 = 24;
}




extern "C" char my_random() {
    char c = rand() % (RAND_WIDTH) + 'A';
    random_buffer[a2 * 2] = c;
    return c;
}


extern "C" void print_a_char(char c) {
    std::cout << c;
}


void test_failed() {
    std::cout << ">>> test failed" << std::endl;
    exit(0);
}

void print(char*s)
{
    for (int i = 0; i < 52; i++) 
        s[i]=='\0'?printf(" "):printf("%c",s[i]);
    std::cout<<"\n";
}
int main() {
    student_setting();

    // init
    srand(time(nullptr));
    std::cout << ">>> begin test" << std::endl;
    if_flag = 0;
    while_flag = new char[52];
    random_buffer = new char[52];
    if(!while_flag || !random_buffer) {
        std::cout << "test can not run" << std::endl;
        exit(-1);
    }
    memset(while_flag, 0, 52);
    memset(random_buffer, 0, 52);

    student_function();

    if(a1 >= 40) {
        if (if_flag != (a1 + 3) / 5) {
            test_failed();
        }
    } else if(a1 >= 18) {
        if (if_flag != 80 - (a1 * 2)) {
            test_failed();
        }
    } else {
        if (if_flag != a1 << 5) {
            test_failed();
        }
    }

    std::cout << ">>> if test pass!" << std::endl;

    bool check_flag = true;
    int flag = 1;
	for (int i = 0; i < 52; i++) {
		if (while_flag[i] != random_buffer[i]) {
			flag = 0;
			break;
		}
	}

	if (flag && a2 >= ARRAY_LENGTH / 2) {
        std::cout<<"a2="<<a2<<"\n";
        print(while_flag);
        print(random_buffer);
		std::cout << ">>> while test pass!" << std::endl;
	} else {
        test_failed();
    }
    your_function();
}

