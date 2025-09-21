#include <stdio.h>
#include <stdbool.h>

// token constants
#define BIN 1
#define OCT 2
#define DEC 3
#define HEX 4
#define ERROR -1

// state constants
#define NUM_STATES 10
#define START_STATE 0
#define NUM_COLS 7

// final state constants
#define STATE_BIN 6
#define STATE_OCT 7
#define STATE_DEC 8
#define STATE_HEX 9

// character class constants - simplified to avoid overlaps
#define COL_0 0           // '0'
#define COL_1 1           // '1'
#define COL_2_7 2         // [2-7]
#define COL_8_9 3         // [8-9]
#define COL_PREFIX 4      // [bodh]
#define COL_HEX_LETTER 5  // [A-Fa-f]
#define COL_OTHER 6       // other

// transition table
int next_state[NUM_STATES][NUM_COLS] = {
    // state 0: start
    //    0        1      2-7      8-9    bodh     A-F    other
    {     1, STATE_DEC, STATE_DEC, STATE_DEC, ERROR, ERROR, ERROR },

    // state 1: after '0'
    //    0        1      2-7      8-9    bodh     A-F    other
    { STATE_DEC, STATE_DEC, STATE_DEC, STATE_DEC,   2, ERROR, STATE_DEC },

    // state 2: after '0h'
    //    0        1      2-7      8-9    bodh     A-F    other
    { STATE_HEX, STATE_HEX, STATE_HEX, STATE_HEX, ERROR, STATE_HEX, STATE_HEX },

    // state 3: after '0b'
    //    0        1      2-7      8-9    bodh     A-F    other
    { STATE_BIN, STATE_BIN, ERROR, ERROR, ERROR, ERROR, STATE_BIN },

    // state 4: after '0o'
    //    0        1      2-7      8-9    bodh     A-F    other
    { STATE_OCT, STATE_OCT, STATE_OCT, ERROR, ERROR, ERROR, STATE_OCT },

    // state 5: after '0d'
    //    0        1      2-7      8-9    bodh     A-F    other
    { STATE_DEC, STATE_DEC, STATE_DEC, STATE_DEC, ERROR, ERROR, STATE_DEC },

    // state 6: BIN final - self loop on [01]
    //    0        1      2-7      8-9    bodh     A-F    other
    { STATE_BIN, STATE_BIN, ERROR, ERROR, ERROR, ERROR, STATE_BIN },

    // state 7: OCT final - self loop on [0-7]
    //    0        1      2-7      8-9    bodh     A-F    other
    { STATE_OCT, STATE_OCT, STATE_OCT, ERROR, ERROR, ERROR, STATE_OCT },

    // state 8: DEC final - self loop on [0-9]
    //    0        1      2-7      8-9    bodh     A-F    other
    { STATE_DEC, STATE_DEC, STATE_DEC, STATE_DEC, ERROR, ERROR, STATE_DEC },

    // state 9: HEX final - self loop on [0-9A-Fa-f]
    //    0        1      2-7      8-9    bodh     A-F    other
    { STATE_HEX, STATE_HEX, STATE_HEX, STATE_HEX, ERROR, STATE_HEX, STATE_HEX }
};

// classify character - fixed to handle overlaps correctly
int classify_char(char c) {
    if (c == '0') return COL_0;
    if (c == '1') return COL_1;
    if (c >= '2' && c <= '7') return COL_2_7;
    if (c == '8' || c == '9') return COL_8_9;
    if (c == 'b' || c == 'o' || c == 'd' || c == 'h') return COL_PREFIX;
    if ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) return COL_HEX_LETTER;
    return COL_OTHER;
}

// determine which specific state to go to after reading prefix
int get_prefix_state(int current_state, char c) {
    if (current_state == 1) {  // after '0'
        switch(c) {
            case 'b': return 3;  // go to '0b' state
            case 'o': return 4;  // go to '0o' state
            case 'd': return 5;  // go to '0d' state
            case 'h': return 2;  // go to '0h' state
            default: return ERROR;
        }
    }
    return ERROR;
}

// check if state is final
bool is_final_state(int state) {
    return state >= STATE_BIN && state <= STATE_HEX;
}

// map state to token type
int get_token_type(int state) {
    switch(state) {
        case STATE_BIN: return BIN;
        case STATE_OCT: return OCT;
        case STATE_DEC: return DEC;
        case STATE_HEX: return HEX;
        default: return ERROR;
    }
}

// input buffer management
static char input_buffer[100];
static int buffer_pos = 0;
static int buffer_len = 0;

void set_input(const char* str) {
    int i = 0;
    while (str[i] && i < 99) {
        input_buffer[i] = str[i];
        i++;
    }
    input_buffer[i] = '\0';
    buffer_len = i;
    buffer_pos = 0;
}

char nextchar() {
    if (buffer_pos < buffer_len) {
        return input_buffer[buffer_pos++];
    }
    return '\0';
}

void putback_char(char c) {
    if (buffer_pos > 0) {
        buffer_pos--;
    }
}

// main scanner
int gettoken() {
    int state = START_STATE;
    char c;

    while (!is_final_state(state)) {
        c = nextchar();

        // handle
        if (c == '\0') {
            switch (state) {
                case 2: return HEX;
                case 3: return BIN;
                case 4: return OCT;
                case 5: return DEC;
                case 1: return DEC;
                default: return ERROR;
            }
        }

        int char_class = classify_char(c);

        if (state == 1 && char_class == COL_PREFIX) {
            state = get_prefix_state(state, c);
            if (state == ERROR) {
                return ERROR;
            }
            continue;
        }

        int next = next_state[state][char_class];

        if (next == ERROR) {
            switch (state) {
                case 2:
                    putback_char(c);
                    return HEX;
                case 3:
                    putback_char(c);
                    return BIN;
                case 4:
                    putback_char(c);
                    return OCT;
                case 5:
                    putback_char(c);
                    return DEC;
                default: return ERROR;
            }
        }
        state = next;
    }

    // put back the character that caused final state transition
    putback_char(c);
    return get_token_type(state);
}

const char* token_name(int token) {
    switch(token) {
        case BIN: return "BIN";
        case OCT: return "OCT";
        case DEC: return "DEC";
        case HEX: return "HEX";
        case ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void test_input(const char* input) {
    printf("Testing: '%-8s' -> ", input);
    set_input(input);
    int token = gettoken();
    printf("%s\n", token_name(token));
}

int main() {
    test_input("0");      // DEC
    test_input("123");    // DEC
    test_input("0d");     // DEC
    test_input("0d1234"); // DEC
    test_input("0b");     // BIN
    test_input("0b101");  // BIN
    test_input("0o");     // OCT
    test_input("0o127");  // OCT
    test_input("0d");     // DEC
    test_input("0d359");  // DEC
    test_input("0h");     // HEX
    test_input("0hABC");  // HEX

    return 0;
}
