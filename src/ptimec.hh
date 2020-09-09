/**
 * Copyright (c) 2014, Timothy Stack
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * * Neither the name of Timothy Stack nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @file ptimec.hh
 */

#ifndef pctimec_hh
#define pctimec_hh

// XXX
#define __STDC_FORMAT_MACROS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <inttypes.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <cstdlib>
#include <iomanip>
#include <ostream>

struct tm *secs2tm(time_t *tim_p, struct tm *res);
time_t tm2sec(const struct tm *t);

enum exttm_bits_t {
    ETB_YEAR_SET,
    ETB_MONTH_SET,
    ETB_DAY_SET,
    ETB_MACHINE_ORIENTED,
    ETB_EPOCH_TIME,
};

enum exttm_flags_t {
    ETF_YEAR_SET = (1UL << ETB_YEAR_SET),
    ETF_MONTH_SET = (1UL << ETB_MONTH_SET),
    ETF_DAY_SET = (1UL << ETB_DAY_SET),
    ETF_MACHINE_ORIENTED = (1UL << ETB_MACHINE_ORIENTED),
    ETF_EPOCH_TIME = (1UL << ETB_EPOCH_TIME),
};

struct exttm {
    struct tm et_tm;
    int32_t et_nsec;
    unsigned int et_flags;
    long et_gmtoff;

    bool operator==(const exttm &other) const {
        return memcmp(this, &other, sizeof(exttm)) == 0;
    };

    struct timeval to_timeval() const {
        struct timeval retval;

        retval.tv_sec = tm2sec(&this->et_tm);
        retval.tv_usec = this->et_nsec * 1000;

        return retval;
    };
};

std::ostream& operator<< (std::ostream& os, const exttm& value);

#define PTIME_CONSUME(amount, block) \
    if ((off_inout + amount) > len) { \
        return false; \
    } \
    \
    block \
    \
    off_inout += amount;

#define PTIME_APPEND(ch) \
    if ((off_inout + 2) >= len) { \
        return; \
    } \
    dst[off_inout] = ch; \
    off_inout += 1;

#define ABR_TO_INT(a, b, c) \
    (((a) << 24) | ((b) << 16) | ((c) << 8))

inline
bool ptime_upto(char ch, const char *str, off_t &off_inout, ssize_t len)
{
    for (; off_inout < len; off_inout++) {
        if (str[off_inout] == ch) {
            return true;
        }
    }

    return false;
}

inline
bool ptime_upto_end(const char *str, off_t &off_inout, ssize_t len)
{
    off_inout = len;

    return true;
}

bool ptime_b_slow(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len);

inline bool ptime_b(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    if (off_inout + 3 < len) {
        auto month_start = (unsigned char *) &str[off_inout];
        uint32_t month_int =
            ABR_TO_INT(month_start[0] & ~0x20UL,
                       month_start[1] & ~0x20UL,
                       month_start[2] & ~0x20UL);
        int val;

        switch (month_int) {
        case ABR_TO_INT('J', 'A', 'N'):
            val = 0;
            break;
        case ABR_TO_INT('F', 'E', 'B'):
            val = 1;
            break;
        case ABR_TO_INT('M', 'A', 'R'):
            val = 2;
            break;
        case ABR_TO_INT('A', 'P', 'R'):
            val = 3;
            break;
        case ABR_TO_INT('M', 'A', 'Y'):
            val = 4;
            break;
        case ABR_TO_INT('J', 'U', 'N'):
            val = 5;
            break;
        case ABR_TO_INT('J', 'U', 'L'):
            val = 6;
            break;
        case ABR_TO_INT('A', 'U', 'G'):
            val = 7;
            break;
        case ABR_TO_INT('S', 'E', 'P'):
            val = 8;
            break;
        case ABR_TO_INT('O', 'C', 'T'):
            val = 9;
            break;
        case ABR_TO_INT('N', 'O', 'V'):
            val = 10;
            break;
        case ABR_TO_INT('D', 'E', 'C'):
            val = 11;
            break;
        default:
            val = -1;
            break;
        }
        if (val >= 0) {
            off_inout += 3;
            dst->et_tm.tm_mon = val;
            dst->et_flags |= ETF_MONTH_SET;
            return true;
        }
    }

    return ptime_b_slow(dst, str, off_inout, len);
}

