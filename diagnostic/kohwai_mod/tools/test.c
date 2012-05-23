#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "../src/kowhai.h"
#include "../src/kowhai_utils.h"
#include "../src/kowhai_protocol.h"
#include "../src/kowhai_protocol_server.h"
#include "../src/kowhai_serialize.h"
#include "xpsocket.h"
#include "beep.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

//
// treenode symbols
//

#include "symbols.h"

//
// settings tree descriptor
//

#define FLUX_CAP_COUNT 2
#define COEFF_COUNT    6
#define OWNER_MAX_LEN  12

struct kowhai_node_t settings_descriptor[] =
{
    { KOW_BRANCH_START,     SYM_SETTINGS,       1,                0 },

    { KOW_BRANCH_START,     SYM_FLUXCAPACITOR,  FLUX_CAP_COUNT,   0 },
    { KOW_CHAR,             SYM_OWNER,          OWNER_MAX_LEN,    0 },
    { KOW_UINT32,           SYM_FREQUENCY,      1,                0 },
    { KOW_UINT32,           SYM_GAIN,           1,                0 },
    { KOW_FLOAT,            SYM_COEFFICIENT,    COEFF_COUNT,      0 },
    { KOW_BRANCH_END,       SYM_FLUXCAPACITOR,  0,                0 },

    { KOW_BRANCH_START,     SYM_OVEN,           1,                0 },
    { KOW_INT16,            SYM_TEMP,           1,                0 },
    { KOW_UINT16,           SYM_TIMEOUT,        1,                0 },
    { KOW_BRANCH_END,       SYM_OVEN,           0,                0 },

    { KOW_BRANCH_END,       SYM_SETTINGS,       1,                0 },
};

//
// shadow tree descriptor
//

struct kowhai_node_t shadow_descriptor[] =
{
    { KOW_BRANCH_START,     SYM_SHADOW,         1,                0 },
    { KOW_UINT8,            SYM_RUNNING,        1,                0 },
    { KOW_UINT8,            SYM_STATUS,         1,                0 },
    { KOW_BRANCH_END,       SYM_SHADOW,         0,                0 },
};

//
// action tree descriptor
//

#define ACTION_START 1
#define ACTION_STOP  2
#define ACTION_BEEP  3

struct kowhai_node_t action_descriptor[] =
{
    { KOW_BRANCH_START,     SYM_ACTIONS,        1,                 0 },
    { KOW_BRANCH_START,     SYM_START,          1,                 ACTION_START },
    { KOW_UINT32,           SYM_DELAY,          1,                 0 },
    { KOW_BRANCH_END,       SYM_START,          0,                 0 },
    { KOW_BRANCH_START,     SYM_STOP,           1,                 ACTION_STOP },
    { KOW_BRANCH_END,       SYM_STOP,           0,                 0 },
    { KOW_BRANCH_START,     SYM_BEEP,           1,                 ACTION_BEEP},
    { KOW_INT32,            SYM_FREQUENCY,      1,                 0 },
    { KOW_INT32,            SYM_DURATION,       1,                 0 },
    { KOW_BRANCH_END,       SYM_BEEP,           0,                 0 },
    { KOW_BRANCH_END,       SYM_ACTIONS,        0,                 0 },
};

//
// scope tree descriptor
//

#define NUM_PIXELS 512

struct kowhai_node_t scope_descriptor[] =
{
    { KOW_BRANCH_START,     SYM_SCOPE,          1,                 0 },
    { KOW_UINT16,           SYM_PIXELS,         NUM_PIXELS,        0 },
    { KOW_BRANCH_END,       SYM_SCOPE,          0,                 0 },
};

//
// settings tree structs
//

#pragma pack(1)

struct flux_capacitor_t
{
    char owner[OWNER_MAX_LEN];
    uint32_t frequency;
    uint32_t gain;
    float coefficient[COEFF_COUNT];
};

struct oven_t
{
    int16_t temp;
    uint16_t timeout;
};

struct settings_data_t
{
    struct flux_capacitor_t flux_capacitor[FLUX_CAP_COUNT];
    struct oven_t oven;
};

//
// shadow tree stucts
//

struct shadow_data_t
{
    uint8_t running;
    uint8_t status;
};

//
// action tree stucts
//

struct start_t
{
    uint32_t delay;
};

struct beep_t
{
    int32_t freq;
    int32_t duration;
};

struct action_data_t
{
    struct start_t start;
    struct beep_t beep;
};

//
// scope tree stucts
//

struct scope_data_t
{
    uint16_t pixels[NUM_PIXELS];
};

#pragma pack()

//
// test commands
//

#define TEST_BASIC           0
#define TEST_PROTOCOL_SERVER 1
#define TEST_PROTOCOL_CLIENT 2

//
// test tree ids
//

#define TREE_ID_SETTINGS 0
#define TREE_ID_SHADOW   1
#define TREE_ID_ACTIONS  2
#define TREE_ID_SCOPE    3

//
// test trees
//

struct settings_data_t settings;
struct kowhai_tree_t settings_tree = {settings_descriptor, &settings};
struct shadow_data_t shadow;
struct kowhai_tree_t shadow_tree = {shadow_descriptor, &shadow};
struct action_data_t action = { 100, 500, 100 };
struct kowhai_tree_t action_tree = {action_descriptor, &action};
struct scope_data_t scope;
struct kowhai_tree_t scope_tree = {scope_descriptor, &scope};

