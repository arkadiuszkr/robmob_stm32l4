Robot mobilny – implementacja STM32L4

Idea projektu: Robot mobilny jako układ wahadła odwróconego na dwóch kołach. Porównanie implementacji pętli sterowania na Linux (Raspberry Pi z kernel patch) vs FreeRTOS (STM32L476) z możliwym rozszerzeniem o ESP32

...mermaid
graph LR  

subgraph "MCU"
    PI["Raspberry Pi 4 / STM32 / ESP32"]
    CONNECTOR["Standard GPIO/Signal Plug"]
end

subgraph "Sensors"
    GYRO["Gyroscope (SPI)"]
    ENCODER["Encoder (GPIO 2-phase)"]
    BUTTONS["Buttons (GPIO)"]
end

GYRO --> CONNECTOR 
ENCODER --> CONNECTOR 
BUTTONS --> CONNECTOR 

SCREEN["Screen (SPI 2)"]
MOTORS["Motors"]

CONNECTOR --> SCREEN
CONNECTOR -->|PWM| MOTORS

classDef sensors fill:#b4dcef,stroke:#0288d1,stroke-width:2px;
classDef pi fill:#c8e6c9,stroke:#2e7d32,stroke-width:2px;
classDef outputs fill:#ffe0b2,stroke:#f57c00,stroke-width:2px;
classDef actuators fill:#ffcdd2,stroke:#c62828,stroke-width:2px;
classDef connector fill:#d1c4e9,stroke:#673ab7,stroke-width:2px,stroke-dasharray:5 5;

class PI pi;
class GYRO,ENCODER,BUTTONS sensors;
class SCREEN outputs;
class MOTORS actuators;
class CONNECTOR connector;
...




Najważniejsze elementy projektu
Architektura (main_Robot.c/freertos_header.h)
    • początkowy prototyp aplikacji na Raspberry Pi z wykorzystaniem POSIX Threads
    • następnie projekt został przeniesiony na STM32L476RG przez FreeRTOS (początek git wrzuconego na to repo)
    • rozdzielenie struktury programu na osobne pliki, stworzenie interface dla podzespołów
    • komunikacja między zadaniami realizowana jest za pomocą kolejek wiadomości, kolejek zdarzeń oraz TaskNotify

...mermaid
flowchart TD

    subgraph Input Layer
        UARTRX["UART Commands"]
        BUTTONS["Buttons (EXTI)"]
    end

    subgraph Application Layer
        EVENTQ["Event Queue"]
        MENU["Menu System"]
    end

    subgraph Display Layer
        DRIVER["DisplayDriver"]
        RENDER["LCD Rendering Task"]
        UARTTask["UART Tx Task"]
    end

    subgraph Output Layer
        TERMINAL["UART Terminal"]
        LCD["ILI9488 LCD"]
    end

    UARTRX --> EVENTQ
    BUTTONS --> EVENTQ

    EVENTQ --> MENU

    MENU --> DRIVER

    DRIVER -->|UART Message Queue| UARTTask
    UARTTask --> TERMINAL

    DRIVER -->|"Saved snapshot"| RENDER

    RENDER -->|"SPI DMA"| LCD
...
System menu (cli_menu.c/cli_menu.h)
    • modułowy system menu w C 1(stworzony przed migracją, na Raspberry Pi)
    • sterowanie menu odbywa się poprzez wspólną kolejkę zdarzeń poprzez:
        ◦ komendy przesyłane przez UART,
        ◦ przyciski sprzętowe obsługiwane przez przerwania zewnętrzne
Warstwa abstrakcji wyświetlacza (displayDriver.c/displayDriver.h)
    • komunikacja UART
    • wyświetlacz LCD
    • (poprzednio) terminal Linux podczas prototypowania
Sterownik LCD ILI9488 (lcd_ILI9488.c)
    • logika dwóch buferów (frontBuffer i backBuffer)
    • synchronizacja poprzez semaforę binarną oraz TaskNotify
    • transmisja SPI (18bit RGB) przez DMA z prędkością 20MBit/s

