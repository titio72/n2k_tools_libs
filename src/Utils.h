#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <time.h>
#include <cstring>
#include <stdint.h>
#include <stdio.h>

class N2KSid
{
public:
    N2KSid();
    unsigned char getNew();
    unsigned char getCurrent();

private:
    unsigned char sid;
};

class ByteBuffer
{
public:
    ByteBuffer(const ByteBuffer &b)
    {
        auto_expand = b.auto_expand;
        buf_size = b.buf_size;
        buffer = new uint8_t[buf_size];
        offset = b.offset;
        memcpy(buffer, b.buffer, offset);
    }

    ByteBuffer(size_t size, bool auto_expand = false) : buf_size(size), offset(0), auto_expand(auto_expand)
    {
        buffer = new uint8_t[size];
    }

    ByteBuffer(void *data, size_t size, bool auto_expand = false) : buf_size(size), offset(size), auto_expand(auto_expand)
    {
        buffer = new uint8_t[size];
        memcpy(buffer, data, size);
    }

    virtual ~ByteBuffer()
    {
        delete buffer;
    }

    ByteBuffer &operator=(const ByteBuffer &b)
    {
        if (this != &b)
        {
            delete buffer;
            auto_expand = b.auto_expand;
            buf_size = b.buf_size;
            buffer = new uint8_t[buf_size];
            offset = b.offset;
            memcpy(buffer, b.buffer, offset);
        }
        return *this;
    }

    bool resize(size_t new_size)
    {
        if (new_size > buf_size)
        {
            //printf("Resizing buffer from %zu to %zu\n", buf_size, new_size);
            if (!auto_expand)
                return false;

            uint8_t* old_buffer = buffer;
            buffer = new uint8_t[new_size];
            if (buffer == nullptr)
            {
                buffer = old_buffer; // restore old buffer on failure
                return false;
            }
            memcpy(buffer, old_buffer, offset);
            delete[] old_buffer;
            buf_size = new_size;
            printf("Resized buffer to %zu\n", buf_size);
            return true;
        }
        return true;
    }

    ByteBuffer &operator<<(char *t)
    {
        return (*this) << (const char *)t;
    }

    ByteBuffer &operator<<(const char *t)
    {
        size_t t_size = strlen(t);
        resize(offset + t_size + 1);
        if (t_size < 255 && (offset + t_size) < buf_size)
        {
            *this << (uint8_t)t_size;
            memcpy(buffer + offset, t, t_size);
            offset += t_size;
        }
        return *this;
    }

    template <typename T>
    ByteBuffer &operator<<(const T &t)
    {
        size_t t_size = sizeof(T);
        //printf("Appending %zu bytes\n", t_size);
        resize(offset + t_size);
        if (offset + t_size <= buf_size)
        {
            memcpy(buffer + offset, &t, t_size);
            offset += t_size;
        }
        return *this;
    }

    ByteBuffer &reset()
    {
        offset = 0;
        return *this;
    }

    void get_data(uint8_t *dest, size_t len) const
    {
        if (len > offset)
            len = offset;
        if (offset == 0)
            return;
        memcpy(dest, buffer, len);
    }

    uint8_t *data() { return buffer; }
    size_t size() const { return buf_size; }
    size_t length() const { return offset; }

    bool operator==(const ByteBuffer &other) const
    {
        if (offset != other.offset)
            return false;
        return (memcmp(buffer, other.buffer, offset) == 0);
    }

private:
    uint8_t *buffer;
    size_t buf_size;
    size_t offset;
    bool auto_expand;
};

typedef ByteBuffer *ByteBufferPtr;

bool startswith(const char *str_to_find, const char *str);
int getDaysSince1970(int y, int m, int d);
const char *time_to_ISO(time_t t, int millis);
char *replace(char const *const original, char const *const pattern, char const *const replacement, bool first = false);
int indexOf(const char *haystack, const char *needle);

double norm_deg(double d);
int16_t norm_deg(int16_t d);

template <typename T>
bool array_contains(T test, T *int_set, int sz)
{
    if (int_set == NULL || sz <= 0)
        return false;

    for (int i = 0; i < sz; i++)
    {
        if (test == int_set[i])
            return true;
    }
    return false;
}

ulong _millis();
int msleep(long msec);
unsigned long get_free_mem();
unsigned long check_elapsed(ulong time, ulong &last_time, ulong period);
void format_thousands_sep(char *buffer, long l);
double lpf(double value, double previous_value, double alpha);

#endif