//
// test protocol packet size
//

#define MAX_PACKET_SIZE 0x40

//
// test functions
//

union kowhai_symbol_t symbols1[] = {SYM_SETTINGS, SYM_OVEN, SYM_TEMP};
union kowhai_symbol_t symbols2[] = {SYM_SETTINGS, SYM_OVEN, SYM_TIMEOUT};
union kowhai_symbol_t symbols3[] = {SYM_SETTINGS, SYM_FLUXCAPACITOR};
union kowhai_symbol_t symbols4[] = {431, 12343};
union kowhai_symbol_t symbols5[] = {SYM_SHADOW, SYM_STATUS};
union kowhai_symbol_t symbols6[] = {SYM_SETTINGS, SYM_FLUXCAPACITOR, SYM_GAIN};
union kowhai_symbol_t symbols7[] = {SYM_SETTINGS, SYM_FLUXCAPACITOR, SYM_COEFFICIENT};
union kowhai_symbol_t symbols8[] = {SYM_SETTINGS, KOWHAI_SYMBOL(SYM_FLUXCAPACITOR, 1), SYM_GAIN};
union kowhai_symbol_t symbols9[] = {SYM_SETTINGS, KOWHAI_SYMBOL(SYM_FLUXCAPACITOR, 1), KOWHAI_SYMBOL(SYM_COEFFICIENT, 3)};
union kowhai_symbol_t symbols10[] = {SYM_SETTINGS, SYM_FLUXCAPACITOR, KOWHAI_SYMBOL(SYM_COEFFICIENT, 3)};
union kowhai_symbol_t symbols11[] = {SYM_SETTINGS, SYM_OVEN};
union kowhai_symbol_t symbols12[] = {SYM_SETTINGS, KOWHAI_SYMBOL(SYM_FLUXCAPACITOR, 1)};
union kowhai_symbol_t symbols13[] = {SYM_SETTINGS, KOWHAI_SYMBOL(SYM_FLUXCAPACITOR, 0), KOWHAI_SYMBOL(SYM_OWNER, 3)};