...mermaid
sequenceDiagram
    participant App
    participant Snapshot
    participant Renderer
    participant Font
    participant Buffer
    participant SPI
    participant LCD

    App->>Snapshot: _lcd_clear()

    App->>Snapshot: _lcd_addToSnapshot_printLine("Settings")
    App->>Snapshot: _lcd_addToSnapshot_printBold("WiFi")
    App->>Snapshot: _lcd_addToSnapshot_printLine("Network")

    App->>Snapshot: _lcd_drawMenu()
    Note over App,Snapshot: lcd_requestedReprint = true

    Renderer->>Snapshot: Acquire semaphore

    loop For each LCD window
        Renderer->>LCD: LCD_SetWindow(x0,y0,x1,y1)

        Renderer->>Buffer: generatePixelBuffer(y0,y1)

        loop For each snapshot line
            Renderer->>Font: drawTextLine()

            loop For each character
                Renderer->>Font: glyph lookup

                Font-->>Renderer: Glyph descriptor

                Renderer->>Font: bitmap lookup

                Font-->>Renderer: glyph bitmap data

                Renderer->>Buffer: Swap fontBuffer and backBuffer pointers (wait until DMA finishes)
            end
        end

        Renderer->>SPI: "transmitSPI_PixelBuffer(): Set DMA start"
        SPI->>LCD: Window pixel data
    end

