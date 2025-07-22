#include <reg51.h>

#define LCD_data P2
#define NUMBER "+9779749460915"

// LCD control bits
sbit RS = P3^2;
sbit RW = P3^3;
sbit EN = P3^4;
sbit buzzer = P0^0;

// Sensor Pins
sbit main_tank_low = P1^0;
sbit pot1_low = P1^1;
sbit pot2_low = P1^2;
sbit main_tank_high = P1^3;
sbit pot1_high = P1^4;
sbit pot2_high = P1^5;

// Valve outputs (Active LOW)
sbit valve2 = P3^6;  // Pot1
sbit valve3 = P3^7;  // Pot2

unsigned char prev_status = 0xFF;
bit prev_fill_main = 1;
bit prev_fill_pot1 = 1;
bit prev_fill_pot2 = 1;

// Delay
void delay(unsigned int time) {
    unsigned int i, j;
    for (i = 0; i < time; i++)
        for (j = 0; j < 1275; j++);
}

// LCD functions
void LCD_cmd(unsigned char command) {
    LCD_data = command;
    RS = 0;
    RW = 0;
    EN = 1;
    delay(10);
    EN = 0;
}

void LCD_data_write(unsigned char data_) {
    LCD_data = data_;
    RS = 1;
    RW = 0;
    EN = 1;
    delay(10);
    EN = 0;
}

void LCD_init() {
    LCD_cmd(0x38);
    delay(10);
    LCD_cmd(0x0E);
    delay(10);
    LCD_cmd(0x01);
    delay(10);
}

void LCD_string_write(unsigned char *string) {
    int i = 0;
    while (string[i] != '\0') {
        LCD_data_write(string[i]);
        i++;
    }
}

// UART
void ser_init(void) {
    SCON = 0x50;
    TMOD |= 0x20;
    TH1 = 0xFD;
    TL1 = 0xFD;
    TR1 = 1;
}

void tx(unsigned char send) {
    SBUF = send;
    while (TI == 0);
    TI = 0;
}

void tx_str(unsigned char *s) {
    while (*s) {
        tx(*s++);
    }
}

void gsm_delay(void) {
    unsigned int i;
    for (i = 0; i < 50000; i++);
}

void sms(unsigned char *num1, unsigned char *msg) {
    gsm_delay();
    tx_str("AT\r");
    gsm_delay();
    tx_str("AT+CMGF=1\r");
    gsm_delay();
    tx_str("AT+CMGS=\"");
    tx_str(num1);
    tx_str("\"\r");
    gsm_delay();
    tx_str(msg);
    tx(0x1A);
    gsm_delay();
}

// Buzzer active LOW
void buzz_for_ms(unsigned int ms) {
    buzzer = 0;   // ON
    delay(ms);
    buzzer = 1;   // OFF
}

