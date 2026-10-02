#include "../src/app/app_09/infrared_file_utils.h"
#include <cstdio>
#include <cstdlib>

#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #expr); std::exit(1); } } while (0)

int main() {
    using namespace infrared_file;
    for (const char* name : {"TV_POWER", "ROOM-1", "Living.Room", "Signal"}) CHECK(validName(name));
    for (const char* name : {"", ".", "..", "../TV", "a/b", "a\\b", "TV.", "TV ", "a:b", "a*", "a?"})
        CHECK(!validName(name));
    CHECK(!validName(nullptr));
    for (int count=0; count<100; ++count) {
        for (int offset=-2; offset<110; ++offset) {
            for (int selected=-1; selected<8; ++selected) {
                int s=selected, o=offset;
                const int visible=listWindow(count,5,s,o);
                CHECK(visible>=0 && visible<=5 && o>=0);
                if (!count) { CHECK(s==0 && o==0 && visible==0); continue; }
                CHECK(s>=0 && s<visible && s+o<count);
                for (int row=0; row<visible; ++row) CHECK(row+o<count);
            }
        }
    }
    uint32_t value=0;
    CHECK(hexBytes("04 00 00 00",value) && value==4);
    CHECK(hexBytes("\tAB CD ef FF  ",value) && value==0xFFEFCDABu);
    CHECK(hexBytes("00",value) && value==0);
    for (const char* input : {"", " ", "ZZ", "01 ZZ", "100", "-1", "0x12", "01 02 03 04 05", "01,02"})
        CHECK(!hexBytes(input,value));
    CHECK(!hexBytes(nullptr,value));
    std::puts("Infrared: names, list refresh bounds and malformed hex regression tests passed");
}
