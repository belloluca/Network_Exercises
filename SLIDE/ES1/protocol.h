#define QUALITY 5

#pragma pack(push, 1)
struct Message {    
    int id; // 4 byte
    int temp; // 4 byte
    int hum; // 4 byte
    char air[5]; // 5 byte
    bool AlarmMode; // 1 byte
}; // 18 byte
#pragma pack(pop)