void core_tests()
{
    uint16_t offset;
    int size;
    int count;
    struct kowhai_node_t* node;

    uint8_t status;
    uint16_t temp;
    uint16_t timeout;
    uint32_t gain;
    float coeff;
    char owner_initial;
    struct flux_capacitor_t flux_capacitor = {"empty", 1, 2, 10, 20, 30, 40, 50, 60};

    // test tree parsing
    printf("test kowhai_get_node...\t\t\t");
    assert(kowhai_get_node(settings_tree.desc, 3, symbols1, &offset, &node) == KOW_STATUS_OK);
    assert(offset == offsetof(struct settings_data_t, oven.temp));
    assert(node == &settings_descriptor[8]);
    assert(kowhai_get_node(settings_tree.desc, 3, symbols2, &offset, &node) == KOW_STATUS_OK);
    assert(offset == offsetof(struct settings_data_t, oven.timeout));
    assert(node == &settings_descriptor[9]);
    assert(kowhai_get_node(settings_tree.desc, 2, symbols3, &offset, &node) == KOW_STATUS_OK);
    assert(offset == offsetof(struct settings_data_t, flux_capacitor));
    assert(node == &settings_descriptor[1]);
    assert(kowhai_get_node(settings_tree.desc, 2, symbols4, &offset, &node) == KOW_STATUS_INVALID_SYMBOL_PATH);
    assert(kowhai_get_node(settings_tree.desc, 3, symbols6, &offset, &node) == KOW_STATUS_OK);
    assert(offset == offsetof(struct settings_data_t, flux_capacitor[0].gain));
    assert(node == &settings_descriptor[4]);
    assert(kowhai_get_node(settings_tree.desc, 3, symbols8, &offset, &node) == KOW_STATUS_OK);
    assert(offset == offsetof(struct settings_data_t, flux_capacitor[1].gain));
    assert(node == &settings_descriptor[4]);
    assert(kowhai_get_node(settings_tree.desc, 3, symbols9, &offset, &node) == KOW_STATUS_OK);
    assert(offset == offsetof(struct settings_data_t, flux_capacitor[1].coefficient[3]));
    assert(node == &settings_descriptor[5]);
    assert(kowhai_get_node(settings_tree.desc, 3, symbols10, &offset, &node) == KOW_STATUS_OK);
    assert(offset == offsetof(struct settings_data_t, flux_capacitor[0].coefficient[3]));
    assert(node == &settings_descriptor[5]);
    assert(kowhai_get_node(settings_tree.desc, 2, symbols12, &offset, &node) == KOW_STATUS_OK);
    assert(offset == offsetof(struct settings_data_t, flux_capacitor[1]));
    assert(node == &settings_descriptor[1]);

    printf(" passed!\n");

    // test get node size
    printf("test kowhai_get_node_size & kowhai_get_node_count...\t\t");
    assert(kowhai_get_node_size(settings_tree.desc, &size) == KOW_STATUS_OK);
    assert(size == sizeof(struct settings_data_t));
    assert(kowhai_get_node_count(settings_tree.desc, &count) == KOW_STATUS_OK);
    assert(count == sizeof(settings_descriptor)/sizeof(struct kowhai_node_t));
    assert(kowhai_get_node_count(&settings_tree.desc[2], &count) == KOW_STATUS_OK);
    assert(count == 1);
    printf(" passed!\n");

    // test read/write settings
    printf("test kowhai_read/kowhai_write...\t");
    status = 1;
    shadow.status = 0;
    assert(kowhai_write(&shadow_tree, 2, symbols5, 0, &status, 1) == KOW_STATUS_OK);
    assert(shadow.status == 1);
    status = 0;
    assert(kowhai_read(&shadow_tree, 2, symbols5, 0, &status, 1) == KOW_STATUS_OK);
    assert(status == 1);
    timeout = 999;
    settings.oven.timeout = 0;
    assert(kowhai_write(&settings_tree, 3, symbols2, 0, &timeout, sizeof(timeout)) == KOW_STATUS_OK);
    assert(settings.oven.timeout == 999);
    timeout = 0;
    assert(kowhai_read(&settings_tree, 3, symbols2, 0, &timeout, sizeof(timeout)) == KOW_STATUS_OK);
    assert(timeout == 999);
    flux_capacitor.frequency = 100; flux_capacitor.gain = 200;
    settings.flux_capacitor[0].frequency = 0; settings.flux_capacitor[0].gain = 0;
    assert(kowhai_write(&settings_tree, 2, symbols3, 0, &flux_capacitor, sizeof(flux_capacitor)) == KOW_STATUS_OK);
    assert(settings.flux_capacitor[0].frequency == 100 && settings.flux_capacitor[0].gain == 200);
    flux_capacitor.frequency = 0; flux_capacitor.gain = 0;
    assert(kowhai_read(&settings_tree, 2, symbols3, 0, &flux_capacitor, sizeof(flux_capacitor)) == KOW_STATUS_OK);
    assert(flux_capacitor.frequency == 100 && flux_capacitor.gain == 200);
    coeff = 999.9f;
    settings.flux_capacitor[1].coefficient[3] = 0;
    assert(kowhai_write(&settings_tree, 3, symbols9, 0, &coeff, sizeof(coeff)) == KOW_STATUS_OK);
    assert(settings.flux_capacitor[1].coefficient[3] == 999.9f);
    coeff = 0;
    assert(kowhai_read(&settings_tree, 3, symbols9, 0, &coeff, sizeof(coeff)) == KOW_STATUS_OK);
    assert(coeff == 999.9f);
    printf(" passed!\n");

    // test set/get settings
    printf("test kowhai_get_xxx/kowhai_set_xxx...\t");
    shadow.status = 0;
    assert(kowhai_set_int8(&shadow_tree, 2, symbols5, 255) == KOW_STATUS_OK);
    assert(shadow.status == 255);
    assert(kowhai_get_int8(&shadow_tree, 2, symbols5, &status) == KOW_STATUS_OK);
    assert(status == 255);
    settings.oven.temp = 0;
    assert(kowhai_set_int16(&settings_tree, 3, symbols1, 999) == KOW_STATUS_OK);
    assert(settings.oven.temp == 999);
    assert(kowhai_get_int16(&settings_tree, 3, symbols1, &temp) == KOW_STATUS_OK);
    assert(temp == 999);
    settings.oven.timeout = 0;
    assert(kowhai_set_int16(&settings_tree, 3, symbols2, 999) == KOW_STATUS_OK);
    assert(settings.oven.timeout == 999);
    assert(kowhai_get_int16(&settings_tree, 3, symbols2, &timeout) == KOW_STATUS_OK);
    assert(timeout == 999);
    settings.flux_capacitor[0].gain = 0;
    assert(kowhai_set_int32(&settings_tree, 3, symbols6, 999) == KOW_STATUS_OK);
    assert(settings.flux_capacitor[0].gain == 999);
    assert(kowhai_get_int32(&settings_tree, 3, symbols6, &gain) == KOW_STATUS_OK);
    assert(gain == 999);
    settings.flux_capacitor[0].coefficient[0] = 0;
    assert(kowhai_set_float(&settings_tree, 3, symbols7, 999.9f) == KOW_STATUS_OK);
    assert(settings.flux_capacitor[0].coefficient[0] ==  999.9f);
    assert(kowhai_get_float(&settings_tree, 3, symbols7, &coeff) == KOW_STATUS_OK);
    assert(coeff == 999.9f);
    sprintf(settings.flux_capacitor[0].owner, "Dr brown");
    sprintf(settings.flux_capacitor[1].owner, "Marty McFly");
    assert(kowhai_set_char(&settings_tree, 3, symbols13, 'B') == KOW_STATUS_OK);
    assert(kowhai_get_char(&settings_tree, 3, symbols13, &owner_initial) == KOW_STATUS_OK);
    assert(owner_initial == 'B');
    printf(" passed!\n");
}

char* get_symbol_name(void* param, uint16_t symbol)
{
    return symbols[symbol];
}