inline void ftime_a(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    switch (tm.et_tm.tm_wday) {
        case 0:
            PTIME_APPEND('S');
            PTIME_APPEND('u');
            PTIME_APPEND('n');
            break;
        case 1:
            PTIME_APPEND('M');
            PTIME_APPEND('o');
            PTIME_APPEND('n');
            break;
        case 2:
            PTIME_APPEND('T');
            PTIME_APPEND('u');
            PTIME_APPEND('e');
            break;
        case 3:
            PTIME_APPEND('W');
            PTIME_APPEND('e');
            PTIME_APPEND('d');
            break;
        case 4:
            PTIME_APPEND('T');
            PTIME_APPEND('h');
            PTIME_APPEND('u');
            break;
        case 5:
            PTIME_APPEND('F');
            PTIME_APPEND('r');
            PTIME_APPEND('i');
            break;
        case 6:
            PTIME_APPEND('S');
            PTIME_APPEND('a');
            PTIME_APPEND('t');
            break;
        default:
            PTIME_APPEND('X');
            PTIME_APPEND('X');
            PTIME_APPEND('X');
            break;
    }
}

inline void ftime_Z(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{

}

inline void ftime_b(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    switch (tm.et_tm.tm_mon) {
        case 0:
            PTIME_APPEND('J');
            PTIME_APPEND('a');
            PTIME_APPEND('n');
            break;
        case 1:
            PTIME_APPEND('F');
            PTIME_APPEND('e');
            PTIME_APPEND('b');
            break;
        case 2:
            PTIME_APPEND('M');
            PTIME_APPEND('a');
            PTIME_APPEND('r');
            break;
        case 3:
            PTIME_APPEND('A');
            PTIME_APPEND('p');
            PTIME_APPEND('r');
            break;
        case 4:
            PTIME_APPEND('M');
            PTIME_APPEND('a');
            PTIME_APPEND('y');
            break;
        case 5:
            PTIME_APPEND('J');
            PTIME_APPEND('u');
            PTIME_APPEND('n');
            break;
        case 6:
            PTIME_APPEND('J');
            PTIME_APPEND('u');
            PTIME_APPEND('l');
            break;
        case 7:
            PTIME_APPEND('A');
            PTIME_APPEND('u');
            PTIME_APPEND('g');
            break;
        case 8:
            PTIME_APPEND('S');
            PTIME_APPEND('e');
            PTIME_APPEND('p');
            break;
        case 9:
            PTIME_APPEND('O');
            PTIME_APPEND('c');
            PTIME_APPEND('t');
            break;
        case 10:
            PTIME_APPEND('N');
            PTIME_APPEND('o');
            PTIME_APPEND('v');
            break;
        case 11:
            PTIME_APPEND('D');
            PTIME_APPEND('e');
            PTIME_APPEND('c');
            break;
        default:
            PTIME_APPEND('X');
            PTIME_APPEND('X');
            PTIME_APPEND('X');
            break;
    }
}

inline bool ptime_S(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    PTIME_CONSUME(2, {
        if (str[off_inout + 1] > '9') {
            return false;
        }
        dst->et_tm.tm_sec = (str[off_inout] - '0') * 10 + (str[off_inout + 1] - '0');
    });

    return (dst->et_tm.tm_sec >= 0 && dst->et_tm.tm_sec <= 59);
}

inline void ftime_S(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    PTIME_APPEND('0' + ((tm.et_tm.tm_sec / 10) % 10));
    PTIME_APPEND('0' + ((tm.et_tm.tm_sec /  1) % 10));
}

inline bool ptime_s(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    time_t epoch = 0;

    while (off_inout < len && isdigit(str[off_inout])) {
        epoch *= 10;
        epoch += str[off_inout] - '0';
        off_inout += 1;
    }

    secs2tm(&epoch, &dst->et_tm);
    dst->et_flags = ETF_DAY_SET|ETF_MONTH_SET|ETF_YEAR_SET|ETF_MACHINE_ORIENTED|ETF_EPOCH_TIME;

    return (epoch > 0);
}

inline void ftime_s(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    time_t t = tm2sec(&tm.et_tm);

    snprintf(&dst[off_inout], len - off_inout, "%ld", t);
    off_inout = strlen(dst);
}

inline bool ptime_L(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    int ms = 0;

    PTIME_CONSUME(3, {
        char c0 = str[off_inout];
        char c1 = str[off_inout + 1];
        char c2 = str[off_inout + 2];
        if (!isdigit(c0) || !isdigit(c1) || !isdigit(c2)) {
            return false;
        }
        ms = (
                (str[off_inout    ] - '0') * 100 +
                (str[off_inout + 1] - '0') * 10 +
                (str[off_inout + 2] - '0'));
    });

    if ((ms >= 0 && ms <= 999)) {
        dst->et_nsec = ms * 1000000;
        return true;
    }
    return false;
}

inline void ftime_L(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    int millis = tm.et_nsec / 1000000;

    PTIME_APPEND('0' + ((millis / 100) % 10));
    PTIME_APPEND('0' + ((millis /  10) % 10));
    PTIME_APPEND('0' + ((millis /   1) % 10));
}

inline bool ptime_M(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    PTIME_CONSUME(2, {
        if (str[off_inout + 1] > '9') {
            return false;
        }
        dst->et_tm.tm_min = (str[off_inout] - '0') * 10 + (str[off_inout + 1] - '0');
    });

    return (dst->et_tm.tm_min >= 0 && dst->et_tm.tm_min <= 59);
}

inline void ftime_M(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    PTIME_APPEND('0' + ((tm.et_tm.tm_min / 10) % 10));
    PTIME_APPEND('0' + ((tm.et_tm.tm_min /  1) % 10));
}

inline bool ptime_H(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    PTIME_CONSUME(2, {
        if (str[off_inout + 1] > '9') {
            return false;
        }
        dst->et_tm.tm_hour = (str[off_inout] - '0') * 10 + (str[off_inout + 1] - '0');
    });

    return (dst->et_tm.tm_hour >= 0 && dst->et_tm.tm_hour <= 23);
}

inline void ftime_H(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    PTIME_APPEND('0' + ((tm.et_tm.tm_hour / 10) % 10));
    PTIME_APPEND('0' + ((tm.et_tm.tm_hour /  1) % 10));
}

inline bool ptime_i(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    uint64_t epoch_ms = 0;
    time_t epoch;

    while (off_inout < len && isdigit(str[off_inout])) {
        epoch_ms *= 10;
        epoch_ms += str[off_inout] - '0';
        off_inout += 1;
    }

    dst->et_nsec = (epoch_ms % 1000ULL) * 1000000;
    epoch = (epoch_ms / 1000ULL);
    secs2tm(&epoch, &dst->et_tm);
    dst->et_flags = ETF_DAY_SET|ETF_MONTH_SET|ETF_YEAR_SET|ETF_MACHINE_ORIENTED|ETF_EPOCH_TIME;

    return (epoch_ms > 0);
}

inline void ftime_i(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    int64_t t = tm2sec(&tm.et_tm);

    t += tm.et_nsec / 1000000;
    snprintf(&dst[off_inout], len - off_inout, "%" PRId64, t);
    off_inout = strlen(dst);
}

#include "base/lnav_log.hh"

inline bool ptime_6(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    uint64_t epoch_us = 0;
    time_t epoch;

    while (off_inout < len && isdigit(str[off_inout])) {
        epoch_us *= 10;
        epoch_us += str[off_inout] - '0';
        off_inout += 1;
    }

    dst->et_nsec = (epoch_us % 1000000ULL) * 1000ULL;
    epoch = (epoch_us / 1000000ULL);
    secs2tm(&epoch, &dst->et_tm);
    dst->et_flags = ETF_DAY_SET|ETF_MONTH_SET|ETF_YEAR_SET|ETF_MACHINE_ORIENTED|ETF_EPOCH_TIME;

    return (epoch_us > 0);
}

inline void ftime_6(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    int64_t t = tm2sec(&tm.et_tm);

    t += tm.et_nsec / 1000;
    snprintf(&dst[off_inout], len - off_inout, "%" PRId64, t);
    off_inout = strlen(dst);
}

inline bool ptime_I(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    PTIME_CONSUME(2, {
        if (str[off_inout + 1] > '9') {
            return false;
        }
        dst->et_tm.tm_hour = (str[off_inout] - '0') * 10 + (str[off_inout + 1] - '0');

        if (dst->et_tm.tm_hour < 1 || dst->et_tm.tm_hour > 12) {
            return false;
        }
    });

    return true;
}

inline void ftime_I(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    int hour = tm.et_tm.tm_hour;

    if (hour >= 12) {
        hour -= 12;
    }
    if (hour == 0) {
        hour = 12;
    }

    PTIME_APPEND('0' + ((hour / 10) % 10));
    PTIME_APPEND('0' + ((hour /  1) % 10));
}

inline bool ptime_d(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    PTIME_CONSUME(2, {
        if (str[off_inout] == ' ') {
            dst->et_tm.tm_mday = 0;
        }
        else {
            dst->et_tm.tm_mday = (str[off_inout] - '0') * 10;
        }
        if (str[off_inout + 1] > '9') {
            return false;
        }
        dst->et_tm.tm_mday += (str[off_inout + 1] - '0');
    });

    if (dst->et_tm.tm_mday >= 1 && dst->et_tm.tm_mday <= 31) {
        dst->et_flags |= ETF_DAY_SET;
        return true;
    }
    return false;
}

inline void ftime_d(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    PTIME_APPEND('0' + ((tm.et_tm.tm_mday / 10) % 10));
    PTIME_APPEND('0' + ((tm.et_tm.tm_mday /  1) % 10));
}

inline bool ptime_e(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    dst->et_tm.tm_mday = 0;
    PTIME_CONSUME(1, {
        if (str[off_inout] < '1' || str[off_inout] > '9') {
            return false;
        }
        dst->et_tm.tm_mday = str[off_inout] - '0';
    });
    if (off_inout + 1 < len) {
        if (str[off_inout] >= '0' && str[off_inout] <= '9') {
            dst->et_tm.tm_mday *= 10;
            dst->et_tm.tm_mday += str[off_inout] - '0';
            off_inout += 1;
        }
    }

    if (dst->et_tm.tm_mday >= 1 && dst->et_tm.tm_mday <= 31) {
        dst->et_flags |= ETF_DAY_SET;
        return true;
    }
    return false;
}

inline void ftime_e(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    if (tm.et_tm.tm_mday < 10) {
        PTIME_APPEND(' ');
    }
    else {
        PTIME_APPEND('0' + ((tm.et_tm.tm_mday / 10) % 10));
    }
    PTIME_APPEND('0' + ((tm.et_tm.tm_mday /  1) % 10));
}

inline bool ptime_m(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    off_t orig_off = off_inout;

    dst->et_tm.tm_mon = 0;
    PTIME_CONSUME(1, {
        if (str[off_inout] < '0' || str[off_inout] > '9') {
            return false;
        }
        dst->et_tm.tm_mon = str[off_inout] - '0';
    });
    if (off_inout + 1 < len) {
        if (str[off_inout] >= '0' && str[off_inout] <= '9') {
            dst->et_tm.tm_mon *= 10;
            dst->et_tm.tm_mon += str[off_inout] - '0';
            off_inout += 1;
        }
    }

    dst->et_tm.tm_mon -= 1;

    if (dst->et_tm.tm_mon >= 0 && dst->et_tm.tm_mon <= 11) {
        dst->et_flags |= ETF_MONTH_SET;
        return true;
    }

    off_inout = orig_off;
    return false;
}

inline void ftime_m(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    PTIME_APPEND('0' + (((tm.et_tm.tm_mon + 1) / 10) % 10));
    PTIME_APPEND('0' + (((tm.et_tm.tm_mon + 1) /  1) % 10));
}

inline bool ptime_k(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    dst->et_tm.tm_hour = 0;
    PTIME_CONSUME(1, {
        if (str[off_inout] < '0' || str[off_inout] > '9') {
            return false;
        }
        dst->et_tm.tm_hour = str[off_inout] - '0';
    });
    if (off_inout + 1 < len) {
        if (str[off_inout] >= '0' && str[off_inout] <= '9') {
            dst->et_tm.tm_hour *= 10;
            dst->et_tm.tm_hour += str[off_inout] - '0';
            off_inout += 1;
        }
    }

    return (dst->et_tm.tm_hour >= 0 && dst->et_tm.tm_hour <= 23);
}

inline void ftime_k(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    if (tm.et_tm.tm_hour < 10) {
        PTIME_APPEND(' ');
    }
    else {
        PTIME_APPEND('0' + ((tm.et_tm.tm_hour / 10) % 10));
    }
    PTIME_APPEND('0' + ((tm.et_tm.tm_hour /  1) % 10));
}

inline bool ptime_l(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    off_t orig_off = off_inout;
    bool consumed_space = false;

    dst->et_tm.tm_hour = 0;

    if ((off_inout + 1) > len) {
        return false;
    }

    if (str[off_inout] == ' ') {
        consumed_space = true;
        off_inout += 1;
    }

    if ((off_inout + 1) > len) {
        off_inout = orig_off;
        return false;
    }

    if (str[off_inout] < '1' || str[off_inout] > '9') {
        off_inout = orig_off;
        return false;
    }

    dst->et_tm.tm_hour = str[off_inout] - '0';
    off_inout += 1;

    if (consumed_space || str[off_inout] < '0' || str[off_inout] > '9') {
        return true;
    }

    dst->et_tm.tm_hour *= 10;
    dst->et_tm.tm_hour += str[off_inout] - '0';
    off_inout += 1;

    if (dst->et_tm.tm_hour >= 0 && dst->et_tm.tm_hour <= 23) {
        return true;
    }

    off_inout = orig_off;
    return false;
}

inline void ftime_l(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    int hour = tm.et_tm.tm_hour;

    if (hour >= 12) {
        hour -= 12;
    }
    if (hour == 0) {
        hour = 12;
    }

    if (hour < 10) {
        PTIME_APPEND(' ');
    }
    else {
        PTIME_APPEND('0' + ((hour / 10) % 10));
    }
    PTIME_APPEND('0' + ((hour /  1) % 10));
}

inline bool ptime_p(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    PTIME_CONSUME(2, {
        char lead = str[off_inout];

        if ((str[off_inout + 1] & 0xdf) != 'M') {
            return false;
        }
        else if ((lead & 0xdf) == 'A') {
            if (dst->et_tm.tm_hour == 12) {
                dst->et_tm.tm_hour = 0;
            }
        }
        else if ((lead & 0xdf) == 'P') {
            if (dst->et_tm.tm_hour < 12) {
                dst->et_tm.tm_hour += 12;
            }
        }
        else {
            return false;
        }
    });

    return true;
}

inline void ftime_p(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    if (tm.et_tm.tm_hour < 12) {
        PTIME_APPEND('A');
    }
    else {
        PTIME_APPEND('P');
    }
    PTIME_APPEND('M');
}

inline bool ptime_Y(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    PTIME_CONSUME(4, {
        dst->et_tm.tm_year = (
            (str[off_inout + 0] - '0') * 1000 +
            (str[off_inout + 1] - '0') *  100 +
            (str[off_inout + 2] - '0') *   10 +
            (str[off_inout + 3] - '0') *    1) - 1900;

        if (dst->et_tm.tm_year < 0 || dst->et_tm.tm_year > 1100) {
            return false;
        }

        dst->et_flags |= ETF_YEAR_SET;
    });

    return true;
}

inline void ftime_Y(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    int year = tm.et_tm.tm_year + 1900;

    PTIME_APPEND('0' + ((year / 1000) % 10));
    PTIME_APPEND('0' + ((year /  100) % 10));
    PTIME_APPEND('0' + ((year /   10) % 10));
    PTIME_APPEND('0' + ((year /    1) % 10));
}

inline bool ptime_y(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    PTIME_CONSUME(2, {
        dst->et_tm.tm_year = (
            (str[off_inout + 0] - '0') * 10 +
            (str[off_inout + 1] - '0') *  1);
    });

    if (dst->et_tm.tm_year >= 0 && dst->et_tm.tm_year < 100) {
        if (dst->et_tm.tm_year < 69) {
            dst->et_tm.tm_year += 100;
        }

        dst->et_flags |= ETF_YEAR_SET;
        return true;
    }
    return false;
}

inline void ftime_y(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    int year = tm.et_tm.tm_year + 1900;

    PTIME_APPEND('0' + ((year /   10) % 10));
    PTIME_APPEND('0' + ((year /    1) % 10));
}

inline bool ptime_z(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    int consume_amount = 5;

    if ((off_inout + 6) <= len && str[off_inout + 3] == ':') {
        consume_amount = 6;
    }
    PTIME_CONSUME(consume_amount, {
        long sign;
        long hours;
        long mins;
        int skip_colon = (consume_amount == 6) ? 1 : 0;

        if (str[off_inout] == '+') {
            sign = 1;
        }
        else if (str[off_inout] == '-') {
            sign = -1;
        }
        else {
            return false;
        }

        hours = (
            (str[off_inout + 1] - '0') * 10 +
            (str[off_inout + 2] - '0') *  1) * 60 * 60;
        mins = (
            (str[off_inout + skip_colon + 3] - '0') *   10 +
            (str[off_inout + skip_colon + 4] - '0') *    1) * 60;
        dst->et_gmtoff = sign * (hours + mins);
#ifdef HAVE_STRUCT_TM_TM_ZONE
        dst->et_tm.tm_gmtoff = sign * (hours + mins);
#endif
    });

    return true;
}

inline void ftime_z(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    long gmtoff = std::abs(tm.et_gmtoff) / 60;

    if (tm.et_gmtoff < 0) {
        PTIME_APPEND('-');
    }
    else {
        PTIME_APPEND('+');
    }

    long mins = gmtoff % 60;
    long hours = gmtoff / 60;

    PTIME_APPEND('0' + ((hours / 10) % 10));
    PTIME_APPEND('0' + ((hours /  1) % 10));
    PTIME_APPEND('0' + ((mins  / 10) % 10));
    PTIME_APPEND('0' + ((mins  /  1) % 10));
}

inline bool ptime_f(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    PTIME_CONSUME(6, {
        for (int lpc = 0; lpc < 6; lpc++) {
            if (str[off_inout + lpc] < '0' || str[off_inout + lpc] > '9') {
                return false;
            }
        }
        dst->et_nsec = (
                (str[off_inout + 0] - '0') * 100000 +
                (str[off_inout + 1] - '0') *  10000 +
                (str[off_inout + 2] - '0') *   1000 +
                (str[off_inout + 3] - '0') *    100 +
                (str[off_inout + 4] - '0') *     10 +
                (str[off_inout + 5] - '0') *      1) * 1000;
    });

    return true;
}

inline void ftime_f(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    uint32_t micros = tm.et_nsec / 1000;

    PTIME_APPEND('0' + ((micros / 100000) % 10));
    PTIME_APPEND('0' + ((micros /  10000) % 10));
    PTIME_APPEND('0' + ((micros /   1000) % 10));
    PTIME_APPEND('0' + ((micros /    100) % 10));
    PTIME_APPEND('0' + ((micros /     10) % 10));
    PTIME_APPEND('0' + ((micros /      1) % 10));
}

inline bool ptime_N(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    PTIME_CONSUME(9, {
        for (int lpc = 0; lpc < 9; lpc++) {
            if (str[off_inout + lpc] < '0' || str[off_inout + lpc] > '9') {
                return false;
            }
        }
        dst->et_nsec = (
            (str[off_inout + 0] - '0') * 100000000 +
            (str[off_inout + 1] - '0') *  10000000 +
            (str[off_inout + 2] - '0') *   1000000 +
            (str[off_inout + 3] - '0') *    100000 +
            (str[off_inout + 4] - '0') *     10000 +
            (str[off_inout + 5] - '0') *      1000 +
            (str[off_inout + 6] - '0') *       100 +
            (str[off_inout + 7] - '0') *        10 +
            (str[off_inout + 8] - '0') *         1);
    });

    return true;
}

inline void ftime_N(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{
    uint32_t nano = tm.et_nsec;

    PTIME_APPEND('0' + ((nano / 100000000) % 10));
    PTIME_APPEND('0' + ((nano /  10000000) % 10));
    PTIME_APPEND('0' + ((nano /   1000000) % 10));
    PTIME_APPEND('0' + ((nano /    100000) % 10));
    PTIME_APPEND('0' + ((nano /     10000) % 10));
    PTIME_APPEND('0' + ((nano /      1000) % 10));
    PTIME_APPEND('0' + ((nano /       100) % 10));
    PTIME_APPEND('0' + ((nano /        10) % 10));
    PTIME_APPEND('0' + ((nano /         1) % 10));
}

inline bool ptime_char(char val, const char *str, off_t &off_inout, ssize_t len)
{
    PTIME_CONSUME(1, {
        if (str[off_inout] != val) {
            return false;
        }
    });

    return true;
}

inline void ftime_char(char *dst, off_t &off_inout, ssize_t len, char ch)
{
    PTIME_APPEND(ch);
}

template<typename T>
inline bool ptime_hex_to_quad(T &value_inout, const char quad)
{
    value_inout <<= 4;
    if ('0' <= quad && quad <= '9') {
        value_inout |= ((quad - '0') & 0x0f);
    }
    else if ('a' <= quad && quad <= 'f') {
        value_inout |= 10 + ((quad - 'a') & 0x0f);
    }
    else if ('A' <= quad && quad <= 'F') {
        value_inout |= 10 + ((quad - 'A') & 0x0f);
    }
    else {
        return false;
    }

    return true;
}

inline bool ptime_at(struct exttm *dst, const char *str, off_t &off_inout, ssize_t len)
{
    PTIME_CONSUME(16, {
        int64_t secs = 0;

        for (int lpc = 0; lpc < 16; lpc++) {
            char quad = str[off_inout + lpc];

            if (!ptime_hex_to_quad(secs, quad)) {
                return false;
            }
        }
        dst->et_nsec = 0;

        time_t small_secs = secs - 4611686018427387914ULL;
        secs2tm(&small_secs, &dst->et_tm);
    });

    if ((len - off_inout) == 8) {
        PTIME_CONSUME(8, {
            for (int lpc = 0; lpc < 8; lpc++) {
                char quad = str[off_inout + lpc];

                if (!ptime_hex_to_quad(dst->et_nsec, quad)) {
                    return false;
                }
            }
        });
    }

    dst->et_flags |= ETF_DAY_SET|ETF_MONTH_SET|ETF_YEAR_SET|ETF_MACHINE_ORIENTED|ETF_EPOCH_TIME;

    return true;
}

inline void ftime_at(char *dst, off_t &off_inout, ssize_t len, const struct exttm &tm)
{

}

typedef bool (*ptime_func)(struct exttm *dst, const char *str, off_t &off, ssize_t len);
typedef void (*ftime_func)(char *dst, off_t &off_inout, size_t len, const struct exttm &tm);

bool ptime_fmt(const char *fmt, struct exttm *dst, const char *str, off_t &off, ssize_t len);
size_t ftime_fmt(char *dst, size_t len, const char *fmt, const struct exttm &tm);

struct ptime_fmt {
    const char *pf_fmt;
    ptime_func pf_func;
    ftime_func pf_ffunc;
};

extern struct ptime_fmt PTIMEC_FORMATS[];

extern const char *PTIMEC_FORMAT_STR[];

#endif
