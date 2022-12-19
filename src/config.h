
#include <stdbool.h>
#include <stdint.h>


typedef union {
    uint8_t bytes[3];
    uint32_t value;
} dtr_t;

typedef enum {
    DEVMODE_NORMAL,
    DEVMODE_PROVISIONING,
    DEVMODE_WRITING,
} device_mode_t;

typedef struct {
    uint8_t shortAddr;  
    uint8_t numButtons;
    uint8_t targets[5]; // The targets

    uint8_t shortPressTimer; // in 20 ms increments
    uint8_t doublePressTimer; 
    uint8_t repeatTimer;  
    uint8_t stuckTimer; // In seconds.

    // --- Persistence Threshold -- Values after this one will be reset to defaults after reboot or on call to retrieve config.
    device_mode_t mode;
    uint8_t randomAddr[3];
    uint8_t searchAddr[3];
    dtr_t dtr;
    void *dataPtr;
} config_t;

#define PERSISTENT_CONFIG_SIZE 11

extern config_t config;


void persistConfig();
void retrieveConfig();
void initialiseRNG();
void randomiseSearchAddr();
int8_t searchAddressCompare();