void serialization_tests()
{
    int buf_size = 0x1000;
    int desc_size = buf_size;
    int data_size = buf_size;
    char* js = (char*)malloc(buf_size);
    char* scratch = (char*)malloc(buf_size);
    struct kowhai_node_t* desc = (struct kowhai_node_t*)malloc(desc_size);
    char* data = (char*)malloc(data_size);

    printf("test kowhai_serialize/kowhai_deserialize...\n");

    // kowhai_serialize
    assert(js != NULL && scratch != NULL && desc != NULL && data != NULL);
    buf_size = 100;
    assert(kowhai_serialize(settings_tree, js, &buf_size, NULL, get_symbol_name) == KOW_STATUS_TARGET_BUFFER_TOO_SMALL);
    buf_size = 0x1000;
    assert(kowhai_serialize(settings_tree, js, &buf_size, NULL, get_symbol_name) == KOW_STATUS_OK);
    printf("---\n");
    printf(js);
    printf("\n***\n");
    printf("js length: %d\n", strlen(js));
    printf("---\n");
    // kowhai_deserialize
    buf_size = 0x1000;
    desc_size = 10;
    data_size = 10;
    assert(kowhai_deserialize(js, scratch, 100, desc, &desc_size, data, &data_size) == KOW_STATUS_SCRATCH_TOO_SMALL);
    assert(kowhai_deserialize(js, scratch, buf_size, desc, &desc_size, data, &data_size) == KOW_STATUS_TARGET_BUFFER_TOO_SMALL);
    desc_size = buf_size / sizeof(struct kowhai_node_t);
    assert(kowhai_deserialize(js, scratch, buf_size, desc, &desc_size, data, &data_size) == KOW_STATUS_TARGET_BUFFER_TOO_SMALL);
    data_size = buf_size;
    assert(kowhai_deserialize(js, scratch, buf_size, desc, &desc_size, data, &data_size) == KOW_STATUS_OK);
    assert(desc_size == sizeof(settings_descriptor) / sizeof(struct kowhai_node_t));
    assert(data_size == sizeof(settings));

    printf(" passed!\n");
}

void diff_tests()
{
    printf("kowhai_diff tests!\n");
    printf("  TODO: implement me!!!!\n");
    getc(stdin);
    printf(" passed!\n");
}

void merge_tests(void)
{
    #pragma pack(1)

    struct kowhai_node_t old_settings_descriptor[] =
    {
        { KOW_BRANCH_START,     SYM_SETTINGS,       1,                0 },

        { KOW_BRANCH_START,     SYM_FLUXCAPACITOR,  FLUX_CAP_COUNT,   0 },
        { KOW_UINT32,           SYM_FREQUENCY,      1,                0 },
        { KOW_UINT32,           SYM_GAIN,           1,                0 },
        { KOW_FLOAT,            SYM_COEFFICIENT,    COEFF_COUNT,      0 },
        { KOW_BRANCH_END,       SYM_FLUXCAPACITOR,  0,                0 },

        { KOW_BRANCH_START,     SYM_OVEN,           1,                0 },
        { KOW_INT16,            SYM_TEMP,           1,                0 },
        { KOW_UINT16,           SYM_TIMEOUT,        1,                0 },
        { KOW_BRANCH_END,       SYM_OVEN,           0,                0 },

        { KOW_BRANCH_END,       SYM_SETTINGS,       1,                0 },
    };
    
    struct old_settings_data_t
    {
        struct flux_capacitor_t
        {
            uint32_t frequency;
            uint32_t gain;
            float coefficient[COEFF_COUNT];
        } flux_capacitor [FLUX_CAP_COUNT];

        struct oven_t
        {
            int16_t temp;
            uint16_t timeout;
        } oven;
    };

    struct old_settings_data_t old_settings =
    {
        // flux capacitor array
        {
            { 1000, 200, {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f} },
            { 3000, 400, {0.6f, 0.5f, 0.4f, 0.3f, 0.2f, 0.1f} }
        },
        // oven
        { 180, 30 }
    };

    struct kowhai_node_t new_settings_descriptor[] =
    {
        { KOW_BRANCH_START,     SYM_SETTINGS,       1,                0 },

        { KOW_BRANCH_START,     SYM_FLUXCAPACITOR,  FLUX_CAP_COUNT,   0 },
        { KOW_UINT32,           SYM_FREQUENCY,      1,                0 },
        { KOW_UINT8,            SYM_RUNNING,        1,                0 },
        { KOW_FLOAT,            SYM_COEFFICIENT,    COEFF_COUNT,      0 },
        { KOW_BRANCH_END,       SYM_FLUXCAPACITOR,  0,                0 },

        { KOW_BRANCH_START,     SYM_OVEN,           1,                0 },
        { KOW_INT16,            SYM_TEMP,           1,                0 },
        { KOW_INT32,            SYM_BEEP,           2,                0 },
        { KOW_UINT16,           SYM_TIMEOUT,        1,                0 },
        { KOW_BRANCH_END,       SYM_OVEN,           0,                0 },

        { KOW_BRANCH_END,       SYM_SETTINGS,       1,                0 },
    };

    struct new_settings_data_t
    {
        struct new_flux_capacitor_t
        {
            uint32_t frequency;
            uint8_t running;
            float coefficient[COEFF_COUNT];
        } flux_capacitor [FLUX_CAP_COUNT];

        struct new_oven_t
        {
            int16_t temp;
            uint32_t beep[2];
            uint16_t timeout;
        } oven;
    };

    struct new_settings_data_t new_settings =
    {
        // flux capacitor array
        {
            { 5000, 0, {-0.1f, -0.2f, -0.3f, -0.4f, -0.5f, -0.6f} },
            { 6000, 1, {-0.6f, -0.5f, -0.4f, -0.3f, -0.2f, -0.1f} }
        },
        // oven
        { 220, {10000, 5000}, 20 }
    };

    struct kowhai_tree_t old_settings_tree = {old_settings_descriptor, &old_settings};
    struct kowhai_tree_t new_settings_tree = {new_settings_descriptor, &new_settings};

    int cap;
    int coeff;

    #pragma pack()

    printf("test kowhai kowhai_merge...\t");

    // test the merge of old into new
    assert(kowhai_merge(&new_settings_tree, &old_settings_tree) == KOW_STATUS_OK);

    // test things that should not have been merged are untouched
    assert(new_settings.flux_capacitor[0].running == 0);
    assert(new_settings.flux_capacitor[1].running == 1);
    assert(new_settings.oven.beep[0] == 10000);
    assert(new_settings.oven.beep[1] == 5000);

    // check that things that should be merged are
    for (cap = 0; cap < FLUX_CAP_COUNT; cap++)
    {
        assert(new_settings.flux_capacitor[cap].frequency == old_settings.flux_capacitor[cap].frequency);
        for (coeff = 0; coeff < COEFF_COUNT; coeff++)
            assert(new_settings.flux_capacitor[cap].coefficient[coeff] == old_settings.flux_capacitor[cap].coefficient[coeff]);
    }
    assert(new_settings.oven.temp == old_settings.oven.temp);
    assert(new_settings.oven.timeout == old_settings.oven.timeout);
    
    printf(" passed!\n");
}

