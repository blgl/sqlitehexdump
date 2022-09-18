/* Copyright 2022 Bo Lindbergh */

#include <limits.h>
#include <string.h>
#include <sqlite3ext.h>

SQLITE_EXTENSION_INIT1

typedef struct hexdump_state {
    unsigned char const *data;
    int offsetwidth;
    unsigned int width;
    unsigned int backwards;
    sqlite3_str *str;
} hexdump_state;

static void append_line(
    hexdump_state *state,
    size_t offset,
    unsigned int length)
{
    sqlite3_str *str = state->str;
    unsigned char const *data = state->data+offset;
    unsigned int width = state->width;
    unsigned int pos;

    sqlite3_str_appendf(str, "%.*llX  ", state->offsetwidth, offset);
    if (state->backwards) {
        if (length<width)
            sqlite3_str_appendchar(str, (width-length)*3, ' ');
        for (pos = length; pos-->0; )
            sqlite3_str_appendf(str, "%.2X ", data[pos]);
    } else {
        for (pos = 0; pos<length; pos++)
            sqlite3_str_appendf(str, "%.2X ", data[pos]);
        if (length<width)
            sqlite3_str_appendchar(str, (width-length)*3, ' ');
    }
    sqlite3_str_appendchar(str, 1, ' ');
    for (pos = 0; pos<length; pos++) {
        unsigned int c;

        c = data[pos];
        if (c<=0x1F || c>=0x7F)
            c = '.';
        sqlite3_str_appendchar(str, 1, c);
    }
    if (length<width)
        sqlite3_str_appendchar(str, width-length, ' ');
    sqlite3_str_appendchar(str, 1, '\n');
}

static void hexdump_func(
    sqlite3_context *context,
    int n,
    sqlite3_value **args)
{
    hexdump_state state;
    unsigned char const *data;
    size_t datasize, offset;
    unsigned int width;
    sqlite3_str *str = NULL;
    int skipping;
    int status;

    if (n>=1) {
        sqlite3_value *dataarg = args[0];

        if (sqlite3_value_type(dataarg)==SQLITE_NULL)
            return;
        state.data = data = sqlite3_value_blob(dataarg);
        datasize = sqlite3_value_bytes(dataarg);
        if (datasize>0 && !data) {
            sqlite3_result_error_nomem(context);
            return;
        }
    } else {
        sqlite3_result_error_code(context, SQLITE_INTERNAL);
        return;
    }
    width = 16;
    state.backwards = 0;
    if (n>=2) {
        sqlite3_value *widtharg = args[1];
        sqlite3_int64 widthval;

        switch (sqlite3_value_type(widtharg)) {
        case SQLITE_NULL:
            break;
        case SQLITE_INTEGER:
        case SQLITE_FLOAT:
            widthval = sqlite3_value_int64(widtharg);
            if (!widthval) {
                sqlite3_result_error(
                    context, "hexdump width must be non-zero", -1);
                return;
            }
            if (widthval>INT_MAX || widthval<-INT_MAX) {
                sqlite3_result_error(context, "excessive hexdump width", -1);
                return;
            }
            if (widthval<0) {
                width = -widthval;
                state.backwards = 1;
            } else {
                width = widthval;
            }
            break;
        default:
            sqlite3_result_error(context, "hexdump width must be numeric", -1);
            return;
        }
    }
    state.width = width;
    {
        char buf[32];

        sqlite3_snprintf(sizeof buf, buf, "%llX", (sqlite3_uint64)datasize);
        state.offsetwidth = strlen(buf);
    }

    state.str = str = sqlite3_str_new(sqlite3_context_db_handle(context));
    offset = 0;
    skipping = 0;
    while (datasize>=width) {
        if (offset>0 && !memcmp(data+offset, data+offset-width, width)) {
            skipping = 1;
        } else {
            if (skipping) {
                sqlite3_str_append(str, "*\n", 2);
                skipping = 0;
            }
        }
        if (!skipping)
            append_line(&state, offset, width);
        offset += width;
        datasize -= width;
    }
    if (skipping) {
        sqlite3_str_append(str, "*\n", 2);
        skipping = 0;
    }
    if (datasize>0) {
        append_line(&state, offset, datasize);
        offset += datasize;
        datasize = 0;
    }
    sqlite3_str_appendf(
        str, "%.*llX", state.offsetwidth, (sqlite3_uint64)offset);
    status = sqlite3_str_errcode(str);
    if (status==SQLITE_OK) {
        void *dump;
        size_t dumpsize;

        dumpsize = sqlite3_str_length(str);
        dump = sqlite3_str_finish(str);
        if (!dump) {
            sqlite3_result_error_nomem(context);
        } else {
            sqlite3_result_text(context, dump, dumpsize, sqlite3_free);
        }
    } else {
        sqlite3_str_reset(str);
        sqlite3_str_finish(str);
        sqlite3_result_error_code(context, status);
    }
}

#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_extension_init(
    sqlite3 *db,
    char **errmsgOut,
    sqlite3_api_routines const *api)
{
    int status;

    (void)errmsgOut;
    SQLITE_EXTENSION_INIT2(api);
    status = sqlite3_create_function(
        db,
        "hexdump",
        1,
        SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
        NULL,
        hexdump_func,
        0,
        0);
    if (status!=SQLITE_OK)
        goto bail;
    status = sqlite3_create_function(
        db,
        "hexdump",
        2,
        SQLITE_UTF8 | SQLITE_DETERMINISTIC | SQLITE_INNOCUOUS,
        NULL,
        hexdump_func,
        0,
        0);
bail:
    if (status!=SQLITE_OK && status!=SQLITE_OK_LOAD_PERMANENTLY)
        *errmsgOut = sqlite3_mprintf("%s", sqlite3_errstr(status));
    return status;
}