void main() {
    unsigned char status;

    // Initialization
    P1 = 0xFF;
    P2 = 0x00;
    RS = RW = EN = 0;
    buzzer = 1;      // OFF
    valve2 = 1;      // OFF
    valve3 = 1;      // OFF

    LCD_init();
    LCD_string_write("Welcome...");
    delay(10);
    LCD_cmd(0x01);
    ser_init();

    while (1) {
        status = 0;
        if (main_tank_low == 0) status |= 0x01;
        if (pot1_low == 0)      status |= 0x02;
        if (pot2_low == 0)      status |= 0x04;

        if (status != prev_status) {
            prev_status = status;
            LCD_cmd(0x01);

            if (status == 0x07) {
                LCD_string_write("All tanks are");
                LCD_cmd(0xC0);
                LCD_string_write("low");
                sms(NUMBER, "All tanks are low");
                LCD_cmd(0x01);
                LCD_string_write("Water Filling..");
                valve2 = 0;  // ON
                valve3 = 0;  // ON
                buzz_for_ms(1000);
            } else if (status == 0x03) {
                LCD_string_write("Main & Pot1 low");
                sms(NUMBER, "Main and Pot1 low");
                LCD_cmd(0x01);
                LCD_string_write("Water Filling..");
                valve2 = 0;  // ON
                buzz_for_ms(1000);
            } else if (status == 0x05) {
                LCD_string_write("Main & Pot2 low");
                sms(NUMBER, "Main and Pot2 low");
                LCD_cmd(0x01);
                LCD_string_write("Water Filling..");
                valve3 = 0;  // ON
                buzz_for_ms(1000);
            } else if (status == 0x06) {
                LCD_string_write("Pot1 & Pot2 low");
                sms(NUMBER, "Pot1 and Pot2 low");
                LCD_cmd(0x01);
                LCD_string_write("Water Filling..");
                valve2 = 0;  // ON
                valve3 = 0;  // ON
            } else if (status == 0x01) {
                LCD_string_write("Main tank low");
                sms(NUMBER, "Main tank is low");
                LCD_cmd(0x01);
                LCD_string_write("Water Filling..");
                buzz_for_ms(1000);
            } else if (status == 0x02) {
                LCD_string_write("Pot 1 is low");
                sms(NUMBER, "Pot 1 is low");
                LCD_cmd(0x01);
                LCD_string_write("Water Filling..");
                valve2 = 0;  // ON
            } else if (status == 0x04) {
                LCD_string_write("Pot 2 is low");
                sms(NUMBER, "Pot 2 is low");
                LCD_cmd(0x01);
                LCD_string_write("Water Filling..");
                valve3 = 0;  // ON
            }
        }

        // Main tank filled
        if (main_tank_high == 0 && prev_fill_main == 1) {
            LCD_cmd(0x01);
            LCD_string_write("Main tank filled");
            sms(NUMBER, "Main tank is filled");
            buzz_for_ms(1000);
            prev_fill_main = 0;
        } else if (main_tank_high == 1) {
            prev_fill_main = 1;
        }

        // Both pots filled
        if (pot1_high == 0 && prev_fill_pot1 == 1 &&
            pot2_high == 0 && prev_fill_pot2 == 1) {
            valve2 = 1;  // OFF
            valve3 = 1;  // OFF
            LCD_cmd(0x01);
            LCD_string_write("Pot 1 & 2 filled");
            sms(NUMBER, "Pot 1 and Pot 2 are filled");
            prev_fill_pot1 = 0;
            prev_fill_pot2 = 0;
        } else {
            // Pot1 filled
            if (pot1_high == 0 && prev_fill_pot1 == 1) {
                valve2 = 1;  // OFF
                LCD_cmd(0x01);
                LCD_string_write("Pot 1 is filled");
                sms(NUMBER, "Pot 1 is filled");
                prev_fill_pot1 = 0;
            } else if (pot1_high == 1) {
                prev_fill_pot1 = 1;
            }

            // Pot2 filled
            if (pot2_high == 0 && prev_fill_pot2 == 1) {
                valve3 = 1;  // OFF
                LCD_cmd(0x01);
                LCD_string_write("Pot 2 is filled");
                sms(NUMBER, "Pot 2 is filled");
                prev_fill_pot2 = 0;
            } else if (pot2_high == 1) {
                prev_fill_pot2 = 1;
            }
        }

        // All tanks filled
        if (main_tank_high == 0 && pot1_high == 0 && pot2_high == 0) {
            LCD_cmd(0x01);
            LCD_string_write("All tanks are");
            LCD_cmd(0xC0);
            LCD_string_write("filled");
            sms(NUMBER, "All tanks are filled");
            buzz_for_ms(1000);
        }

        // Control valves (active LOW)
        if (pot1_low == 0 && pot1_high == 1)
            valve2 = 0;  // ON
        else if (pot1_high == 0)
            valve2 = 1;  // OFF

        if (pot2_low == 0 && pot2_high == 1)
            valve3 = 0;  // ON
        else if (pot2_high == 0)
            valve3 = 1;  // OFF

        delay(5);
    }
}