void create_symbol_path_tests()
{
#define SYM_PATH_SIZE 5
#define SYM_PATH_SIZE_RESULT 3
    union kowhai_symbol_t symbol_path[SYM_PATH_SIZE] = {0};
    union kowhai_symbol_t symbol_path_settings_oven_temp[SYM_PATH_SIZE] = {SYM_SETTINGS, SYM_OVEN, SYM_TEMP};
    union kowhai_symbol_t symbol_path_settings_fluxcap_gain[SYM_PATH_SIZE] = {SYM_SETTINGS, SYM_FLUXCAPACITOR, SYM_GAIN};
    int symbol_path_size;
    int result;

    printf("kowhai_create_symbol_path* tests!\n");

    symbol_path_size = 2;
    result = kowhai_create_symbol_path(settings_descriptor, &settings_descriptor[8] /* settings.oven.temp */, symbol_path, &symbol_path_size);
    assert(result == KOW_STATUS_TARGET_BUFFER_TOO_SMALL);
    symbol_path_size = SYM_PATH_SIZE;
    result = kowhai_create_symbol_path(settings_descriptor, &settings_descriptor[8] /* settings.oven.temp */, symbol_path, &symbol_path_size);
    assert(result == KOW_STATUS_OK);
    assert(memcmp(symbol_path, symbol_path_settings_oven_temp, SYM_PATH_SIZE_RESULT * sizeof(union kowhai_symbol_t)) == 0);
    assert(symbol_path_size == SYM_PATH_SIZE_RESULT);
    symbol_path_size = SYM_PATH_SIZE;
    result = kowhai_create_symbol_path(settings_descriptor, &settings_descriptor[4] /* settings.fluxcap.gain */, symbol_path, &symbol_path_size);
    assert(result == KOW_STATUS_OK);
    assert(memcmp(symbol_path, symbol_path_settings_fluxcap_gain, SYM_PATH_SIZE_RESULT * sizeof(union kowhai_symbol_t)) == 0);
    assert(symbol_path_size == SYM_PATH_SIZE_RESULT);

    symbol_path_size = 2;
    result = kowhai_create_symbol_path2(&settings_tree, &settings.oven.temp, symbol_path, &symbol_path_size);
    assert(result == KOW_STATUS_TARGET_BUFFER_TOO_SMALL);
    symbol_path_size = SYM_PATH_SIZE;
    result = kowhai_create_symbol_path2(&settings_tree, &settings.oven.temp, symbol_path, &symbol_path_size);
    assert(result == KOW_STATUS_OK);
    assert(memcmp(symbol_path, symbol_path_settings_oven_temp, SYM_PATH_SIZE_RESULT * sizeof(union kowhai_symbol_t)) == 0);
    assert(symbol_path_size == SYM_PATH_SIZE_RESULT);
    symbol_path_size = SYM_PATH_SIZE;
    symbol_path_settings_fluxcap_gain[1].symbol = KOWHAI_SYMBOL(SYM_FLUXCAPACITOR, 1);
    result = kowhai_create_symbol_path2(&settings_tree, &settings.flux_capacitor[1].gain, symbol_path, &symbol_path_size);
    assert(result == KOW_STATUS_OK);
    assert(memcmp(symbol_path, symbol_path_settings_fluxcap_gain, SYM_PATH_SIZE_RESULT * sizeof(union kowhai_symbol_t)) == 0);
    assert(symbol_path_size == SYM_PATH_SIZE_RESULT);
    symbol_path_size = SYM_PATH_SIZE;
    symbol_path_settings_fluxcap_gain[2].symbol = KOWHAI_SYMBOL(SYM_COEFFICIENT, 4);
    result = kowhai_create_symbol_path2(&settings_tree, &settings.flux_capacitor[1].coefficient[4], symbol_path, &symbol_path_size);
    assert(result == KOW_STATUS_OK);
    assert(memcmp(symbol_path, symbol_path_settings_fluxcap_gain, SYM_PATH_SIZE_RESULT * sizeof(union kowhai_symbol_t)) == 0);
    assert(symbol_path_size == SYM_PATH_SIZE_RESULT);

    printf(" passed!\n");
}

