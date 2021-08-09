#include <stdio.h>

#define CORRECT_R4 0xfeb1
#define CORRECT_R6 0x9298

static inline int swap_bytes(int n) {
    return ((n & 0xff00) >> 8) + ((n & 0x00ff) << 8);
}

static inline long password_cycle(int word, int r4, int r6) {
    r4 += word;
    r4 = swap_bytes(r4);

    r6 = word ^ r6;

    // swap r4 and r6
    r6 = r4 ^ r6;
    r4 = r4 ^ r6;
    r6 = r4 ^ r6;

    return (r4 << 16) + r6;
}

void recursive_brute_force(int output[3], int r4, int r6, int n_chars, int n_recursions, int verbose) {
    for(int first_char = 0; first_char < 255; first_char++) {
        int second_char_range = 255;
        if(n_chars == 1) {
            second_char_range = 1;
        }

        for(int second_char = 0; second_char < second_char_range; second_char++) {
            int word = (first_char << 8) + second_char;

            if(verbose) {
                printf("trying with first word %x\n", word);
            }

            int cycled = password_cycle(word, r4, r6);
            int new_r4 = (int)((cycled & 0xffff0000) >> 16);
            int new_r6 = (int)(cycled & 0xffff);

            if(n_chars < 3) {
                if(new_r4 == CORRECT_R4 && new_r6 == CORRECT_R6) {
                    printf("!!!SOLUTION!!!");
                    output[n_recursions] = word;
                    return;
                }
            }
            else {
                recursive_brute_force(output, new_r4, new_r6, n_chars - 2, n_recursions + 1, 0);
                if(output[n_recursions + 1] > 0) {
                    output[n_recursions] = word;
                    return;
                }
            }
        }
    }
}

int main(int argc, int argv) {
    int solution[3] = {0, 0, 0};

    recursive_brute_force(solution, 0, 0, 5, 0, 1);
    printf("\n%x\n%x\n%x\n", solution[0], solution[1], solution[2]);

    getchar();
}
