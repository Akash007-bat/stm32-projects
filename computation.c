#include "mbed.h"
#include <cstdio>

// Set up the serial interface for UART communication using BufferedSerial
BufferedSerial pc(USBTX, USBRX, 9600);  // TX, RX pins and baud rate

size_t my_strlen(const char* str) {
    const char* s = str;
    while (*s) {
        ++s;
    }
    return s - str;
}

void uart_print(const char *message) {
    pc.write(message, my_strlen(message));
}

void uart_print_float(float value) {
    int int_part = (int)value;
    int frac_part = (int)((value - int_part) * 100000000);  // 10 decimal places
    char buffer[50];
    sprintf(buffer, "%d.%08d", int_part, abs(frac_part));  // Print with 6 decimals
    uart_print(buffer);
}

void my_strcpy(char* dest, const char* src) {
    size_t length = my_strlen(src);  // Get the length of the source string
    for (size_t i = 0; i < length; i++) {
        dest[i] = src[i];
    }
    dest[length] = 0;  // Manually add the null terminator at the end
}

int my_strcmp(const char* str1, const char* str2) {
    const char* short_str;
    const char* long_str;

    // Determine which string is shorter
    if (my_strlen(str1) < my_strlen(str2)) {
        short_str = str1;
        long_str = str2;
    } else {
        short_str = str2;
        long_str = str1;
    }

    // Iterate through the longer string
    while (*long_str) {
        const char* l = long_str;
        const char* s = short_str;

        // Check if the shorter string matches at this position
        while (*s && (*l == *s)) {
            l++;
            s++;
        }

        // If we reached the end of the shorter string, it means we found a match
        if (*s == '\0') {
            return 0;  // Indicating a match found
        }

        long_str++;
    }

    return 1;  // Indicating no match found
}


char* my_strcat(char* dest, const char* src) {
    char* ptr = dest + my_strlen(dest); // Move to the end of dest
    while (*src) {
        *ptr++ = *src++; // Copy src to dest
    }
    *ptr = '\0'; // Null-terminate the concatenated string
    return dest; // Return the destination string
}

// Function to initialize the DWT cycle counter
void init_cycle_counter() {
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
}

// Function to get the current cycle count
uint32_t get_cycle_count() {
    return DWT->CYCCNT;
}

// Function for comparing binary values
bool greatervalue(const char *s, const char *ln) {
    for (short int i = 0; i<=12; i++) {
        if (s[i] < ln[i])
            return false;
        else if (s[i] > ln[i])
            return true;
    }
    return true;
}

// Two's complement function for binary subtraction
void twoscompliment(char *ln) {
    bool compliment = false;
    for (short int i = my_strlen(ln) - 1; i >= 0 && ln[i] != '.'; i--) {
        if (!compliment && ln[i] == '1') {
            compliment = true;
            continue;
        }
        if (compliment) {
            ln[i] = (ln[i] == '0') ? '1' : '0';
        }
    }
}

// Binary subtraction (difference) function
void difference(char *s, const char *ln) {
    // Create a mutable copy of ln for two's complement modification
    char ln_copy[my_strlen(ln) + 1];
    my_strcpy(ln_copy, ln);

    // Perform two's complement on ln_copy
    twoscompliment(ln_copy);

    bool carry = false;

    // Find the lengths of s and ln_copy
    int len_s = my_strlen(s);
    int len_ln = my_strlen(ln_copy);

    // Start from the last character of s and ln_copy
    int i = len_s - 1;
    int j = len_ln - 1;

    // Iterate backward through the strings
    while (i >= 0) {
        if (s[i] == '.') {  // Skip the decimal point
            i--;
            break;
        }

        // Handle the binary subtraction
        int bit_s = (i >= 0) ? s[i] - '0' : 0;
        int bit_ln = (j >= 0 && ln_copy[j] != '.') ? ln_copy[j] - '0' : 0;

        int sum = bit_s + bit_ln + carry;
        if (sum >= 2) {
            s[i] = (sum % 2) + '0';
            carry = true;
        } else {
            s[i] = sum + '0';
            carry = false;
        }

        i--;
        j--;
    }
}

// Function to convert a decimal number to binary
void decimalToBinary(float fraction, char *binary) {
    my_strcpy(binary, "0.");
    if (fraction >= 1) fraction -= 1;
    while (my_strlen(binary) < 12) {
        fraction *= 2;
        my_strcat(binary, (fraction >= 1) ? "1" : "0");
        if (fraction >= 1) fraction -= 1;
    }
}

// Function to compute e^x using binary expansion
float computeExp(float x) {
    float y = 1.0;
    float xi = x;
    const char *lntable[11] = {
        "0.1011000110", "0.0110011111", "0.0011100100",
        "0.0001111001", "0.0000111110", "0.0000100000",
        "0.0000010000", "0.0000001000", "0.0000000100",
        "0.0000000001"
    };
    char s[13];  // 12 bits for the binary representation + null terminator
    decimalToBinary(xi, s);
    short int i = 0;
    while (i<= 10) {
        bool k = greatervalue(s, lntable[i]);
        if (k) {
            difference(s, lntable[i]);
            y *= (1 + (1.0 / (1 << i)));
        } else {
            i++;
        }
        if (my_strcmp(s, "0.0000000000") == 0) {
            break;
        }
    }
    return y;
}

int main() {
    init_cycle_counter();
    while (true) {
        // Reset the cycle counter
        DWT->CYCCNT = 0;
        // Measure start time in clock cycles
        
        float x;
        uart_print("Enter the value of x: \r\n");
        // Read input value from user
        char input_buffer[20];
        int index = 0;
        char c;
        // Read until newline or carriage return character
        while (index < sizeof(input_buffer) - 1) {
            if (pc.read(&c, 1) > 0) {
                if (c == '\n' || c == '\r') {
                    break;  // Break on newline or carriage return
                }
                input_buffer[index++] = c;
            }
        }
        input_buffer[index] = '\0';  // Null-terminate the string
        sscanf(input_buffer, "%f", &x);
        if (x == -7) {
            uart_print("\nend of execution\r\n");
            return 0;
        }
        uart_print("\nStarting program timer after taking the inputs ...\r\n");
        uint32_t start_time = get_cycle_count();
        // Compute e^x
        float result = computeExp(x);
        // Output the result over UART
        char buffer[50];
        sprintf(buffer, "\n10 bits precision \r\n");
        uart_print(buffer);
        uart_print_float(result);
        // Measure end time in clock cycles
        uint32_t end_time = get_cycle_count();
        uint32_t duration = end_time - start_time;
        // Convert the duration from clock cycles to nanoseconds
        uint32_t duration_ns = duration * (1000);
        // Output execution time in nanoseconds over UART
        sprintf(buffer, "\nExecution time: %u ns\r\n", duration_ns);
        uart_print(buffer);
    }
}