void node_written(void* param, struct kowhai_node_t* node)
{
    switch (node->tag)
    {
        case ACTION_START:
            shadow.running = 1;
            shadow.status = rand();
            break;
        case ACTION_STOP:
            shadow.running = 0;
            shadow.status = 0;
            break;
        case ACTION_BEEP:
            beep(action.beep.freq, action.beep.duration);
            break;
    }
}

void server_buffer_send(void* param, void* buffer, size_t buffer_size)
{
    xpsocket_handle conn = (xpsocket_handle)param;
    xpsocket_send(conn, buffer, buffer_size);
}

void server_buffer_received(xpsocket_handle conn, void* param, void* buffer, int buffer_size)
{
    int i;
    struct kowhai_protocol_server_t* server = (struct kowhai_protocol_server_t*)param;
    server->send_packet_param = conn;

    // randomize the scope buffer for funzies
    for (i = 0; i < NUM_PIXELS; i++)
        scope.pixels[i] = rand();

    kowhai_server_process_packet(server, buffer, buffer_size);
}

void test_server_protocol()
{
    char packet_buffer[MAX_PACKET_SIZE];
    struct kowhai_node_t* tree_descriptors[] = {settings_descriptor, shadow_descriptor, action_descriptor, scope_descriptor};
    size_t tree_descriptor_sizes[] = {sizeof(settings_descriptor), sizeof(shadow_descriptor), sizeof(action_descriptor), sizeof(scope_descriptor)};
    void* tree_data_buffers[] = {&settings, &shadow, &action, &scope};
    struct kowhai_protocol_server_t server = {MAX_PACKET_SIZE, packet_buffer, node_written, NULL, server_buffer_send, NULL, 4, tree_descriptors, tree_descriptor_sizes, tree_data_buffers};
    printf("test server protocol...\n");
    xpsocket_init();
    xpsocket_serve(server_buffer_received, &server, MAX_PACKET_SIZE);
    xpsocket_cleanup();

}