Renderer->>Snapshot: Release semaphore
...
Własny silnik renderowania tekstu terminal-style (lcd_ILI9488.c/fonts.h)
    • obsługa bitmapowych czcionek wygenerowanych dla LVGL przez adapter (https://lvgl.io/tools/fontconverter)
    • własny renderer tekstu
    • zawijanie słów
    • renderowanie okienkowe (uint8_t buffer[LCD_Width * Window_Height])
    • rozdzielenie tekstu na okna buferów
    • odświerzanie tylko tej części ekranu, która uległa zmianie (dirty_region)
Napisanie własnego rozwiązania zamiast biblioteki LVGL ze względu na ograniczenia pamięci na STM32L476
Komunikacja UART (freertos.c/input.c)
    • odbiór komend menu w przerwaniach
    • przetwarzanie komunikatów poprzez kolejki wiadomości (Queue_UART_SendDebugHandle)
Szkic PCB dla układu elektronicznego robota
...mermaid
graph TD

    subgraph PCB["PCB"]
        subgraph ESP32_Connector["ESP32 Connector"]
            ESP32_Conn[("Connector")]
            ESP32_SPI_MOSI["SPI MOSI"]
            ESP32_SPI_MISO["SPI MISO"]
            ESP32_SPI_SCK["SPI SCK"]
            ESP32_SPI_CS["SPI CS"]
            ESP32_SPI2_MOSI["SPI2 MOSI"]
            ESP32_SPI2_MISO["SPI2 D/C"]
            ESP32_SPI2_SCK["SPI2 SCK"]
            ESP32_SPI2_CS["SPI2 CS"]
            ESP32_BUTTON_1["Button 1"]
            ESP32_BUTTON_2["Button 2"]
            ESP32_BUTTON_3["Button 3"]
            ESP32_BUTTON_4["Button 4"]
            ESP32_LEFT_ENCODER_A["Left Encoder A"]
            ESP32_LEFT_ENCODER_B["Left Encoder B"]
            ESP32_LEFT_PWM["Left Motor PWM"]
            ESP32_RIGHT_ENCODER_A["Right Encoder A"]
            ESP32_RIGHT_ENCODER_B["Right Encoder B"]
            ESP32_RIGHT_PWM["Right Motor PWM"]
            ESP32_POWER["Power (3.3V)"]
            ESP32_GND["GND"]
            ESP32_RGB_CH1["RGB Diode CH1"]
            ESP32_RGB_CH2["RGB Diode CH2"]
            ESP32_RGB_CH3["RGB Diode CH3"]
        end

        subgraph STM32_Connector["NUCLEO-L476RG"]
            STM32_Conn[("Connector")]
            STM32_SPI_MOSI["SPI MOSI"]
            STM32_SPI_MISO["SPI MISO"]
            STM32_SPI_SCK["SPI SCK"]
            STM32_SPI_CS["SPI CS"]
            STM32_SPI2_MOSI["SPI2 MOSI"]
            STM32_SPI2_MISO["SPI2 D/C"]
            STM32_SPI2_SCK["SPI2 SCK"]
            STM32_SPI2_CS["SPI2 CS"]
            STM32_BUTTON_1["Button 1"]
            STM32_BUTTON_2["Button 2"]
            STM32_BUTTON_3["Button 3"]
            STM32_BUTTON_4["Button 4"]
            STM32_LEFT_ENCODER_A["Left Encoder A"]
            STM32_LEFT_ENCODER_B["Left Encoder B"]
            STM32_LEFT_PWM["Left Motor PWM"]
            STM32_RIGHT_ENCODER_A["Right Encoder A"]
            STM32_RIGHT_ENCODER_B["Right Encoder B"]
            STM32_RIGHT_PWM["Right Motor PWM"]
            STM32_POWER["Power (3.3V)"]
            STM32_GND["GND"]
            STM32_RGB_CH1["RGB Diode CH1"]
            STM32_RGB_CH2["RGB Diode CH2"]
            STM32_RGB_CH3["RGB Diode CH3"]
        end

        subgraph PI_Connector["Raspberry Pi 4B Connector"]
            PI_Conn[("Connector")]
            PI_SPI_MOSI["SPI MOSI"]
            PI_SPI_MISO["SPI MISO"]
            PI_SPI_SCK["SPI SCK"]
            PI_SPI_CS["SPI CS"]
            PI_SPI2_MOSI["SPI2 MOSI"]
            PI_SPI2_MISO["SPI2 D/C"]
            PI_SPI2_SCK["SPI2 SCK"]
            PI_SPI2_CS["SPI2 CS"]
            PI_BUTTON_1["Button 1"]
            PI_BUTTON_2["Button 2"]
            PI_BUTTON_3["Button 3"]
            PI_BUTTON_4["Button 4"]
            PI_LEFT_ENCODER_A["Left Encoder A"]
            PI_LEFT_ENCODER_B["Left Encoder B"]
            PI_LEFT_PWM["Left Motor PWM"]
            PI_RIGHT_ENCODER_A["Right Encoder A"]
            PI_RIGHT_ENCODER_B["Right Encoder B"]
            PI_RIGHT_PWM["Right Motor PWM"]
            PI_RGB_CH1["RGB Diode CH1"]
            PI_RGB_CH2["RGB Diode CH2"]
            PI_RGB_CH3["RGB Diode CH3"]
        end

        %% In-between PCB Connectors
        INBETWEEN_SPI_MOSI["SPI MOSI"]
        INBETWEEN_SPI_MISO["SPI MISO"]
        INBETWEEN_SPI_SCK["SPI SCK"]
        INBETWEEN_SPI_CS["SPI CS"]
        INBETWEEN_SPI2_MOSI["SPI2 MOSI"]
        INBETWEEN_SPI2_MISO["SPI2 D/C"]
        INBETWEEN_SPI2_SCK["SPI2 SCK"]
        INBETWEEN_SPI2_CS["SPI2 CS"]
        INBETWEEN_BUTTON_1["Button 1"]
        INBETWEEN_BUTTON_2["Button 2"]
        INBETWEEN_BUTTON_3["Button 3"]
        INBETWEEN_BUTTON_4["Button 4"]
        INBETWEEN_LEFT_ENCODER_A["Left Encoder A"]
        INBETWEEN_LEFT_ENCODER_B["Left Encoder B"]
        INBETWEEN_LEFT_PWM["Left Motor PWM"]
        INBETWEEN_RIGHT_ENCODER_A["Right Encoder A"]
        INBETWEEN_RIGHT_ENCODER_B["Right Encoder B"]
        INBETWEEN_RIGHT_PWM["Right Motor PWM"]
        INBETWEEN_RGB_CH1["RGB Diode CH1"]
        INBETWEEN_RGB_CH2["RGB Diode CH2"]
        INBETWEEN_RGB_CH3["RGB Diode CH3"]
        INBETWEEN_POWER["Power (3.3V)"]
        INBETWEEN_GND["GND"]
    end

    %% MCU to Connector Connections for RGB Diodes
    ESP32_RGB_CH1 --- STM32_RGB_CH1
    ESP32_RGB_CH2 --- STM32_RGB_CH2
    ESP32_RGB_CH3 --- STM32_RGB_CH3
    STM32_RGB_CH1 --- PI_RGB_CH1
    STM32_RGB_CH2 --- PI_RGB_CH2
    STM32_RGB_CH3 --- PI_RGB_CH3
    PI_RGB_CH1 ---|PWM| INBETWEEN_RGB_CH1
    PI_RGB_CH2 ---|PWM| INBETWEEN_RGB_CH2
    PI_RGB_CH3 ---|PWM| INBETWEEN_RGB_CH3

    %% External RGB Diodes Subsystem
    subgraph RGB_Diodes["RGB Diodes"]
        MOSFET_CH1["MOSFET CH1 (Red)"]
        MOSFET_CH2["MOSFET CH2 (Green)"]
        MOSFET_CH3["MOSFET CH3 (Blue)"]
        RGB_DIODES["9x RGB Diodes"]
    end

    %% External Connections
    INBETWEEN_RGB_CH1 --- MOSFET_CH1
    INBETWEEN_RGB_CH2 --- MOSFET_CH2
    INBETWEEN_RGB_CH3 --- MOSFET_CH3

    MOSFET_CH1 --- RGB_DIODES
    MOSFET_CH2 --- RGB_DIODES
    MOSFET_CH3 --- RGB_DIODES

    ESP32["ESP32"]
    STM32["STM32"]
    PI["Raspberry Pi 4B"]

    GYRO["Gyroscope (SPI)"]
    BUTTONS["4x Buttons (GPIO)"]
    OLED_SCREEN["LCD Screen (SPI2)"]

    subgraph Wheels["Wheels"]
        subgraph Left_Wheel["Left Wheel"]
            LEFT_MOTOR["Left Motor"]
            LEFT_ENCODER["Left Encoder"]
        end
        subgraph Right_Wheel["Right Wheel"]
            RIGHT_MOTOR["Right Motor"]
            RIGHT_ENCODER["Right Encoder"]
        end
    end

    %% MCU to Connector Connections
    ESP32 --> ESP32_Conn
    STM32 --> STM32_Conn
    PI --> PI_Conn

    %% ESP32 to STM32 Connections
    ESP32_SPI_MOSI --- STM32_SPI_MOSI
    ESP32_SPI_MISO --- STM32_SPI_MISO
    ESP32_SPI_SCK --- STM32_SPI_SCK
    ESP32_SPI_CS --- STM32_SPI_CS
    ESP32_SPI2_MOSI --- STM32_SPI2_MOSI
    ESP32_SPI2_MISO --- STM32_SPI2_MISO
    ESP32_SPI2_SCK --- STM32_SPI2_SCK
    ESP32_SPI2_CS --- STM32_SPI2_CS
    ESP32_BUTTON_1 --- STM32_BUTTON_1
    ESP32_BUTTON_2 --- STM32_BUTTON_2
    ESP32_BUTTON_3 --- STM32_BUTTON_3
    ESP32_BUTTON_4 --- STM32_BUTTON_4
    ESP32_LEFT_ENCODER_A --- STM32_LEFT_ENCODER_A
    ESP32_LEFT_ENCODER_B --- STM32_LEFT_ENCODER_B
    ESP32_LEFT_PWM --- STM32_LEFT_PWM
    ESP32_RIGHT_ENCODER_A --- STM32_RIGHT_ENCODER_A
    ESP32_RIGHT_ENCODER_B --- STM32_RIGHT_ENCODER_B
    ESP32_RIGHT_PWM --- STM32_RIGHT_PWM
    ESP32_POWER --- STM32_POWER
    ESP32_GND --- STM32_GND

    %% STM32 to Pi Connections
    STM32_SPI_MOSI --- PI_SPI_MOSI
    STM32_SPI_MISO --- PI_SPI_MISO
    STM32_SPI_SCK --- PI_SPI_SCK
    STM32_SPI_CS --- PI_SPI_CS
    STM32_SPI2_MOSI --- PI_SPI2_MOSI
    STM32_SPI2_MISO --- PI_SPI2_MISO
    STM32_SPI2_SCK --- PI_SPI2_SCK
    STM32_SPI2_CS --- PI_SPI2_CS
    STM32_BUTTON_1 --- PI_BUTTON_1
    STM32_BUTTON_2 --- PI_BUTTON_2
    STM32_BUTTON_3 --- PI_BUTTON_3
    STM32_BUTTON_4 --- PI_BUTTON_4
    STM32_LEFT_ENCODER_A --- PI_LEFT_ENCODER_A
    STM32_LEFT_ENCODER_B --- PI_LEFT_ENCODER_B
    STM32_LEFT_PWM --- PI_LEFT_PWM
    STM32_RIGHT_ENCODER_A --- PI_RIGHT_ENCODER_A
    STM32_RIGHT_ENCODER_B --- PI_RIGHT_ENCODER_B
    STM32_RIGHT_PWM --- PI_RIGHT_PWM

    %% Pi to In-between Connections
    PI_SPI_MOSI --- INBETWEEN_SPI_MOSI
    PI_SPI_MISO --- INBETWEEN_SPI_MISO
    PI_SPI_SCK --- INBETWEEN_SPI_SCK
    PI_SPI_CS --- INBETWEEN_SPI_CS
    PI_SPI2_MOSI --- INBETWEEN_SPI2_MOSI
    PI_SPI2_MISO --- INBETWEEN_SPI2_MISO
    PI_SPI2_SCK --- INBETWEEN_SPI2_SCK
    PI_SPI2_CS --- INBETWEEN_SPI2_CS
    PI_BUTTON_1 --- INBETWEEN_BUTTON_1
    PI_BUTTON_2 --- INBETWEEN_BUTTON_2
    PI_BUTTON_3 --- INBETWEEN_BUTTON_3
    PI_BUTTON_4 --- INBETWEEN_BUTTON_4
    PI_LEFT_ENCODER_A --- INBETWEEN_LEFT_ENCODER_A
    PI_LEFT_ENCODER_B --- INBETWEEN_LEFT_ENCODER_B
    PI_LEFT_PWM ---|PWM| INBETWEEN_LEFT_PWM
    PI_RIGHT_ENCODER_A --- INBETWEEN_RIGHT_ENCODER_A
    PI_RIGHT_ENCODER_B --- INBETWEEN_RIGHT_ENCODER_B
    PI_RIGHT_PWM ---|PWM| INBETWEEN_RIGHT_PWM
    STM32_POWER --- INBETWEEN_POWER
    STM32_GND --- INBETWEEN_GND

    %% In-between to Peripherals Connections
    INBETWEEN_SPI_MOSI <--> GYRO
    INBETWEEN_SPI_MISO <--> GYRO
    INBETWEEN_SPI_SCK --> GYRO
    INBETWEEN_SPI_CS --> GYRO

    INBETWEEN_SPI2_MOSI <--> OLED_SCREEN
    INBETWEEN_SPI2_MISO <--> OLED_SCREEN
    INBETWEEN_SPI2_SCK --> OLED_SCREEN
    INBETWEEN_SPI2_CS --> OLED_SCREEN

    INBETWEEN_BUTTON_1 --- BUTTONS
    INBETWEEN_BUTTON_2 --- BUTTONS
    INBETWEEN_BUTTON_3 --- BUTTONS
    INBETWEEN_BUTTON_4 --- BUTTONS

    INBETWEEN_LEFT_PWM -->|IN1| TRANSISTOR_LEFT["Motor driver"] -->|OUT1| LEFT_MOTOR
    INBETWEEN_LEFT_ENCODER_A <-.-> LEFT_ENCODER
    INBETWEEN_LEFT_ENCODER_B <-.-> LEFT_ENCODER

    INBETWEEN_RIGHT_PWM -->|IN2| TRANSISTOR_LEFT -->|OUT2| RIGHT_MOTOR
    INBETWEEN_RIGHT_ENCODER_A <-.-> RIGHT_ENCODER
    INBETWEEN_RIGHT_ENCODER_B <-.-> RIGHT_ENCODER

    %% Styling
    classDef mcu fill:#f0f0f0,stroke:#333,stroke-width:2px;
    classDef stm32 fill:#c8e6c9,stroke:#2e7d32,stroke-width:2px;
    classDef pi fill:#e1bee7,stroke:#7b1fa2,stroke-width:2px;
    classDef esp32 fill:#fff9c4,stroke:#f57f17,stroke-width:2px;
    classDef inbetween fill:#f5f5f5,stroke:#9e9e9e,stroke-width:1px;
    classDef sensors fill:#b4dcef,stroke:#0288d1,stroke-width:2px;
    classDef outputs fill:#ffe0b2,stroke:#f57c00,stroke-width:2px;
    classDef actuators fill:#ffcdd2,stroke:#c62828,stroke-width:2px;
    classDef connector fill:#d1c4e9,stroke:#673ab7,stroke-width:2px,stroke-dasharray:5 5;
    classDef transistor fill:#ffab91,stroke:#bf360c,stroke-width:2px;
    classDef rgbRed fill:#ffcdd2,stroke:#c62828,stroke-width:2px;
    classDef rgbGreen fill:#c8e6c9,stroke:#2e7d32,stroke-width:2px;
    classDef rgbBlue fill:#bbdefb,stroke:#0d47a1,stroke-width:2px;
    classDef rgbGroup fill:#e3f2fd,stroke:#2196f3,stroke-width:2px;

    class ESP32,STM32,PI mcu;
    class GYRO,BUTTONS sensors;
    class OLED_SCREEN outputs;
    class LEFT_MOTOR,RIGHT_MOTOR,LEFT_ENCODER,RIGHT_ENCODER actuators;
    class ESP32,ESP32_SPI_MOSI,ESP32_SPI_MISO,ESP32_SPI_SCK,ESP32_SPI_CS,ESP32_SPI2_MOSI,ESP32_SPI2_MISO,ESP32_SPI2_SCK,ESP32_SPI2_CS,ESP32_BUTTON_1,ESP32_BUTTON_2,ESP32_BUTTON_3,ESP32_BUTTON_4,ESP32_LEFT_ENCODER_A,ESP32_LEFT_ENCODER_B,ESP32_LEFT_PWM,ESP32_RIGHT_ENCODER_A,ESP32_RIGHT_ENCODER_B,ESP32_RIGHT_PWM,ESP32_POWER,ESP32_GND,ESP32_RGB_CH1,ESP32_RGB_CH2,ESP32_RGB_CH3 esp32;
    class STM32,STM32_SPI_MOSI,STM32_SPI_MISO,STM32_SPI_SCK,STM32_SPI_CS,STM32_SPI2_MOSI,STM32_SPI2_MISO,STM32_SPI2_SCK,STM32_SPI2_CS,STM32_BUTTON_1,STM32_BUTTON_2,STM32_BUTTON_3,STM32_BUTTON_4,STM32_LEFT_ENCODER_A,STM32_LEFT_ENCODER_B,STM32_LEFT_PWM,STM32_RIGHT_ENCODER_A,STM32_RIGHT_ENCODER_B,STM32_RIGHT_PWM,STM32_POWER,STM32_GND,STM32_RGB_CH1,STM32_RGB_CH2,STM32_RGB_CH3 stm32;
    class PI,PI_SPI_MOSI,PI_SPI_MISO,PI_SPI_SCK,PI_SPI_CS,PI_SPI2_MOSI,PI_SPI2_MISO,PI_SPI2_SCK,PI_SPI2_CS,PI_BUTTON_1,PI_BUTTON_2,PI_BUTTON_3,PI_BUTTON_4,PI_LEFT_ENCODER_A,PI_LEFT_ENCODER_B,PI_LEFT_PWM,PI_RIGHT_ENCODER_A,PI_RIGHT_ENCODER_B,PI_RIGHT_PWM,PI_POWER,PI_GND,PI_RGB_CH1,PI_RGB_CH2,PI_RGB_CH3 pi;
    class INBETWEEN_SPI_MOSI,INBETWEEN_SPI_MISO,INBETWEEN_SPI_SCK,INBETWEEN_SPI_CS,INBETWEEN_SPI2_MOSI,INBETWEEN_SPI2_MISO,INBETWEEN_SPI2_SCK,INBETWEEN_SPI2_CS,INBETWEEN_BUTTON_1,INBETWEEN_BUTTON_2,INBETWEEN_BUTTON_3,INBETWEEN_BUTTON_4,INBETWEEN_LEFT_ENCODER_A,INBETWEEN_LEFT_ENCODER_B,INBETWEEN_LEFT_PWM,INBETWEEN_RIGHT_ENCODER_A,INBETWEEN_RIGHT_ENCODER_B,INBETWEEN_RIGHT_PWM,INBETWEEN_POWER,INBETWEEN_GND,INBETWEEN_RGB_CH1,INBETWEEN_RGB_CH2,INBETWEEN_RGB_CH3 inbetween;
    class TRANSISTOR_LEFT,TRANSISTOR_RIGHT transistor;
    class MOSFET_CH1 rgbRed;
    class MOSFET_CH2 rgbGreen;
    class MOSFET_CH3 rgbBlue;
    class RGB_DIODES rgbGroup;
...