void test_client_protocol()
{
    xpsocket_handle conn;
    printf("test client protocol...\n");
    xpsocket_init();
    conn = xpsocket_init_client();
    if (conn != NULL)
    {
        char buffer[MAX_PACKET_SIZE];
        int bytes_required;
        int received_size;
        struct kowhai_protocol_t prot;
        int overhead;
        int offset, size, temp;
        char value;
        struct oven_t oven = {0x0102, 321};
        struct flux_capacitor_t flux_cap[2] = {{{"Marty Mc Fly"}, 100, 200, {1, 2, 3, 4, 5, 6}}, {{"Dr Brown"}, 110, 210, {11, 12, 13, 14, 15, 16}}};
        // write oven.temp
        temp = 25;
        POPULATE_PROTOCOL_WRITE(prot, TREE_ID_SETTINGS, KOW_CMD_WRITE_DATA, 3, symbols1, KOW_INT16, 0, sizeof(uint16_t), &temp);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.tree_id == TREE_ID_SETTINGS);
        assert(prot.header.command == KOW_CMD_WRITE_DATA_ACK);
        assert(prot.payload.spec.data.symbols.count == 3);
        assert(memcmp(prot.payload.spec.data.symbols.array_, symbols1, sizeof(union kowhai_symbol_t) * 3) == 0);
        assert(prot.payload.spec.data.memory.type == KOW_INT16);
        assert(prot.payload.spec.data.memory.offset == 0);
        assert(prot.payload.spec.data.memory.size == sizeof(uint16_t));
        assert(*((uint16_t*)prot.payload.buffer) == temp);
        // write low byte of oven.temp
        value = 255;
        POPULATE_PROTOCOL_WRITE(prot, TREE_ID_SETTINGS, KOW_CMD_WRITE_DATA, 3, symbols1, KOW_INT16, 1, 1, &value);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.tree_id == TREE_ID_SETTINGS);
        assert(prot.header.command == KOW_CMD_WRITE_DATA_ACK);
        assert(prot.payload.spec.data.symbols.count == 3);
        assert(memcmp(prot.payload.spec.data.symbols.array_, symbols1, sizeof(union kowhai_symbol_t) * 3) == 0);
        assert(prot.payload.spec.data.memory.type == KOW_INT16);
        assert(prot.payload.spec.data.memory.offset == 1);
        assert(prot.payload.spec.data.memory.size == 1);
        assert(*((char*)prot.payload.buffer) == value);
        // double check oven.temp
        POPULATE_PROTOCOL_READ(prot, TREE_ID_SETTINGS, KOW_CMD_READ_DATA, 3, symbols1);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(*((uint16_t*)prot.payload.buffer) >> 8 == (uint8_t)value);
        // write oven
        POPULATE_PROTOCOL_WRITE(prot, TREE_ID_SETTINGS, KOW_CMD_WRITE_DATA, 2, symbols11, 0, 0, sizeof(oven), &oven);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.tree_id == TREE_ID_SETTINGS);
        assert(prot.header.command == KOW_CMD_WRITE_DATA_ACK);
        assert(prot.payload.spec.data.symbols.count == 2);
        assert(memcmp(prot.payload.spec.data.symbols.array_, symbols11, sizeof(union kowhai_symbol_t) * 2) == 0);
        assert(prot.payload.spec.data.memory.type == 0);
        assert(prot.payload.spec.data.memory.offset == 0);
        assert(prot.payload.spec.data.memory.size == sizeof(oven));
        assert(memcmp(prot.payload.buffer, &oven, sizeof(oven)) == 0);
        // write flux capacitor array
        POPULATE_PROTOCOL_WRITE(prot, TREE_ID_SETTINGS, KOW_CMD_WRITE_DATA, 2, symbols3, 0, 0, sizeof(struct flux_capacitor_t) * 2, flux_cap);
        kowhai_protocol_get_overhead(&prot, &overhead);
        prot.payload.spec.data.memory.size = MAX_PACKET_SIZE - overhead;
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.tree_id == TREE_ID_SETTINGS);
        assert(prot.header.command == KOW_CMD_WRITE_DATA_ACK);
        assert(prot.payload.spec.data.symbols.count == 2);
        assert(memcmp(prot.payload.spec.data.symbols.array_, symbols3, sizeof(union kowhai_symbol_t) * 2) == 0);
        assert(prot.payload.spec.data.memory.type == 0);
        assert(prot.payload.spec.data.memory.offset == 0);
        assert(prot.payload.spec.data.memory.size == MAX_PACKET_SIZE - overhead);
        assert(memcmp(prot.payload.buffer, flux_cap, MAX_PACKET_SIZE - overhead) == 0);
        offset = prot.payload.spec.data.memory.size;
        size = sizeof(struct flux_capacitor_t) * 2 - offset;
        POPULATE_PROTOCOL_WRITE(prot, TREE_ID_SETTINGS, KOW_CMD_WRITE_DATA, 2, symbols3, 0, offset, size, (char*)flux_cap + offset);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.command == KOW_CMD_WRITE_DATA_ACK);
        assert(prot.payload.spec.data.memory.offset == offset);
        assert(prot.payload.spec.data.memory.size == size);
        assert(memcmp(prot.payload.buffer, (char*)flux_cap + offset, size) == 0);
        // write flux capacitor[1]
        POPULATE_PROTOCOL_WRITE(prot, TREE_ID_SETTINGS, KOW_CMD_WRITE_DATA, 2, symbols12, 0, 0, sizeof(struct flux_capacitor_t), &flux_cap[1]);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.tree_id == TREE_ID_SETTINGS);
        assert(prot.header.command == KOW_CMD_WRITE_DATA_ACK);
        assert(prot.payload.spec.data.symbols.count == 2);
        assert(memcmp(prot.payload.spec.data.symbols.array_, symbols12, sizeof(union kowhai_symbol_t) * 2) == 0);
        assert(prot.payload.spec.data.memory.type == 0);
        assert(prot.payload.spec.data.memory.offset == 0);
        assert(prot.payload.spec.data.memory.size == sizeof(struct flux_capacitor_t));
        assert(memcmp(prot.payload.buffer, &flux_cap[1], sizeof(struct flux_capacitor_t)) == 0);
        // read oven.temp
        POPULATE_PROTOCOL_READ(prot, TREE_ID_SETTINGS, KOW_CMD_READ_DATA, 3, symbols1);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.tree_id == TREE_ID_SETTINGS);
        assert(prot.header.command == KOW_CMD_READ_DATA_ACK_END);
        assert(prot.payload.spec.data.symbols.count == 3);
        assert(memcmp(prot.payload.spec.data.symbols.array_, symbols1, sizeof(union kowhai_symbol_t) * 3) == 0);
        assert(prot.payload.spec.data.memory.type == KOW_INT16);
        assert(prot.payload.spec.data.memory.offset == 0);
        assert(prot.payload.spec.data.memory.size == sizeof(int16_t));
        assert(*((int16_t*)prot.payload.buffer) == 0x0102);
        // read flux capacitor array
        POPULATE_PROTOCOL_READ(prot, TREE_ID_SETTINGS, KOW_CMD_READ_DATA, 2, symbols3);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.tree_id == TREE_ID_SETTINGS);
        assert(prot.header.command == KOW_CMD_READ_DATA_ACK);
        assert(prot.payload.spec.data.symbols.count == 2);
        assert(memcmp(prot.payload.spec.data.symbols.array_, symbols3, sizeof(union kowhai_symbol_t) * 2) == 0);
        assert(prot.payload.spec.data.memory.type == 0);
        assert(prot.payload.spec.data.memory.offset == 0);
        kowhai_protocol_get_overhead(&prot, &overhead);
        assert(overhead == 17);
        assert(prot.payload.spec.data.memory.size == MAX_PACKET_SIZE - overhead);
        assert(memcmp(prot.payload.buffer, flux_cap, prot.payload.spec.data.memory.size) == 0);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.tree_id == TREE_ID_SETTINGS);
        assert(prot.header.command == KOW_CMD_READ_DATA_ACK_END);
        assert(prot.payload.spec.data.symbols.count == 2);
        assert(memcmp(prot.payload.spec.data.symbols.array_, symbols3, sizeof(union kowhai_symbol_t) * 2) == 0);
        assert(prot.payload.spec.data.memory.type == 0);
        assert(prot.payload.spec.data.memory.offset == MAX_PACKET_SIZE - overhead);
        assert(prot.payload.spec.data.memory.size == sizeof(struct flux_capacitor_t) * 2 - prot.payload.spec.data.memory.offset);
        assert(memcmp(prot.payload.buffer, (char*)flux_cap + prot.payload.spec.data.memory.offset, prot.payload.spec.data.memory.size) == 0);
        // read settings tree descriptor
        POPULATE_PROTOCOL_CMD(prot, TREE_ID_SETTINGS, KOW_CMD_READ_DESCRIPTOR);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.tree_id == TREE_ID_SETTINGS);
        assert(prot.header.command == KOW_CMD_READ_DESCRIPTOR_ACK);
        assert(prot.payload.spec.descriptor.node_count == sizeof(settings_descriptor) / sizeof(settings_descriptor[0]));
        assert(prot.payload.spec.data.memory.offset == 0);
        kowhai_protocol_get_overhead(&prot, &overhead);
        assert(overhead == 8);
        assert(prot.payload.spec.descriptor.size == MAX_PACKET_SIZE - overhead);
        assert(memcmp(prot.payload.buffer, settings_descriptor, prot.payload.spec.data.memory.size) == 0);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.tree_id == TREE_ID_SETTINGS);
        assert(prot.header.command == KOW_CMD_READ_DESCRIPTOR_ACK_END);
        assert(prot.payload.spec.descriptor.node_count == sizeof(settings_descriptor) / sizeof(settings_descriptor[0]));
        assert(prot.payload.spec.descriptor.offset == MAX_PACKET_SIZE - overhead);
        assert(prot.payload.spec.descriptor.size == sizeof(settings_descriptor) - prot.payload.spec.descriptor.offset);
        assert(memcmp(prot.payload.buffer, (char*)settings_descriptor + prot.payload.spec.data.memory.offset, prot.payload.spec.data.memory.size) == 0);
        // test invalid tree id
        POPULATE_PROTOCOL_CMD(prot, 255, KOW_CMD_READ_DESCRIPTOR);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.tree_id == 255);
        assert(prot.header.command == KOW_CMD_ERROR_INVALID_TREE_ID);
        // test invalid command
        POPULATE_PROTOCOL_CMD(prot, TREE_ID_SETTINGS, KOW_CMD_READ_DESCRIPTOR);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        buffer[1] = 255;
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.tree_id == TREE_ID_SETTINGS);
        assert(prot.header.command == KOW_CMD_ERROR_INVALID_COMMAND);
        // test invalid symbol path
        POPULATE_PROTOCOL_READ(prot, TREE_ID_SETTINGS, KOW_CMD_READ_DATA, 2, symbols4);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.tree_id == TREE_ID_SETTINGS);
        assert(prot.header.command == KOW_CMD_ERROR_INVALID_SYMBOL_PATH);
        POPULATE_PROTOCOL_WRITE(prot, TREE_ID_SETTINGS, KOW_CMD_WRITE_DATA, 2, symbols4, KOW_INT16, 0, sizeof(uint16_t), &temp);
        assert(kowhai_protocol_create(buffer, MAX_PACKET_SIZE, &prot, &bytes_required) == KOW_STATUS_OK);
        xpsocket_send(conn, buffer, bytes_required);
        memset(buffer, 0, MAX_PACKET_SIZE);
        xpsocket_receive(conn, buffer, MAX_PACKET_SIZE, &received_size);
        kowhai_protocol_parse(buffer, received_size, &prot);
        assert(prot.header.tree_id == TREE_ID_SETTINGS);
        assert(prot.header.command == KOW_CMD_ERROR_INVALID_SYMBOL_PATH);

        xpsocket_free_client(conn);
    }
    xpsocket_cleanup();
    printf("\t\t\t\t\t passed!\n");
}

//
// main
//

int main(int argc, char* argv[])
{
    int test_command = TEST_BASIC;

#ifdef KOWHAI_DBG
    printf("kowhai debugging enabled!\n");
#endif

    // determine test command
    if (argc > 1)
    {
        if (strcmp("server", argv[1]) == 0)
            test_command = TEST_PROTOCOL_SERVER;
        else if (strcmp("client", argv[1]) == 0)
            test_command = TEST_PROTOCOL_CLIENT;
    }

    // core tests
    core_tests();
    // test serialization
    serialization_tests();
    // test utils
    diff_tests();
    merge_tests();
    create_symbol_path_tests();
    // test server protocol
    if (test_command == TEST_PROTOCOL_SERVER)
        test_server_protocol();
    // test client protocol
    if (test_command == TEST_PROTOCOL_CLIENT)
        test_client_protocol();

    return 0;
}
